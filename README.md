# 3D Snake Game

![Screenshot of the game in action](screenshot.png)
_Replace this text with an actual screenshot of the game._

## Overview

This is a classic Snake game implemented in 3D using OpenGL and GLUT. The player controls a snake that moves on a 3D grid, eating apples to grow longer and score points. The game features 3D environments, textured objects, lighting, and collision detection with walls and the snake's own body.

## Features

-   3D rendering of the game world.
-   Textured models for the ground, walls, snake, and apples.
-   Lighting effects to enhance visual realism.
-   Collision detection with game boundaries, internal obstacles, and the snake's body.
-   Score and High Score tracking.
-   "Game Over" screen with restart option.

## How to Build and Run

To build and run this project, you will need a C++ compiler (like g++) and the GLUT library installed on your system.

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/your-username/Snake-3D.git
    cd Snake-3D
    ```

2.  **Compile the source code:**
    Make sure `stb_image.h` is in the same directory as `main.cpp`.
    ```bash
    g++ main.cpp -o snake3d -lGL -lGLU -lglut -lstb_image
    ```
    (Note: `-lstb_image` might not be necessary if `stb_image.h` is compiled as a header-only library directly in `main.cpp`).

3.  **Run the game:**
    ```bash
    ./snake3d
    ```

## Controls

-   **Arrow Keys:** Move the snake (Up, Down, Left, Right).
-   **SPACE:** Restart the game after a "Game Over".

## Dependencies

-   **OpenGL:** For 3D rendering.
-   **GLUT (OpenGL Utility Toolkit):** For window management, input handling, and event loops.
-   **stb_image.h:** A single-file public domain library for loading images.

## Project Structure

-   `main.cpp`: Main source code file containing game logic, rendering, and OpenGL setup.
-   `stb_image.h`: Header-only library for loading image files.
-   `textures/`: Directory containing image files used for textures (e.g., `grass.bmp`, `snake.bmp`, `apple.png`).

## Contributing

Feel free to fork the repository, make improvements, and submit pull requests.


---
_Developed by [Tsedex Ashu]_
_Based on the classic Snake game concept._
