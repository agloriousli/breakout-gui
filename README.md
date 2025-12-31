# Breakout Game

A modern implementation of the classic Breakout arcade game built with C++ and Qt 6, featuring dynamic gameplay mechanics, visual effects, and an intuitive GUI.

## Features

- **Classic Breakout Gameplay**: Destroy bricks with a bouncing ball controlled by a paddle
- **Visual Effects**: Ball trails, impact flashes, particle explosions, and score popups
- **Power-ups**: Paddle expansion and slow-motion abilities
- **Multiple Levels**: Progressive difficulty with increasing brick layouts
- **Score Multipliers**: Combo system for consecutive brick destruction
- **Game Persistence**: Save and load endgame states
- **Smooth Animation**: 60 FPS rendering with physics-based particle effects

## Prerequisites

### System Requirements

- **macOS 11.0+** or **Linux** (Ubuntu 20.04+)
- **CMake 3.16+**
- **C++ 17 compiler** (Clang or GCC)
- **Qt 6.0+** (Core, Gui, Widgets modules)

### Installing Dependencies

#### macOS (using Homebrew)

```bash
brew install cmake qt@6
```

#### Ubuntu/Debian

```bash
sudo apt-get install cmake qt6-base-dev qt6-tools-dev
```

## Building the Game

### Step 1: Clone or Navigate to Project

```bash
cd /path/to/breakout-gui
```

### Step 2: Create Build Directory

```bash
mkdir -p build-gui
cd build-gui
```

### Step 3: Configure with CMake

```bash
cmake .. -DBUILD_APP=ON
```

This enables the Qt GUI application build. Other options:
- `-DBUILD_TESTING=ON` (default): Build unit tests
- `-DBUILD_TESTING=OFF`: Skip tests
- `-DCMAKE_BUILD_TYPE=Release`: Optimized release build (default is Debug)

### Step 4: Compile

```bash
cmake --build . --parallel
```

### Step 5: Run the Game

```bash
./breakout_app
```

## Quick Build & Run

For a one-line build and launch:

```bash
cd /Users/glorial/breakout-gui && mkdir -p build-gui && cd build-gui && cmake .. -DBUILD_APP=ON && cmake --build . && ./breakout_app
```

## Game Controls

| Key | Action |
|-----|--------|
| **A** or **←** | Move paddle left |
| **D** or **→** | Move paddle right |
| **Space** / **Enter** | Launch ball / Confirm |
| **P** or **Esc** | Pause/Resume game |
| **S** | Save current game (while paused) |
| **M** | Return to main menu |
| **R** | Restart current level |

## Gameplay

1. **Main Menu**: Select game difficulty (ball speed) and starting level
2. **Pre-Launch**: Ball is attached to paddle; press Space to launch
3. **Active Play**: Use A/D or arrow keys to move the paddle and hit the ball
4. **Power-ups**: Collect falling icons:
   - 🟢 **Green**: Expands paddle width temporarily
   - 🔵 **Blue**: Slows ball speed temporarily
5. **Level Progression**: Clear all bricks to advance to the next level
6. **Game Over**: Lose all lives and the game ends; restart to play again

## Architecture

### Project Structure

```
breakout-gui/
├── src/
│   ├── core/              # Game engine logic
│   │   ├── entities/      # Ball, Paddle, Brick classes
│   │   ├── game/          # GameEngine, PhysicsEngine, LevelManager
│   │   └── utils/         # Vector2D, collision detection, random
│   ├── ui/                # Qt GUI components
│   │   ├── main_window.cpp/h
│   │   ├── game_widget.cpp/h
│   │   └── endgame_editor.cpp/h
│   ├── data/              # Data persistence
│   │   ├── config_manager.cpp/h
│   │   └── endgame_manager.cpp/h
│   └── main.cpp           # Application entry point
├── CMakeLists.txt         # Build configuration
└── docs/                  # Additional documentation
```

### Key Components

- **GameEngine** (`src/core/game/game_engine.cpp`): Core game logic, state management, score tracking
- **GameWidget** (`src/ui/game_widget.cpp`): Qt widget rendering, input handling, visual effects
- **PhysicsEngine** (`src/core/game/physics_engine.cpp`): Ball physics, collision detection, paddle movement
- **LevelManager** (`src/core/game/level_manager.cpp`): Brick layouts and level progression

## Configuration

Game settings are stored in Qt configuration format and can be modified through the GUI:

- **Ball Speed**: Slider from slow (1) to fast (10)
- **Starting Level**: Choose initial difficulty level
- **Random Seed**: For consistent or varied gameplay

## Troubleshooting

### Build Fails with "Qt6 not found"

Ensure Qt 6 is installed and CMake can find it:

```bash
cmake .. -DBUILD_APP=ON -DQt6_DIR=$(brew --prefix qt6)/lib/cmake/Qt6
```

### Missing Dependencies

On Ubuntu, if headers are missing:

```bash
sudo apt-get install qt6-base-dev qt6-tools-dev libgl1-mesa-dev
```

### Performance Issues

- Reduce visual effects complexity in `game_widget.cpp`
- Lower the ball speed setting
- Close other applications

## Development

### Running Tests

```bash
cd build-gui
cmake .. -DBUILD_TESTING=ON
cmake --build .
ctest --output-on-failure
```

### Code Style

The project follows:
- Modern C++17 conventions
- Qt coding guidelines
- Comments for complex logic sections

## Future Enhancements

- Sound effects and background music
- Additional power-up types
- Online leaderboards
- Mobile touch controls
- Custom level editor

## License

This project is provided as-is for educational purposes.

## Support

For issues or questions, review the code comments and check the `docs/` directory for additional documentation.




