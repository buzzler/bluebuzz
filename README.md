# BlueBuzz

BlueBuzz is an ESP32-based controller that bridges Bluetooth Gamepads, Keyboards, and Mice to MSX computers via GPIO pins. It supports multiple input modes and can be updated over-the-air (OTA).

## Features

- **Multi-Mode Support**:
  - Gamepad mode with turbo buttons
  - Keyboard mode for MSX keyboard emulation
  - Mouse mode for MSX mouse support
- **Bluetooth Connectivity**:
  - Automatic pairing with Bluetooth controllers
  - Support for multiple controller types
- **OTA Updates**: Over-the-air firmware updates via web interface
- **Rumble Feedback**: Vibration feedback for gamepad input
- **LED Indicators**: Visual status feedback

## Hardware Requirements

- ESP32 development board
- Bluetooth controller (compatible with Bluepad32)
- MSX computer with GPIO expansion capability
- 10kΩ pull-up resistors for buttons (if needed)

## Pin Configuration

| Function | GPIO |
|----------|------|
| UP Button | 14 |
| DOWN Button | 26 |
| LEFT Button | 33 |
| RIGHT Button | 32 |
| A Button | 27 |
| B Button | 25 |
| OUT Signal | 17 |
| LED Indicator | 18 |

![schematic](./KiCad/schematic.png)

## Modes of Operation

### Gamepad Mode
- Supports standard gamepad controls
- Turbo buttons (L1/R1) for adjustable rapid fire
- D-Pad and analog stick support

### Keyboard Mode
- Emulates MSX keyboard input
- Arrow keys and WASD for movement
- N/Space/Enter for action buttons
- M/Esc for secondary actions

### Mouse Mode
- Converts controller mouse movements to MSX mouse input
- Button mapping for left/right/middle clicks
- Scroll wheel support

## Setup Instructions

1. Install required libraries:
   - ESP32 Board Support
   - Bluepad32
   - ESPAsyncWebServer
   - ElegantOTA

2. Upload firmware to ESP32 board

3. Connect to WiFi network "BlueBuzz" (default password: none)

4. Access OTA update interface at `http://192.168.1.1/update`

## Usage

### Connecting Controllers
1. Power on BlueBuzz
2. Press and hold SELECT+START on controller for 3 seconds to enter pairing mode
3. Pair with your Bluetooth controller

### Switching Modes
- **Pairing Mode**: SELECT + START (up)
- **OTA Update**: SELECT + START (down)

## Technical Details

### State Machine
The device uses a state machine to manage different operational modes:
1. INIT - System initialization
2. PAIRING - Bluetooth pairing mode
3. CONNECTED - Waiting for controller connection
4. GAMEPAD - Gamepad input processing
5. KEYBOARD - Keyboard input processing
6. MOUSE - Mouse input processing
7. OTA - Over-the-air update mode
8. ERROR - Error handling

### Input Processing
- Gamepad: Direct button mapping with turbo functionality
- Keyboard: Key mapping to MSX keyboard codes
- Mouse: Relative movement conversion to MSX mouse commands

### Power Management
- CPU frequency set to 80MHz for stability
- Efficient power usage in idle states

## Troubleshooting

### Common Issues
1. **Controller not connecting**:
   - Ensure device is in pairing mode (LED blinking)
   - Check Bluetooth compatibility
   - Try forgetting previous connections

2. **No input detection**:
   - Verify pin connections
   - Confirm controller is properly paired
   - Check MSX GPIO configuration

3. **OTA Update fails**:
   - Ensure stable WiFi connection
   - Verify correct firmware file
   - Check available storage space

## License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0).

## Acknowledgments

- Uses Bluepad32 library for Bluetooth controller support
- ESPAsyncWebServer and ElegantOTA for OTA updates

- Inspired by MSX computing community projects
