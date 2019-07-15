cmake_minimum_required(VERSION 3.12)

project(xsystem35 LANGUAGES C)

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckLibraryExists)

# A wrapper around find_package() that skips the check if ENABLE_<package> is false
macro (optional_find_package package)
  option(ENABLE_${package} "Use ${package} if available" ON)
  if (ENABLE_${package})
	find_package(${package} ${ARGN})
  endif()
endmacro()

# A wrapper around pkg_check_modules() that skips the check if ENABLE_<prefix> is false
macro (optional_pkg_check_modules prefix)
  option(ENABLE_${prefix} "Use ${prefix} if available" ON)
  if (ENABLE_${prefix})
	pkg_check_modules(${prefix} ${ARGN})
  endif()
endmacro()

check_function_exists(mkdtemp HAVE_MKDTEMP)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
if (NOT HAVE_MKDTEMP)
  include(CheckIncludeFiles)
  check_include_files("stddef.h;stdlib.h;string.h" STDC_HEADERS)
  check_include_file(fcntl.h HAVE_FCNTL_H)
  check_include_file(sys/time.h HAVE_SYS_TIME_H)
  check_include_file(stdint.h HAVE_STDINT_H)
  check_include_file(unistd.h HAVE_UNISTD_H)
endif()

if (EMSCRIPTEN)
  set(SDL2_FOUND 1)
  set(FREETYPE_FOUND 1)
else()
  include(TestBigEndian)
  TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

  if (NOT DEFINED ENABLE_MODULES)
    check_library_exists(dl dlopen "" ENABLE_MODULES)
  endif()
  if (ENABLE_MODULES)
	link_libraries(dl)
  endif()

  find_package(ZLIB REQUIRED)
  optional_find_package(X11)
  optional_find_package(GTK2 COMPONENTS gtk)
  optional_find_package(Freetype)
  include(FindPkgConfig)
  if (NOT X11_FOUND)
	optional_pkg_check_modules(SDL2 sdl2)
  endif()
  optional_pkg_check_modules(SDL2MIXER SDL2_mixer)

  include_directories(
	${ZLIB_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIRS}
	${X11_INCLUDE_DIR}
	${GTK2_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIRS}
	${SDL2MIXER_INCLUDE_DIRS}
	)
  link_libraries(
	${ZLIB_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${X11_LIBRARIES}
	${GTK2_LIBRARIES}
	${SDL2_LIBRARIES}
	${SDL2MIXER_LIBRARIES}
	)
  link_directories(
	${SDL2_LIBRARY_DIRS}
	${SDL2MIXER_LIBRARY_DIRS}
	)
endif()

if (GTK2_FOUND)
  set(ENABLE_GTK 1)
endif()

# Menu

if (EMSCRIPTEN)
  set(SRC_MENU menu_emscripten.c)
elseif (GTK2_FOUND)
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
else()
  set(SRC_MENU menu_null.c)
endif()

# Graphics

if (X11_FOUND)
  set(SRC_GRAPHDEV xcore_draw.c xcore_video.c xcore_mode.c xcore_event.c xcore_cursor.c xcore_maskupdate.c image.c)
  set(SUMMARY_GRAPHDEV "X11")
elseif (SDL2_FOUND)
  set(SRC_GRAPHDEV sdl_video.c sdl_mode.c sdl_draw.c sdl_event.c sdl_image.c sdl_cursor.c image.c)
  set(SUMMARY_GRAPHDEV "SDL2")
  set(ENABLE_SDL 1)
else()
  message(FATAL_ERROR "*** No graphic device available ***")
endif()

# PCM audio

if (EMSCRIPTEN)
  list(APPEND SRC_AUDIO)
  list(APPEND SUMMARY_AUDIO "Emscripten")
elseif (SDL2MIXER_FOUND)
  list(APPEND SRC_AUDIO pcm.sdlmixer.c)
  list(APPEND SUMMARY_AUDIO "SDL_mixer")
  set(ENABLE_SDLMIXER 1)
else()
  list(APPEND SRC_AUDIO pcm.dummy.c)
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
else()
  list(APPEND SRC_CDROM cdrom.empty.c)
endif()
if (SDL2MIXER_FOUND)
  list(APPEND SRC_CDROM cdrom.mp3.c)
  list(APPEND SUMMARY_CDROM "SDL_mixer (wav|mp3|ogg...)")
  set(ENABLE_CDROM_MP3 1)
endif()
set(CDROM_DEVICE "/dev/cdrom" CACHE STRING "CDROM Device Name")

# MIDI

if (EMSCRIPTEN)
  list(APPEND SRC_MIDI midi.emscripten.c)
  list(APPEND SUMMARY_MIDI "Emscripten")
else()
  list(APPEND SRC_MIDI midi.rawmidi.c midifile.c)
  list(APPEND SUMMARY_MIDI "raw")
  set(MIDI_DEVICE "/dev/midi" CACHE STRING "midi device")
  set(ENABLE_MIDI_RAWMIDI 1)
  option(QUIET_MIDI "No display midi stdout" OFF)

  include(CheckSymbolExists)
  if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
	check_symbol_exists(SNDCTL_SEQ_NRSYNTHS machine/soundcard.h ENABLE_MIDI_SEQMIDI)
  elseif (CMAKE_SYSTEM_NAME MATCHES "(NetBSD|OpenBSD)")
	check_symbol_exists(SNDCTL_SEQ_NRSYNTHS soundcard.h ENABLE_MIDI_SEQMIDI)
  else ()
	check_symbol_exists(SNDCTL_SEQ_NRSYNTHS sys/soundcard.h ENABLE_MIDI_SEQMIDI)
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
endif()

# Joystick

if (EMSCRIPTEN)
  set(SRC_JOYSTICK joystick_dmy.c)
elseif (SDL2_FOUND)
  set(SRC_JOYSTICK joystick_sdl.c)
  set(SUMMARY_JOYSTICK "SDL")
  set(HAVE_SDLJOY 1)
else()
  check_include_file(linux/joystick.h HAVE_LINUX_JOYSTICK_H)
  if (HAVE_LINUX_JOYSTICK_H)
	set(SRC_JOYSTICK joystick_linux.c)
	set(SUMMARY_JOYSTICK "Linux ioctl")
	set(JOY_DEVICE "/dev/js0" CACHE STRING "joystick device")
  else ()
	set(SRC_JOYSTICK joystick_dmy.c)
  endif ()
endif()

# Font

if (X11_FOUND)
  list(APPEND SRC_FONT font_x11.c)
  list(APPEND SUMMARY_FONT "x11")
  set(ENABLE_X11FONT 1)
endif()
if (FREETYPE_FOUND)
  list(APPEND SRC_FONT font_freetype2.c cp932tojis0213.c)
  list(APPEND SUMMARY_FONT "freetype2")
  set(ENABLE_FT2 1)
endif()
if (NOT SRC_FONT)
  message(FATAL_ERROR "*** No font device available ***")
endif()

# XFree86extensions

if (X11_FOUND)
  check_library_exists(Xxf86vm XF86VidModeQueryExtension "" HAVE_XF86VMODE)
  if (HAVE_XF86VMODE)
    link_libraries(Xxf86vm)
  endif()
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
print_summary(graphics ${SUMMARY_GRAPHDEV})
print_summary(audio ${SUMMARY_AUDIO})
print_summary(cdrom ${SUMMARY_CDROM})
print_summary(midi ${SUMMARY_MIDI})
print_summary(font ${SUMMARY_FONT})
print_summary(joystick ${SUMMARY_JOYSTICK})
message("------------------")

set(PACKAGE ${PROJECT_NAME})
configure_file(config.h.cmake.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_compile_definitions(HAVE_CONFIG_H)
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

add_subdirectory(src)
if (ENABLE_NLS)
  add_subdirectory(po)
endif()

if (ENABLE_MODULES)
  add_subdirectory(modules)
endif()