# xsystem35-vita

This is a port of [xsystem35-sdl2](https://github.com/kichikuou/xsystem35-sdl2) to the Playstation Vita. xsystem35 is an open-source implementation of AliceSoft's System 3.x game engine.

## Building

Make sure you have the Vita SDK installed and the `$VITASDK` environment variable set correctly.

	mkdir -p out/vita
	cd out/vita
	cmake -DVITA=1 ../../
	make

Then copy the file `xsystem35.vpk` to your Vita and install it as you would any other homebrew.

## Installing games

Create a subdirectory under `ux0:data/xsystem35/` and copy all `.ald` and `.ain` files from the game directory into it.

In order to get BGM, you must rip the CD audio from the game disk and create a playlist pointing to the files (see the section on BGM [here](https://haniwa.website/games/preparing-a-game-directory.html)). xsystem35-vita defaults to looking for a file named `playlist` in the game directory so it is not necessary to create a `.xsys35rc` file.

E.g. the directory for (English) Kichikuou Rance should look something like this:

    ux0:
        data
            xsystem35
                Kichikuou Rance
                    bgm
                        kichiku_2.mp3
                        kichiku_3.mp3
                        ...
                    KICHIKUGA.ALD
                    KICHIKUGB.ALD
                    KICHIKUSA.ALD
                    KICHIKUWA.ALD
                    System39.ain
                    playlist

where the file `playlist` contains:

	(The first line in this file is ignored.)
	bgm/kichiku_2.mp3
	bgm/kichiku_3.mp3
	...

# xsytem35-sdl2

This is a multi-platform port of `xsystem35`, a free implementation of
AliceSoft's System 3.x game engine.

## Unique Features

In addition to the original System 3.x's functionalities, xsystem35-sdl2 has
the following features.

### Playing audio files as fake CD music

Many System 3.x games had music as audio tracks on the CD-ROM. Xsystem35 can
play music from audio files instead, to avoid the hassle of inserting CDs. To
use ripped audio files, create a file named `playlist.txt` in the game
directory, and enter the paths to your tracks, one per line. For example:

```
# The first line is not used
BGM/track02.mp3
BGM/track03.mp3
...
```

The first line is not used, because track 1 of game CD is usually a data track.

### Unicode translation support

The original System 3.x only supported Shift_JIS (a Japanese character
encoding), but xsystem35 supports Unicode and is able to run games translated
into languages other than Japanese and English.

See [xsys35c](https://github.com/kichikuou/xsys35c)'s document for how to
build a game with Unicode mode.

### Debugging

Xsystem35 has a built-in debugger that allows you to step through the game and
examine / modify variables in the game. There are two ways to use the debugger:

- Through [Visual Studio Code](https://code.visualstudio.com/) (recommended):
  The [vscode-system3x](https://github.com/kichikuou/vscode-system3x) extension
  provides graphical debugging interface for System 3.x.
- Using CUI debugger: Running xsystem35 with `-debugger` option will start the
  debugger with console interface. Type `help` to see a list of available
  commands.

## Installing

Prebuilt packages for Windows and Android can be downloaded from the
[Releases](https://github.com/kichikuou/xsystem35-sdl2/releases) page. For
other platforms, see the [Building](#building) section.

## Running
### Windows

Execute `xsytem35`, and it will show a dialog to select a folder. Select the
game folder (where the ALD files are located).

### Android

See [android/README.md](https://github.com/kichikuou/xsystem35-sdl2/blob/master/android/README.md#use).

### Other Platforms

Run xsystem35 from within the game directory.

    cd /path/to/game_directory
    xsystem35

## Building
### Linux (Debian / Ubuntu)

    $ sudo apt install build-essential cmake libgtk-3-dev libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev libwebp-dev libcjson-dev
    $ mkdir -p out/debug
    $ cd out/debug
    $ cmake -DCMAKE_BUILD_TYPE=Debug ../../
    $ make && make install

### MacOS

[Homebrew](https://brew.sh/index_ja) is needed.

    $ brew install cmake pkg-config sdl2 sdl2_mixer sdl2_ttf libjpeg webp cjson
    $ mkdir -p out/debug
    $ cd out/debug
    $ cmake -DCMAKE_BUILD_TYPE=Debug ../../
    $ make && make install

### Windows

[MSYS2](https://www.msys2.org) is needed.

    $ pacman -S cmake mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-libjpeg-turbo mingw-w64-x86_64-libwebp mingw-w64-x86_64-cjson
    $ mkdir -p out/debug
    $ cd out/debug
    $ cmake -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug ../../
    $ make

### Emscripten

    $ mkdir -p out/wasm
    $ cd out/wasm
    $ emcmake cmake -DCMAKE_BUILD_TYPE=MinSizeRel ../../
    $ make

To use the generated binary, checkout
[Kichikuou on Web](https://github.com/kichikuou/web) and copy `out/xsystem35.*`
into its `docs` directory.

### Android

See [android/README.md](android/).
