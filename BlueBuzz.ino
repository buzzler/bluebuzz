/**
 * @file BlueBuzz.ino
 * @brief Arduino sketch for interfacing Bluetooth gamepads with MSX-style digital inputs.
 *
 * This program uses the Bluepad32 library to connect Bluetooth controllers and map their inputs
 * to digital pins, emulating an MSX joystick interface. It supports turbo functionality for A/B buttons,
 * rumble feedback, and limits the number of connected controllers.
 *
 * Constants:
 *   - DELAY: Main loop delay in milliseconds.
 *   - PIN_UP, PIN_DOWN, PIN_LEFT, PIN_RIGHT, PIN_A, PIN_B: Pin assignments for MSX joystick signals.
 *   - CONTROLLER_LIMIT: Maximum number of controllers allowed to connect.
 *   - controllerList: Array holding pointers to connected controllers.
 *
 * State Variables:
 *   - controllerCount: Number of currently connected controllers.
 *   - press_*: State flags for each button/direction.
 *   - turbo_a, turbo_b: Turbo speed settings for A/B buttons.
 *   - turbo_a_counter, turbo_b_counter: Counters for turbo button toggling.
 *
 * Functions:
 *   - setPin(pin, pressed): Sets pin mode and output based on button state.
 *   - setRumble(ctl, duration, strengthLeft, strengthRight): Triggers controller rumble feedback.
 *   - onConnectedController(ctl): Handles new controller connections and enforces connection limit.
 *   - onDisconnectedController(ctl): Handles controller disconnections and re-enables connections if below limit.
 *   - processMSX(ctl, index): Reads controller input, maps to MSX signals, manages turbo and rumble.
 *   - setup(): Initializes Bluepad32, pins, and disables virtual devices.
 *   - loop(): Main loop, updates controller states and processes input.
 *
 * Usage:
 *   - Connect a Bluetooth controller.
 *   - Controller inputs are mapped to MSX joystick pins.
 *   - Turbo functionality can be adjusted using shoulder/trigger buttons.
 *   - Rumble feedback is provided on turbo adjustment.
 */
#include <Bluepad32.h>

enum PlayerIndex {
    PLAYER_1,
    PLAYER_2
};

enum PinIndex {
  PIN_UP,
  PIN_DOWN,
  PIN_LEFT,
  PIN_RIGHT,
  PIN_A,
  PIN_B
};

const int PLAYER_PINS[2][6] = {
  {23, 19, 18, 5, 22, 21},
  {4, 14, 15, 27, 26, 25}
};

const int DELAY = 15;
const int CONTROLLER_LIMIT = 2;
ControllerPtr controllerList[BP32_MAX_GAMEPADS];

int controllerCount = 0;
bool press_up = false;
bool press_down = false;
bool press_left = false;
bool press_right = false;
bool press_a = false;
bool press_b = false;
bool press_ls = false;
bool press_rs = false;
bool press_lt = false;
bool press_rt = false;
int turbo_a = 5;
int turbo_b = 5;
int turbo_a_counter = 0;
int turbo_b_counter = 0;

/**
 * @brief Sets the mode and state of a specified pin based on the pressed state.
 *
 * If pressed is true, the pin is configured as OUTPUT and set to LOW.
 * If pressed is false, the pin is configured as INPUT.
 *
 * @param pin The Arduino pin number to configure.
 * @param pressed Boolean indicating whether the pin should be pressed (true) or released (false).
 */
void setPin(int pin, bool pressed) {
    if (pressed) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    } else {
        pinMode(pin, INPUT);
    }
}

/**
 * @brief Activates the rumble feature on a controller for a specified duration and strength.
 *
 * This function triggers dual rumble motors on the provided controller, if connected.
 * The left and right rumble strengths, as well as the duration, can be customized.
 *
 * @param ctl Pointer to the controller object.
 * @param duration Duration of the rumble in milliseconds (default: 300).
 * @param strengthLeft Strength of the left rumble motor (default: 0x80).
 * @param strengthRight Strength of the right rumble motor (default: 0x40).
 */
void setRumble(ControllerPtr ctl, int duration = 300, uint8_t strengthLeft = 0x80, uint8_t strengthRight = 0x40) {
    if (ctl && ctl->isConnected()) {
        ctl->playDualRumble(0, duration, strengthLeft, strengthRight);
    }
}


/**
 * @brief Handles the event when a new controller is connected.
 *
 * Adds the connected controller to the controller list if there is space available.
 * Increments the controller count and sets the rumble feature for the new controller.
 * If the number of connected controllers reaches the defined limit, disables new Bluetooth connections.
 *
 * @param ctl Pointer to the newly connected controller.
 */
void onConnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (controllerList[i] == nullptr) {
            controllerList[i] = ctl;
            controllerCount++;
            setRumble(ctl);
            break;
        }
    }

    if (controllerCount >= CONTROLLER_LIMIT)
        BP32.enableNewBluetoothConnections(false);
}

/**
 * @brief Handles the disconnection of a Bluetooth controller.
 *
 * This function is called when a controller is disconnected. It removes the controller
 * from the controller list, decrements the controller count, and re-enables new Bluetooth
 * connections if the number of connected controllers falls below the defined limit.
 *
 * @param ctl Pointer to the disconnected controller.
 */
void onDisconnectedController(ControllerPtr ctl) {
    auto before = controllerCount;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (controllerList[i] == ctl) {
            controllerList[i] = nullptr;
            controllerCount--;
            break;
        }
    }

    if (before != controllerCount && controllerCount < CONTROLLER_LIMIT)
        BP32.enableNewBluetoothConnections(true);
}


/**
 * @brief Forgets all connected controllers and resets controller state.
 *
 * This function sets all entries in the controller list to nullptr,
 * resets the controller count to zero, clears stored Bluetooth keys,
 * and enables new Bluetooth connections.
 */
void onForgetAllControllers() {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++)
        controllerList[i] = nullptr;
    controllerCount = 0;
    BP32.forgetBluetoothKeys();
    BP32.enableNewBluetoothConnections(true);
}

/**
 * @brief Processes controller input and updates MSX-compatible output pins.
 *
 * This function reads the current state of the controller (dpad, axes, buttons)
 * and translates them into digital signals for MSX hardware. It handles turbo
 * functionality for buttons A and B, allowing rapid-fire when enabled, and
 * adjusts turbo speed using shoulder and trigger buttons. Rumble feedback is
 * triggered when turbo speed limits are reached.
 *
 * @param ctl Pointer to the controller interface.
 * @param index Index of the controller (unused in this function).
 */
void processMSX(ControllerPtr ctl, int index) {
    uint8_t dpad = ctl->dpad();
    int32_t axisX = ctl->axisX();
    int32_t axisY = ctl->axisY();
    uint16_t buttons = ctl->buttons();
    uint16_t misc = ctl->miscButtons();

    if (misc == MISC_BUTTON_SELECT && misc == MISC_BUTTON_START) {
        onForgetAllControllers();
        return;
    }

    bool up = dpad == DPAD_UP || axisY < -127;
    bool down = dpad == DPAD_DOWN || axisY > 127;
    bool left = dpad == DPAD_LEFT || axisX < -127;
    bool right = dpad == DPAD_RIGHT || axisX > 127;
    bool a = buttons == BUTTON_A;
    bool b = buttons == BUTTON_B;
    bool x = buttons == BUTTON_X;
    bool y = buttons == BUTTON_Y;
    bool ls = buttons == BUTTON_SHOULDER_L;
    bool rs = buttons == BUTTON_SHOULDER_R;
    bool lt = buttons == BUTTON_TRIGGER_L;
    bool rt = buttons == BUTTON_TRIGGER_R;

    if (up != press_up) {
        setPin(PLAYER_PINS[index][PIN_UP], up);
        press_up = up;
    }

    if (down != press_down) {
        setPin(PLAYER_PINS[index][PIN_DOWN], down);
        press_down = down;
    }
    
    if (left != press_left) {
        setPin(PLAYER_PINS[index][PIN_LEFT], left);
        press_left = left;
    }

    if (right != press_right) {
        setPin(PLAYER_PINS[index][PIN_RIGHT], right);
        press_right = right;
    }

    if (ls != press_ls) {
        if (press_ls) {
            turbo_a = turbo_a - 1;
            if (turbo_a < 1) {
                turbo_a = 1;
                setRumble(ctl);
            }
        }
        press_ls = ls;
    }

    if (lt != press_lt) {
        if (press_lt) {
            turbo_a = turbo_a + 1;
            if (turbo_a > 10) {
                turbo_a = 10;
                setRumble(ctl);
            }
        }
        press_lt = lt;
    }

    if (x && turbo_a > 1) {
        turbo_a_counter++;
        int interval = 11 - turbo_a;
        if (turbo_a_counter >= 11 - turbo_a) {
            a = !press_a;
            turbo_a_counter = 0;
        } else {
            a = press_a;
        }
    } else {
        turbo_a_counter = 0;
    }

    if (a != press_a) {
        setPin(PLAYER_PINS[index][PIN_A], a);
        press_a = a;
    }

    if (rs != press_rs) {
        if (press_rs) {
            turbo_b = turbo_b - 1;
            if (turbo_b < 0) {
                turbo_b = 0;
                setRumble(ctl);
            }
        }
        press_rs = rs;
    }

    if (rt != press_rt) {
        if (press_rt) {
            turbo_b = turbo_b + 1;
            if (turbo_b > 10) {
                turbo_b = 10;
                setRumble(ctl);
            }
        }
        press_rt = rt;
    }

    if (y && turbo_b > 1) {
        turbo_b_counter++;
        int interval = 11 - turbo_b;
        if (turbo_b_counter >= interval) {
            b = !press_b;
            turbo_b_counter = 0;
        } else {
            b = press_b;
        }
    } else {
        turbo_b_counter = 0;
    }

    if (b != press_b) {
        setPin(PLAYER_PINS[index][PIN_B], b);
        press_b = b;
    }
}

/**
 * @brief Initializes the BlueBuzz device and its peripherals.
 *
 * This function sets up the BP32 Bluetooth controller with connection and disconnection callbacks,
 * clears any stored Bluetooth keys, and disables the virtual device feature.
 * It also initializes all control pins (strobe, directional, and action buttons) to a low state (false).
 *
 * Called once at startup.
 */
void setup() {
    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.forgetBluetoothKeys();
    BP32.enableVirtualDevice(false);

    for (int i = 0 ; i < CONTROLLER_LIMIT ; i++)
        for (int j = 0 ; j < 6 ; j++)
            setPin(PLAYER_PINS[i][j], false);
}

/**
 * @brief Main loop function for Arduino sketch.
 *
 * Continuously updates the BP32 controller state and processes input data from connected gamepads.
 * If BP32 fails to update, the function waits for a predefined delay and returns early.
 * For each possible gamepad slot, checks if a controller is present, connected, and has new data,
 * then processes the input using processMSX().
 * After processing, waits for a predefined delay before the next iteration.
 */
void loop() {
    if (!BP32.update()) {
        delay(DELAY);
        return;
    }

    for (int i = 0 ; i < BP32_MAX_GAMEPADS ; i++) {
        auto player = controllerList[i];
        if (player && player->isConnected() && player->hasData())
            processMSX(player, i);
    }

    delay(DELAY);
}
