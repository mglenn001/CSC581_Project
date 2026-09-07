# CSC 581 Individual Project
This project is my individual game developed using the game engine created by Team 8. While the engine and its core systems are developed collaboratively as a team, my individual work focuses on using those shared engine features to design, implement, and refine my own playable game. The game demonstrates the functionality of the team engine while allowing me to make my own gameplay, design, and implementation decisions.

## Team 8

Mia Glenn, mglenn2

Sabah Afroz, safroz

Tram Nguyen, tnnguy22

##  Setup Instructions

Follow these steps to clone, build, and run the project on your machine:

### 1. Clone the repository

```bash
git clone https://github.com/mglenn001/CSC581_Project.git
```

----------

### 2. Install SDL3

> SDL3 is the latest major release. You’ll need both SDL3 and optionally `SDL3_image` if the sprite sheet is in PNG format.

#### On **macOS** (with Homebrew):

```bash
brew install sdl3

```

#### On **Ubuntu/Debian**:

```bash
sudo apt install libsdl3-dev

```

#### On **Arch Linux**:

```bash
sudo pacman -S sdl3

```

#### On **Windows**:

Use [MSYS2](https://www.msys2.org/) or [vcpkg](https://vcpkg.io/) for easiest installation.

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL3

```

Then launch the MinGW64 shell and continue.

----------

### 3. Build the project

```bash
mkdir build && cd build
cmake ..
make

```

> Ensure `SDL3_DIR` is correctly set if CMake cannot find SDL.

----------

### 5. Run the game

```bash
./main

```

You should see the game screen successfully in the console.

## Game Control

* **A** — Move Left
* **D** — Move Right
* **W** — Jump
* **Shift** — Hold to Run
* **T** — Toggle Resolution Scaling (Proportional vs. Pixel)

## Resources

https://gandalfhardcore.itch.io/2d-pixel-art-male-and-female-character

https://free-game-assets.itch.io/free-swamp-2d-tileset-pixel-art

https://issdev.delta.ncsu.edu/courses/csc481-sprite-selector/
