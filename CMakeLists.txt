cmake_minimum_required(VERSION 3.13)

if (VITA)
  if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if (DEFINED ENV{VITASDK})
	  set(VITASDK "$ENV{VITASDK}")
      set(CMAKE_TOOLCHAIN_FILE "${VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else()
      message(FATAL ERROR "Please define VITASDK to point to your SDK path!")
    endif()
  endif()
endif()

project(xsystem35 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

if (VITA)
  include("${VITASDK}/share/vita.cmake" REQUIRED)
  include_directories("${VITASDK}/arm-vita-eabi/include/SDL2")
  include_directories("${VITASDK}/arm-vita-eabi/include/freetype2")
  include_directories(vita)
  set(VITA_APP_NAME "xsystem35")
  set(VITA_TITLEID "NNHR00001")
  set(VITA_VERSION "00.01")
  set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include vita_port.h")
  add_compile_definitions(VITA=1)
  add_compile_definitions(VITA_TITLEID="${VITA_TITLEID}")
endif()

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckLibraryExists)
include(CheckSymbolExists)

# In msys, `pkg-config --libs` returns `-lmingw32` but find_library() cannot
# find it in default search paths.
if (MSYS)
  list(APPEND CMAKE_LIBRARY_PATH $ENV{MINGW_PREFIX}/$ENV{MINGW_CHOST}/lib)
endif ()

# A wrapper around pkg_check_modules() that skips the check if ENABLE_<prefix> is false
macro (optional_pkg_check_modules prefix)
  option(ENABLE_${prefix} "Use ${prefix} if available" ON)
  if (ENABLE_${prefix})
    pkg_check_modules(${prefix} ${ARGN})
  endif()
endmacro()

check_symbol_exists(getlogin "unistd.h" HAVE_GETLOGIN)
check_symbol_exists(mmap "sys/mman.h" HAVE_MMAP)
check_symbol_exists(sigaction "signal.h" HAVE_SIGACTION)
check_symbol_exists(uname "sys/utsname.h" HAVE_UNAME)

if (EMSCRIPTEN)
  set(HAVE_JPEG 1)
  set(DEFAULT_FONT_PATH /fonts/)
elseif (VITA)
  if (NOT DEFINED ENABLE_MODULES)
    set(ENABLE_MODULES 1)
  endif()
  set(SDL2_FOUND 1)
  set(SDL2MIXER_FOUND 1)
  set(SDL2TTF_FOUND 1)
elseif (ANDROID)
  set(SDL2MIXER_FOUND 1)
  find_library(ndk_zlib z)
  find_library(ndk_log log)
else()
  find_package(ZLIB REQUIRED)

  include(FindPkgConfig)
  optional_pkg_check_modules(GTK3 IMPORTED_TARGET gtk+-3.0)
  if (GTK3_FOUND)
    set(ENABLE_GTK 1)
  endif()

  pkg_check_modules(SDL2 IMPORTED_TARGET sdl2)
  pkg_check_modules(SDL2TTF IMPORTED_TARGET SDL2_ttf)
  optional_pkg_check_modules(SDL2MIXER IMPORTED_TARGET SDL2_mixer)

  option(ENABLE_DEBUGGER "Enable built-in debugger" ON)
  if (ENABLE_DEBUGGER)
    pkg_check_modules(cJSON IMPORTED_TARGET libcjson)
    if (NOT cJSON_FOUND)
      # libcjson-dev of Debian buster / Ubuntu 20.04 does not install pkgconfig files.
      find_library(cJSON cjson)
      if (NOT cJSON)
        message(FATAL_ERROR "libcjson is required but not found.")
      endif()
    endif()
  endif()

  include(FindJPEG)
  if (JPEG_FOUND)
    set(HAVE_JPEG 1)
  endif()

  optional_pkg_check_modules(WEBP IMPORTED_TARGET libwebp)
  if (WEBP_FOUND)
    set(HAVE_WEBP 1)
  endif()

  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads)
  if (CMAKE_USE_PTHREADS_INIT)
    find_library(PORTMIDI portmidi)
    if (PORTMIDI)
      set(HAVE_PORTMIDI 1)
      set(ENABLE_MIDI_PORTMIDI 1)
    endif()
  endif()

  set(DEFAULT_FONT_PATH ${CMAKE_INSTALL_PREFIX}/share/xsystem35/fonts/)
endif()

# Menu

if (EMSCRIPTEN)
  set(SRC_MENU menu_emscripten.c)
elseif (ANDROID)
  set(SRC_MENU menu_android.c)
elseif (VITA)
  set(SRC_MENU menu_ags.c menu_ags_widgets.c)
elseif (ENABLE_GTK)
  set(SRC_MENU menu.c menu_callback.c menu_gui.c s39init.c)

  # i18n support (currently only menus are translated)
  include(FindIntl)
  include(FindGettext)
  if (Intl_FOUND AND GETTEXT_FOUND)
    set(ENABLE_NLS 1)
    add_compile_definitions(LOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale")
    include_directories(${Intl_INCLUDE_DIRS})
    link_libraries(${Intl_LIBRARIES})
  endif()
else ()
  set(SRC_MENU menu_sdl.c sdl_input.c)
endif()

# PCM audio

if (EMSCRIPTEN)
  list(APPEND SRC_AUDIO)
  list(APPEND SRC_AUDIO pcm.emscripten.c bgm.emscripten.c bgi.c)
  list(APPEND SUMMARY_AUDIO "Emscripten")
elseif (SDL2MIXER_FOUND)
  list(APPEND SRC_AUDIO pcm.sdlmixer.c bgm.sdlmixer.c bgi.c)
  list(APPEND SUMMARY_AUDIO "SDL_mixer")
  set(ENABLE_SDLMIXER 1)
else()
  list(APPEND SRC_AUDIO pcm.dummy.c bgm.dummy.c)
endif()

# CDROM

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND SRC_CDROM cdrom.Linux.c)
  list(APPEND SUMMARY_CDROM "Linux ioctl")
  set(ENABLE_CDROM_LINUX 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  list(APPEND SRC_CDROM cdrom.FreeBSD.c)
  list(APPEND SUMMARY_CDROM "*BSD ioctl")
  set(ENABLE_CDROM_BSD 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  list(APPEND SRC_CDROM cdrom.emscripten.c)
  list(APPEND SUMMARY_CDROM "Emscripten")
  set(ENABLE_CDROM_EMSCRIPTEN 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")
  list(APPEND SRC_CDROM cdrom.android.c)
  list(APPEND SUMMARY_CDROM "Android")
  set(ENABLE_CDROM_ANDROID 1)
else()
  list(APPEND SRC_CDROM cdrom.empty.c)
endif()
if (SDL2MIXER_FOUND AND NOT ANDROID)
  list(APPEND SRC_CDROM cdrom.mp3.c)
  list(APPEND SUMMARY_CDROM "SDL_mixer (wav|mp3|ogg...)")
  set(ENABLE_CDROM_MP3 1)
endif()
set(CDROM_DEVICE "playlist.txt" CACHE STRING "CDROM Device Name")

# MIDI

if (EMSCRIPTEN)
  list(APPEND SRC_MIDI midi.emscripten.c)
  list(APPEND SUMMARY_MIDI "Emscripten")
elseif (VITA)
  list(APPEND SRC_MIDI midi.sdlmixer.c)
  list(APPEND SUMMARY_MIDI "SDL_mixer")
  set(ENABLE_MIDI_SDLMIXER 1)
elseif (ANDROID)
  list(APPEND SRC_MIDI midi.android.c)
  list(APPEND SUMMARY_MIDI "Android")
else()
  if (HAVE_SIGACTION)
    list(APPEND SRC_MIDI midi.rawmidi.c midifile.c)
    list(APPEND SUMMARY_MIDI "raw")
    set(MIDI_DEVICE "/dev/midi" CACHE STRING "midi device")
    set(ENABLE_MIDI_RAWMIDI 1)
    option(QUIET_MIDI "No display midi stdout" OFF)
  endif()

  if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    check_symbol_exists(SNDCTL_SEQ_NRSYNTHS "machine/soundcard.h" ENABLE_MIDI_SEQMIDI)
  elseif (CMAKE_SYSTEM_NAME MATCHES "(NetBSD|OpenBSD)")
    check_symbol_exists(SNDCTL_SEQ_NRSYNTHS "soundcard.h" ENABLE_MIDI_SEQMIDI)
  else ()
    check_symbol_exists(SNDCTL_SEQ_NRSYNTHS "sys/soundcard.h" ENABLE_MIDI_SEQMIDI)
  endif()

  if (ENABLE_MIDI_SEQMIDI)
    list(APPEND SUMMARY_MIDI "OSS sequencer")
    set(SEQ_DEVICE "/dev/sequencer" CACHE STRING "sequencer device")
  endif()

  if (SDL2MIXER_FOUND)
    list(APPEND SRC_MIDI midi.sdlmixer.c)
    list(APPEND SUMMARY_MIDI "SDL_mixer")
    set(ENABLE_MIDI_SDLMIXER 1)
  endif()

  if (HAVE_PORTMIDI AND ENABLE_MIDI_PORTMIDI)
    list(APPEND SRC_MIDI midi.portmidi.c)
    list(APPEND SUMMARY_MIDI "PortMidi")
    link_libraries(${PORTMIDI} Threads::Threads)
  endif()
endif()

# Joystick

if (NOT EMSCRIPTEN AND NOT ANDROID)
  set(HAVE_SDLJOY 1)
endif()

# TODO: MMX

function (print_summary name)
  if (ARGN)
    list(JOIN ARGN ", " MSG)
  else()
    set(MSG "none")
  endif()
  message(" ${name}: ${MSG}")
endfunction()

message("xsystem35 summary:")
message("------------------")
print_summary(audio ${SUMMARY_AUDIO})
print_summary(cdrom ${SUMMARY_CDROM})
print_summary(midi ${SUMMARY_MIDI})
print_summary(joystick ${SUMMARY_JOYSTICK})
message("------------------")

set(PACKAGE ${PROJECT_NAME})
configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_compile_definitions(HAVE_CONFIG_H)
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(fonts)
if (ENABLE_NLS)
  add_subdirectory(po)
endif()

if (VITA)
  vita_create_self(xsystem35.self src/xsystem35)
  vita_create_vpk(xsystem35.vpk ${VITA_TITLEID} xsystem35.self
	VERSION ${VITA_VERSION}
	NAME ${VITA_APP_NAME}
	FILE vita/.xsys35rc .xsys35rc
	FILE vita/fonts/mincho.otf fonts/mincho.otf
	FILE vita/fonts/mincho.otf fonts/mincho.otf.license
	FILE vita/fonts/MTLc3m.ttf fonts/MTLc3m.ttf
	FILE vita/fonts/MTLc3m.ttf fonts/MTLc3m.ttf.license
	FILE vita/sce_sys/icon0.png sce_sys/icon0.png
	FILE vita/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
	FILE vita/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
	FILE vita/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
	)
endif()

add_subdirectory(modules)
target_link_libraries(xsystem35 PRIVATE modules)
