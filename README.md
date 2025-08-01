README.md

# BlueBuzz

BlueBuzz is an Arduino-based project that enables Bluetooth gamepads and mice to interface with MSX computers via GPIO pins. It translates modern Bluetooth controller input into MSX-compatible joystick and mouse signals, supporting turbo fire, rumble feedback, and multiple controller profiles.

## Features

- **Bluetooth Gamepad Support:** Connects modern Bluetooth controllers to MSX hardware.
- **Mouse Emulation:** Supports Bluetooth mice for MSX mouse input.
- **Turbo Fire:** Adjustable turbo fire for buttons A and B, with speed control via shoulder/trigger buttons.
- **Rumble Feedback:** Provides haptic feedback when turbo limits are reached.
- **Multi-Controller Support:** Handles up to two controllers (configurable).
- **Automatic Pin Management:** Dynamically sets GPIO pins for MSX joystick/mouse protocol.
- **Bluetooth Key Management:** Easily forgets all paired devices via controller input.

## Hardware Requirements

- **Arduino Board:** ESP32-based board recommended (for Bluetooth support).
- **Wiring:** Connect Arduino GPIO pins to MSX joystick/mouse port according to the `PLAYER_PINS` mapping in the code.
- **Bluetooth Controllers:** Any compatible Bluetooth gamepad or mouse.

## Pin Mapping

The code uses the following pin mapping for two players:

| Player | UP | DOWN | LEFT | RIGHT | A | B | OUT |
|--------|----|------|------|-------|---|---|-----|
| 1      | 23 | 19   | 18   | 5     | 22| 21| 32  |
| 2      | 4  | 14   | 15   | 27    | 26| 25| 33  |

Adjust the `PLAYER_PINS` array in the code if your wiring differs.

## Software Dependencies

- [Bluepad32](https://github.com/ricardoquesada/Bluepad32): Bluetooth gamepad/mouse library for ESP32.

## Setup

1. **Install Bluepad32** on your Arduino IDE or PlatformIO environment.
2. **Wire the Arduino** to the MSX joystick/mouse port as per the pin mapping.
3. **Upload the `BlueBuzz.ino` sketch** to your ESP32 board.
4. **Power on your MSX** and Arduino.
5. **Pair your Bluetooth controller** (gamepad or mouse) with the Arduino. The device will rumble on successful connection.
6. **Use the controller** to send joystick or mouse input to the MSX.

## Usage

- **Turbo Fire:** Hold `X` (for A) or `Y` (for B) to enable turbo. Adjust speed with shoulder/trigger buttons.
- **Forget All Controllers:** Press both `SELECT` and `START` to clear all paired Bluetooth devices.
- **Mouse Mode:** Connect a Bluetooth mouse; movement and button clicks are sent to the MSX.

## Customization

- **Controller Limit:** Change `CONTROLLER_LIMIT` to allow more controllers.
- **Turbo Speed:** Adjust `turbo_a` and `turbo_b` defaults for different turbo rates.
- **Pin Mapping:** Modify `PLAYER_PINS` for your hardware setup.

## Troubleshooting

- **No Input:** Check wiring and ensure correct pin mapping.
- **Bluetooth Issues:** Use the forget function to clear and re-pair devices.
- **Lag/Delays:** Adjust `DELAY_JOYSTICK` and `DELAY_MOUSE` for optimal performance.

## License

This project is open source. See the LICENSE file for details.

## Credits

- Inspired by the MSX community and the Bluepad32 project.
- Developed by [Your Name or GitHub handle].