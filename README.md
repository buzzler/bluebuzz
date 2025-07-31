# BlueBuzz

BlueBuzz is an Arduino project that enables Bluetooth gamepads to interface with MSX-style digital joystick inputs. Using the Bluepad32 library, BlueBuzz translates modern controller inputs into signals compatible with retro MSX hardware, supporting turbo functionality, rumble feedback, and multi-controller management.

## Features

- **Bluetooth Gamepad Support:** Connect up to two Bluetooth controllers simultaneously.
- **MSX Joystick Emulation:** Maps controller inputs to MSX joystick pins for seamless retro gaming.
- **Turbo Functionality:** Adjustable rapid-fire for A/B buttons using shoulder and trigger controls.
- **Rumble Feedback:** Provides tactile feedback when turbo settings are changed or limits are reached.
- **Controller Management:** Automatically handles connection/disconnection and enforces a connection limit.
- **Forget All Controllers:** Reset all connections using SELECT + START.

## Hardware Requirements

- ESP32-based board (compatible with Bluepad32)
- MSX or similar retro hardware expecting digital joystick inputs
- Bluetooth gamepad(s) (e.g., Xbox, PlayStation, Switch Pro, etc.)
- Wiring from ESP32 pins to MSX joystick port

## Pin Mapping

Each player uses a set of six pins for MSX joystick signals:

| Signal | Player 1 Pin | Player 2 Pin |
|--------|--------------|--------------|
| UP     | 23           | 4            |
| DOWN   | 19           | 14           |
| LEFT   | 18           | 15           |
| RIGHT  | 5            | 27           |
| A      | 22           | 26           |
| B      | 21           | 25           |

## Installation

1. **Clone the Repository:**  
    Download or clone the BlueBuzz project files to your Arduino workspace.

2. **Install Bluepad32 Library:**  
    Add the [Bluepad32](https://github.com/ricardoquesada/bluepad32) library to your Arduino IDE.

3. **Connect Hardware:**  
    Wire the ESP32 pins to your MSX joystick port according to the pin mapping above.

4. **Upload Sketch:**  
    Open `BlueBuzz.ino` in Arduino IDE and upload to your ESP32 board.

## Usage

- **Connect Controllers:**  
  Pair your Bluetooth gamepad(s) with the ESP32. Up to two controllers are supported.

- **Play:**  
  Controller inputs are mapped to MSX joystick signals. Use the gamepad as you would a classic joystick.

- **Turbo Controls:**  
  - Hold **L Shoulder** or **L Trigger** to decrease/increase turbo speed for A button.
  - Hold **R Shoulder** or **R Trigger** to decrease/increase turbo speed for B button.
  - Hold **X** (A turbo) or **Y** (B turbo) to activate turbo fire.

- **Rumble Feedback:**  
  Turbo speed changes at limits trigger controller rumble.

- **Forget All Controllers:**  
  Press **SELECT + START** together to disconnect all controllers and reset Bluetooth keys.

## Customization

- **Pin Assignments:**  
  Modify the `PLAYER_PINS` array in the code to match your hardware setup.

- **Turbo Settings:**  
  Adjust initial turbo speeds and limits in the code (`turbo_a`, `turbo_b`, etc.).

## Troubleshooting

- **Controller Not Connecting:**  
  Ensure your controller is compatible with Bluepad32 and Bluetooth is enabled on the ESP32.

- **No Output on MSX:**  
  Check wiring and pin assignments. Confirm the ESP32 is powered and running the sketch.

- **Turbo Not Working:**  
  Verify turbo buttons are mapped correctly and turbo speed is within allowed range.

## License

This project is provided under the MIT License. See LICENSE file for details.

## Credits

- [Bluepad32](https://github.com/ricardoquesada/bluepad32) by Ricardo Quesada
- Inspired by MSX retro gaming community

---
Enjoy modern wireless controllers on your classic MSX hardware!
