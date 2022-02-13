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

This is a multi-platform port of xsystem35, a free implementation of AriceSoft's System3.x game engine.

## Download
Prebuilt binaries for Windows and Android can be downloaded from the [Releases](https://github.com/kichikuou/xsystem35-sdl2/releases) page.

## Build
### Linux (Debian / Ubuntu)

    $ sudo apt install build-essential cmake libgtk2.0-dev libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev
    $ mkdir -p out/debug
    $ cd out/debug
    $ cmake -DCMAKE_BUILD_TYPE=Debug ../../
    $ make && make install

### MacOS

[Homebrew](https://brew.sh/index_ja) is needed.

    $ brew install cmake pkg-config sdl2 sdl2_mixer freetype libjpeg
    $ mkdir -p out/debug
    $ cd out/debug
    $ cmake -DCMAKE_BUILD_TYPE=Debug ../../
    $ make && make install

### Windows

[MSYS2](https://www.msys2.org) is needed.

    $ pacman -S cmake mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-libjpeg-turbo
    $ mkdir -p out/debug
    $ cd out/debug
    $ cmake -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug ../../
    $ make

### Emscripten

    $ mkdir -p out/wasm
    $ cd out/wasm
    $ emcmake cmake -DCMAKE_BUILD_TYPE=MinSizeRel ../../
    $ make

To use the generated binary, checkout [Kichikuou on Web](https://github.com/kichikuou/web) and copy `out/xsystem35.*` into its `docs` directory.

### Android

See [android/README.md](android/).
