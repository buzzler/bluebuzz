# BlueBuzz for MSX (ESP32 TinyPICO Edition)

BlueBuzz is an Arduino-based project that enables wireless gamepad and mouse input for MSX computers using Bluetooth controllers. This implementation is designed for the ESP32 TinyPICO development board and leverages the [Bluepad32](https://github.com/ricardoquesada/bluepad32) library for Bluetooth HID support.

## Features

- **Bluetooth Gamepad Support:** Connect modern Bluetooth controllers (e.g., Xbox, PlayStation, 8BitDo) and use them as MSX joysticks.
- **Bluetooth Mouse Support:** Use Bluetooth mice for MSX-compatible mouse input.
- **Turbo Functionality:** Adjustable turbo (rapid-fire) for buttons A and B, controlled via shoulder and trigger buttons.
- **Rumble Feedback:** Controller vibration feedback when turbo limits are reached.
- **Multi-Controller Support:** Easily scalable for multiple players (currently set to 1 for MSX).
- **MSX-Compatible Pin Output:** Maps controller input to MSX joystick/mouse pins for seamless integration.
- **Bluetooth Key Management:** Automatic forgetting of Bluetooth keys for easy pairing/reset.

## Hardware Requirements

- **ESP32 TinyPICO** development board
- **MSX computer** (with joystick/mouse port)
- **Level shifter** (recommended for safe voltage interfacing between ESP32 and MSX)
- **Wiring** to connect ESP32 GPIOs to MSX joystick/mouse port

## Pin Mapping

The following ESP32 GPIOs are mapped to MSX joystick/mouse signals:

| MSX Signal | ESP32 GPIO | Description      |
|------------|------------|------------------|
| UP         | 23         | Directional Up   |
| DOWN       | 19         | Directional Down |
| LEFT       | 18         | Directional Left |
| RIGHT      | 5          | Directional Right|
| A          | 22         | Button A         |
| B          | 21         | Button B         |
| OUT        | 32         | Strobe/OUT pin   |

> **Note:** You can adjust the `PLAYER_PINS` array in the code to match your wiring.

## Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software)
- [Bluepad32 library](https://github.com/ricardoquesada/bluepad32)
- ESP32 board support for Arduino

## Installation & Setup

1. **Clone or Download the Project:**
    - Place `BlueBuzz.ino` in your Arduino sketch directory.

2. **Install Bluepad32:**
    - Follow instructions on the [Bluepad32 GitHub](https://github.com/ricardoquesada/bluepad32) to install the library and its dependencies.

3. **Configure Board:**
    - In Arduino IDE, select `TinyPICO` as your board (Tools > Board > ESP32 Arduino > TinyPICO).

4. **Connect Hardware:**
    - Wire ESP32 GPIOs to the MSX joystick/mouse port according to the pin mapping above.
    - Use a level shifter if necessary.

5. **Upload Firmware:**
    - Compile and upload `BlueBuzz.ino` to your TinyPICO.

6. **Pair Bluetooth Controller:**
    - Put your controller in pairing mode.
    - The ESP32 will automatically pair and manage connections.

## Usage

- **Gamepad Mode:** Use the D-pad and buttons as you would on a standard MSX joystick.
- **Turbo:** Hold X (for A) or Y (for B) to enable turbo. Adjust turbo speed with shoulder/trigger buttons.
- **Mouse Mode:** Move the mouse and click buttons for MSX mouse input.
- **Forget Controllers:** Press SELECT + START simultaneously to forget all paired controllers and reset Bluetooth keys.

## Customization

- **Player Limit:** Change `PLAYER_LIMIT` to support more controllers.
- **Pin Mapping:** Edit `PLAYER_PINS` for different wiring.
- **Turbo Settings:** Adjust `turbo_a` and `turbo_b` for default turbo speed.

## Troubleshooting

- **No Input on MSX:** Check wiring and pin mapping. Ensure level shifting if needed.
- **Bluetooth Pairing Issues:** Use the forget controllers feature or reset the ESP32.
- **Laggy Input:** Adjust `DELAY_JOYSTICK` and `DELAY_MOUSE` for optimal performance.

## License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0).

## Credits

- [Bluepad32](https://github.com/ricardoquesada/bluepad32) by Ricardo Quesada
- ESP32 TinyPICO by Unexpected Maker

---

Enjoy wireless gaming on your MSX with modern controllers!