# Limonada - sprite editor

Not much yet.

## building

Install sdl2 and sdl_image; build using makefile.

### gnu + linux

On __Ubuntu 14.04 and above__, type:\
`apt install libsdl2{,-image,-mixer,-ttf,-gfx}-dev`

On __Fedora 25 and above__, type:\
`yum install SDL2{,_image,_mixer,_ttf,_gfx}-devel`

On __Arch Linux__, type:\
`pacman -S sdl2{,_image,_mixer,_ttf,_gfx}`

On __Gentoo__, type:\
`emerge -av libsdl2 sdl2-{image,mixer,ttf,gfx}`

### mac

On __macOS__, install SDL2 via [Homebrew](http://brew.sh) like so:\
`brew install sdl2{,_image,_mixer,_ttf,_gfx} pkg-config`

### windows

On __Windows__,
1. Install mingw-w64 from [Mingw-builds](http://mingw-w64.org/doku.php/download/mingw-builds)
	* Version: latest (at time of writing 6.3.0)
	* Architecture: x86_64
	* Threads: win32
	* Exception: seh
	* Build revision: 1
	* Destination Folder: Select a folder that your Windows user owns
2. Install SDL2 http://libsdl.org/download-2.0.php
	* Extract the SDL2 folder from the archive using a tool like [7zip](http://7-zip.org)
	* Inside the folder, copy the `i686-w64-mingw32` and/or `x86_64-w64-mingw32` depending on the architecture you chose into your mingw-w64 folder e.g. `C:\Program Files\mingw-w64\x86_64-6.3.0-win32-seh-rt_v5-rev1\mingw64`
3. Setup Path environment variable
	* Put your mingw-w64 binaries location into your system Path environment variable. e.g. `C:\Program Files\mingw-w64\x86_64-6.3.0-win32-seh-rt_v5-rev1\mingw64\bin` and `C:\Program Files\mingw-w64\x86_64-6.3.0-win32-seh-rt_v5-rev1\mingw64\x86_64-w64-mingw32\bin`
4. You can repeat __Step 2__ for [SDL_image](https://www.libsdl.org/projects/SDL_image), [SDL_mixer](https://www.libsdl.org/projects/SDL_mixer), [SDL_ttf](https://www.libsdl.org/projects/SDL_ttf)

* Or you can install SDL2 via [Msys2](https://msys2.github.io) like so:
`pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2{,_image,_mixer,_ttf,_gfx}`

## license

GPLv3 or any later version. All code must be given to the project and licensed
under these terms before merging.

I do not plan to sell either the source code or binaries, and will instead
distribute them gratis; anyone asking you for money for this project has ripped
you off.

## credits

[Font is tewi2a by lucy.](https://github.com/lucy/tewi-font)

[SDL install instructions from here.](https://github.com/veandco/go-sdl2)

[Name suggested by PAUSE BREAK.](https://www.youtube.com/channel/UCLcDO-_j-ElBTrVcoaTfMKw)
