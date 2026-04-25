Windows 2D GUI for OS Simulator (Raylib or SDL2)

What was added
- src/gui/gui_raylib.c: Raylib GUI with buttons and live snapshot text.
- src/gui/gui_sdl2.c: SDL2 GUI with keyboard controls and live status in window title.
- build_gui.ps1: PowerShell builder/runner for either backend.

Prerequisites
- A GCC toolchain on PATH (MSYS2 MinGW/UCRT works).
- For Raylib backend: raylib development libraries installed for your GCC target.
- For SDL2 backend: SDL2 development libraries installed for your GCC target.

MSYS2 (UCRT64) install commands
- Open the UCRT64 shell and run:
  pacman -Syu
- Then install one or both backends:
  pacman -S --needed mingw-w64-ucrt-x86_64-raylib
  pacman -S --needed mingw-w64-ucrt-x86_64-SDL2
- Run VS Code from that shell (or ensure UCRT64 bin is on PATH):
  C:\msys64\ucrt64\bin

Build and run
1. Raylib
   - Build:
     .\build_gui.ps1 -Backend raylib -Mode build
   - Build and run:
     .\build_gui.ps1 -Backend raylib -Mode run

2. SDL2
   - Build:
     .\build_gui.ps1 -Backend sdl2 -Mode build
   - Build and run:
     .\build_gui.ps1 -Backend sdl2 -Mode run

Controls
Raylib GUI
- Click Init / Reset to initialize OS with selected scheduler.
- Click Start/Pause to toggle automatic ticks.
- Click Step for single scheduler step.
- Click Tick x10 for ten manual ticks.
- Keyboard 1/2/3 selects RR/HRRN/MLFQ.

SDL2 GUI
- I: init/reset
- 1/2/3: select RR/HRRN/MLFQ
- Space: start/pause
- N: single step
- T: tick x10
- Esc: exit
- You can also click RR/HRRN/MLFQ chips directly.
- Changing algorithm after init resets the simulation and reapplies the new scheduler.
- GUI mode enables auto-input values so `assign ... input` instructions do not block terminal usage.

Notes
- Program arrivals are loaded automatically at clock 0, 1, and 4 (Program_1/2/3).
- The GUI uses your existing simulator modules; this is a frontend only.
