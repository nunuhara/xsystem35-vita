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

## Known Issues

* *Startup time for some games (especially KR) is really bad.* This is caused by the game rendering a bunch of invisible text at startup (TTF font rendering on Vita is pretty slow). I have a plan to fix this (use system font rendering API).

* *The game looks ugly/blurry.* The Vita has a vertical resolution of 544 pixels, whereas System 3.x games usually run at a slightly lower resolution (400 or 480). Slight upscaling like this inherently produces a lousy result. You can run games at their original resolution if your eyes start bleeding: just hit the Start button to open the menu and switch fullscreen off.

# xsytem35-sdl2

アリスソフトのゲームエンジン System3.x のフリー実装である xsystem35 を SDL2 に対応して、emscripten でコンパイルできるようにしたものです。

## ビルド方法
### Linux

[cmake](https://cmake.org/) が必要です。

    $ mkdir -p out/debug
    $ cd out/debug
    $ cmake -DCMAKE_BUILD_TYPE=Debug ../../
    $ make && make install

cmake の実行でエラーになる場合は必要なライブラリをインストールしてください。

グラフィックスシステムとして X11 と SDL2 が使用可能です。両方存在する場合は X11 が優先されますが、`cmake` のオプションに `-DENABLE_X11=NO` を指定すると SDL2 が使われます。

### MacOS

[Homebrew](https://brew.sh/index_ja) が必要です。

    $ brew install cmake pkg-config sdl2 sdl2_mixer freetype
    $ mkdir -p out/debug
    $ cd out/debug
    $ cmake -DCMAKE_BUILD_TYPE=Debug ../../
    $ make && make install

### Emscripten

    $ mkdir -p out/wasm
    $ cd out/wasm
    $ emcmake cmake -DCMAKE_BUILD_TYPE=MinSizeRel ../../
    $ make

実行するには、[鬼畜王 on Webのリポジトリ](https://github.com/kichikuou/web)をチェックアウトして、`docs`ディレクトリに `out/xsystem35.*` をすべてコピーしてください。
