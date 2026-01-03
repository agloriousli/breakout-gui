# Breakout Game

A modern implementation of the classic Breakout arcade game built with C++ and Qt 6, featuring dynamic gameplay mechanics, visual effects, power-ups, sound effects, and an intuitive graphical user interface.

## Features Overview

- 🎮 **Classic Breakout Gameplay** with modern physics and controls
- 🎨 **Rich Visual Effects** including ball trails, particle explosions, and screen flashes
- ⚡ **5 Power-up Types** for enhanced gameplay variety
- 🎵 **Sound Effects & Background Music** for immersive experience
- 📊 **Combo Scoring System** rewarding consecutive hits
- 💾 **Game Persistence** with save/load endgame functionality
- 🎚️ **Customizable Configuration** for difficulty and preferences
- 🏗️ **Endgame Editor** for creating custom challenges

---

## Detailed Feature Description

### Core Game Mechanics

#### Ball and Paddle Physics
- **Swept AABB Collision Detection**: Precise collision handling that prevents tunneling through objects
- **Paddle Reflection System**: Ball angle changes based on where it hits the paddle
  - Center hits → vertical bounce
  - Edge hits → angled deflection (up to ±60°)
- **Constant Speed Maintenance**: Ball speed remains consistent after collisions
- **Pre-launch Mode**: Ball attaches to paddle before launch, moving horizontally with it

#### Brick System

| Brick Type | Appearance | Hits Required | Points |
|------------|------------|---------------|--------|
| **Normal** | Green | 1 | 10 |
| **Durable** | Orange | 2 | 20 |
| **Indestructible** | Gray | ∞ | 0 |

- Durable bricks display remaining hit count
- Indestructible bricks alter ball trajectory without being destroyed

#### Scoring System
- **Base Points**: 10 points per normal brick, 20 for durable bricks
- **Combo Multiplier**: 2× score for consecutive hits within 2 seconds
- **Score Popups**: Floating text displays points earned at destruction location
- **Real-time HUD**: Score, level, and lives displayed at top of screen

### Power-Up System

Power-ups drop randomly (20% chance) when bricks are destroyed:

| Power-Up | Color | Effect | Duration |
|----------|-------|--------|----------|
| 🟢 **Expand Paddle** | Green | Increases paddle width by 50% | 10 seconds |
| ❤️ **Extra Life** | Red | Adds +1 life | Instant |
| 🔵 **Speed Boost** | Blue | Increases paddle movement speed | 8 seconds |
| 🟡 **Point Multiplier** | Gold | 2× score for all hits | 15 seconds |
| 🟣 **Big Ball** | Purple | Increases ball size for easier hits | 12 seconds |

- Power-ups fall from destroyed brick locations
- Collect by touching with the paddle
- Active power-ups have visual indicators

### Visual Effects

| Effect | Description |
|--------|-------------|
| **Ball Trail** | Fading trail following ball movement for motion clarity |
| **Particle Explosions** | Colored particles burst from destroyed bricks |
| **Screen Flash** | Brief white flash on brick destruction |
| **Score Popups** | Animated floating text showing points earned |
| **Overlay Transitions** | Smooth fade transitions for game state changes |

- 60 FPS rendering with anti-aliased graphics
- Gradient fills and glow effects on game objects
- Dark gray background for optimal contrast

### Sound System

| Sound Effect | Trigger |
|--------------|---------|
| 🔊 `ball_hit.wav` | Ball destroys a brick |
| 💀 `game_over.wav` | All lives lost |
| ⭐ `powerup.wav` | Power-up collected |
| 🏆 `victory.wav` | All levels completed |
| 💔 `life_lost.wav` | Ball falls below paddle |
| 🎵 `bgm.wav` | Background music (loops continuously) |

- Background music pauses during game pause and resumes on continue
- Sound effects use Qt's QSoundEffect for low-latency playback
- BGM uses QMediaPlayer with loop support

### Level System

- **3 Default Levels** with progressive difficulty
- **Level Progression**: Clear all destructible bricks to advance
- **Difficulty Scaling**:
  - Ball speed increases with each level
  - More durable and indestructible bricks in later levels
- **Level Complete Overlay**: "Press Enter for Next Level"
- **Victory Screen**: Displayed after completing all levels

### Game States

| State | Description |
|-------|-------------|
| **PreLaunch** | Ball attached to paddle, waiting for launch |
| **Active** | Normal gameplay in progress |
| **Paused** | Game frozen, overlay displayed |
| **LevelComplete** | All bricks cleared, awaiting next level |
| **Victory** | All levels completed successfully |
| **GameOver** | All lives lost, final score displayed |

### Lives System
- **3 Lives** by default
- Life lost when ball falls below paddle
- **Life Loss Overlay**: Shows remaining lives or Game Over
- Visual life indicator in HUD

---

## Configuration System

### Customizable Settings

Access via main menu to adjust:

| Setting | Range | Description |
|---------|-------|-------------|
| **Ball Speed** | 1-10 | Base speed of ball movement |
| **Starting Level** | 1-3 | Initial level to begin game |
| **Random Seed** | -1 or integer | -1 uses system time; fixed value for reproducible layouts |

- Configurations saved to `config/` directory
- Load/Save buttons in main menu
- Input validation for all values

### Endgame System

#### Endgame Editor
Create custom brick layouts with the built-in editor:

- **Grid-based Editor**: Visual placement of bricks
- **Brick Type Selection**: Normal, Durable, or Indestructible
- **Click to Place**: Left-click places bricks, right-click removes
- **Map Size Configuration**: Customizable grid dimensions
- **Initial Level Setting**: Choose starting difficulty
- **Save/Load**: Store challenges in `endgames/` directory

#### Pause-Save Feature
Save game state during pause (press **S**):
- Captures current brick layout
- Records current level
- Compatible with endgame loader
- Resume challenges later or share with others

---

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
| **Space** / **Enter** | Launch ball / Confirm selection |
| **P** or **Esc** | Pause/Resume game |
| **S** | Save current game state (while paused) |
| **M** | Return to main menu |
| **R** | Restart current level |

---

## Gameplay Guide

### Getting Started
1. **Main Menu**: Adjust ball speed slider and select starting level
2. **Load Options**: Optionally load a saved configuration or endgame challenge
3. **Start Game**: Click "Start Game" to begin

### During Gameplay
1. **Pre-Launch**: Ball is attached to paddle; use A/D to position, press Space to launch
2. **Active Play**: Move paddle to keep ball in play and destroy bricks
3. **Collect Power-ups**: Catch falling power-up icons with your paddle
4. **Clear Level**: Destroy all normal and durable bricks to advance
5. **Avoid Ball Loss**: Don't let the ball fall below the paddle

### Power-up Guide
- 🟢 **Green (Expand)**: Your paddle becomes wider - easier to hit the ball
- ❤️ **Red (Extra Life)**: Gain an additional life immediately
- 🔵 **Blue (Speed Boost)**: Paddle moves faster for quick saves
- 🟡 **Gold (Multiplier)**: All points doubled for 15 seconds
- 🟣 **Purple (Big Ball)**: Larger ball is easier to track and hit bricks

### Tips for High Scores
- Aim for brick clusters to maximize combo multiplier
- Collect Point Multiplier power-ups before destroying many bricks
- Use paddle edges to angle the ball toward hard-to-reach areas
- Save Extra Life power-ups are rare - play carefully!

---

## Architecture

### Project Structure

```
breakout-gui/
├── src/
│   ├── core/                    # Game engine logic
│   │   ├── entities/            # Game object classes
│   │   │   ├── ball.cpp/h       # Ball physics and state
│   │   │   ├── paddle.cpp/h     # Paddle movement and sizing
│   │   │   ├── brick.cpp/h      # Brick types and hit tracking
│   │   │   └── powerup.cpp/h    # Power-up types and effects
│   │   ├── game/                # Core game systems
│   │   │   ├── game_engine.cpp/h      # Main game logic and state
│   │   │   ├── physics_engine.cpp/h   # Collision and movement
│   │   │   └── level_manager.cpp/h    # Level layouts and progression
│   │   └── utils/               # Utility classes
│   │       ├── vector2d.cpp/h   # 2D vector math
│   │       ├── collision.cpp/h  # AABB collision detection
│   │       └── random.cpp/h     # Random number generation
│   ├── ui/                      # Qt GUI components
│   │   ├── main_window.cpp/h    # Main application window
│   │   ├── game_widget.cpp/h    # Game rendering and input
│   │   └── endgame_editor.cpp/h # Endgame creation dialog
│   ├── data/                    # Data persistence
│   │   ├── config_manager.cpp/h # Configuration save/load
│   │   └── endgame_manager.cpp/h # Endgame file handling
│   └── main.cpp                 # Application entry point
├── sounds/                      # Audio assets
│   ├── ball_hit.wav             # Brick destruction sound
│   ├── game_over.wav            # Game over sound
│   ├── powerup.wav              # Power-up collection sound
│   ├── victory.wav              # Victory fanfare
│   ├── life_lost.wav            # Life lost sound
│   └── bgm.wav                  # Background music
├── config/                      # User configurations
├── endgames/                    # Saved endgame challenges
├── CMakeLists.txt               # Build configuration
└── README.md                    # This file
```

### Key Components

| Component | File | Responsibility |
|-----------|------|----------------|
| **GameEngine** | `src/core/game/game_engine.cpp` | Core game logic, state machine, scoring, lives |
| **PhysicsEngine** | `src/core/game/physics_engine.cpp` | Collision detection, ball/paddle movement, physics |
| **LevelManager** | `src/core/game/level_manager.cpp` | Brick layouts, level progression, difficulty |
| **GameWidget** | `src/ui/game_widget.cpp` | Qt rendering, input handling, visual effects, sound |
| **MainWindow** | `src/ui/main_window.cpp` | Menu system, configuration UI, window management |
| **EndgameEditor** | `src/ui/endgame_editor.cpp` | Visual brick layout editor dialog |
| **ConfigManager** | `src/data/config_manager.cpp` | Configuration file reading/writing |
| **EndgameManager** | `src/data/endgame_manager.cpp` | Endgame file parsing and saving |

### Technical Highlights

- **Swept AABB Collision**: Handles fast-moving ball without tunneling through objects
- **Qt6 Multimedia**: QSoundEffect for sound effects, QMediaPlayer for background music
- **60 FPS Game Loop**: QTimer-driven update cycle for smooth animation
- **State Machine**: Clean game state transitions with overlay system
- **Particle System**: Physics-based particles for visual feedback

---

## Configuration

### In-Game Settings

Game settings accessible through the main menu GUI:

| Setting | Range | Default | Description |
|---------|-------|---------|-------------|
| **Ball Speed** | 1-10 | 5 | Controls base ball movement speed |
| **Starting Level** | 1-3 | 1 | Which level to begin the game |
| **Random Seed** | -1 or int | -1 | Seed for procedural elements (-1 = random) |

### Configuration Files

Configurations are stored in `config/` directory:

```
config/
├── default.config    # Default settings
└── custom.config     # User-created configurations
```

### Endgame Files

Custom challenges stored in `endgames/` directory:

```
endgames/
├── example.end       # Sample endgame
└── challenge1.end    # User-created challenges
```

File format example:
```
9 18       // Map size (width × height)
3          // Starting level
P 3 4 #    // Place durable brick at (3,4)
P 3 5 @    // Place normal brick at (3,5)
P 4 5 *    // Place indestructible brick at (4,5)
f          // End of layout
```

---

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

---

## Feature Summary

### Basic Game Features ✅
- [x] Main menu with game controls
- [x] Ball and paddle physics with collision detection
- [x] Three brick types (Normal, Durable, Indestructible)
- [x] Scoring system with real-time display
- [x] Multiple levels with progression
- [x] Lives system with game over handling
- [x] Pause/Resume functionality

### Peripheral Features ✅
- [x] Custom configuration save/load
- [x] Endgame editor for custom challenges
- [x] Pause-save game state feature
- [x] Settings persistence

### Extended Features ✅
- [x] Full Qt6 GUI (graphical interface)
- [x] 5 power-up types with visual effects
- [x] Particle explosion system
- [x] Ball trail effect
- [x] Screen flash on impacts
- [x] Score popup animations
- [x] Sound effects for all actions
- [x] Background music with loop
- [x] Combo multiplier system

---

## Future Enhancements

- [ ] Additional power-up types (multi-ball, laser paddle)
- [ ] Online leaderboards
- [ ] More level layouts
- [ ] Custom themes and skins
- [ ] Mobile touch controls
- [ ] Achievement system

---

## License

This project is provided as-is for educational purposes.

## Support

For issues or questions, review the code comments and architecture documentation above.




