# GOESP ![Windows](https://github.com/danielkrupinski/GOESP/workflows/Windows/badge.svg?branch=master&event=push)
Cross-platform streamproof ESP hack for CS:GO. Currently supports Windows and Linux.

## Showcase

### Menu

![Menu](https://i.imgur.com/eJ1oDaL.png)

### Purchase List

![Purchase List](https://i.imgur.com/qXvoe6Y.png)

### Player ESP

![Player ESP](https://i.imgur.com/l4cOW0c.png)

### Drag & Drop

![Drag & Drop](https://i.imgur.com/yDhV2eQ.gif)

## Getting started

### Prerequisites

#### Windows
Microsoft Visual Studio 2019 (preferably the latest version), platform toolset v142 and Windows 10 SDK are required in order to compile GOESP. If you don't have ones, you can download VS [here](https://visualstudio.microsoft.com/) (Tools and Windows SDK are installed during Visual Studio Setup).

#### Linux
- CMake 3.11.0+
- gcc and g++ 9.3.0+
- SDL2 library

Below are example commands for some distributions to install the required packages:
##### Ubuntu
    sudo apt install cmake gcc g++ libsdl2-dev
##### Manjaro
    sudo pacman -S cmake gcc g++ sdl2

### Downloading
There are two options of downloading the source code:

#### Without [git](https://git-scm.com)

Choose this option if you want pure source and you're not going to contribute to the repo. Download size ~600 kB.

To download source code this way [click here](https://github.com/danielkrupinski/GOESP/archive/master.zip).

#### With [git](https://git-scm.com)

Choose this option if you're going to contribute to the repo or you want to use version control system. Git is required to step further, if not installed download it [here](https://git-scm.com) or install it from your package manager.

Open git command prompt and enter following command:

    git clone https://github.com/danielkrupinski/GOESP.git

`GOESP` folder should have been succesfully created, containing all the source files.

### Building

#### Windows
Open `GOESP.sln` file, select **Release** configuration and press <kbd>CTRL</kbd>+<kbd>B</kbd>.

#### Linux
Execute the following commands in the main GOESP directory:

    mkdir Release
    cd Release
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j 4

## FAQ

### How do I open menu?
Press <kbd>INSERT</kbd> while focused on CS:GO window.

### Can I use custom font in the ESP?
Of course. After installing new font you have to unload and load GOESP again.

### Does this hack hook any of game engine functions?
Nope. Functions GOESP hooks are:

on windows:
-   DirectX Present & Reset from overlay
-   SetCursorPos from overlay
-   game window WNDPROC

on linux:
- SDL_PollEvent
- SDL_GL_SwapWindow
- SDL_WarpMouseInWindow

### How GOESP renders its stuff?
GOESP hooks game overlays and draw things using them. Currently supported overlays are Steam and Discord.

### Is the ESP visible on recording?
Nope, but you have to use 'Game capture' instead of 'Window capture' or 'Desktop capture'. Also remember to uncheck 'Capture external overlays' option in your recording software.

### Can I use this alongside other cheats?
Yes, GOESP shouldn't collide with any other game modification.

## License

> Copyright (c) 2019-2020 Daniel Krupiński

This project is licensed under the [MIT License](https://opensource.org/licenses/mit-license.php) - see the [LICENSE](LICENSE) file for details.
