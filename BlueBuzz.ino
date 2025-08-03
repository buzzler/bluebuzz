#include <Bluepad32.h>

enum PinIndex {
  PIN_UP,
  PIN_DOWN,
  PIN_LEFT,
  PIN_RIGHT,
  PIN_A,
  PIN_B,
  PIN_OUT
};

const int DELAY_JOYSTICK = 15;
const int DELAY_MOUSE = 0;
const int PLAYER_LIMIT = 1;
const int PLAYER_PINS[PLAYER_LIMIT][7] = {
  {23, 19, 18, 5, 22, 21, 32}
};
ControllerPtr playerList[BP32_MAX_GAMEPADS];

int delay_ms = DELAY_JOYSTICK;
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
char mouse_x = 0;
char mouse_y = 0;
long timer;

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
    if (ctl && ctl->isConnected() && ctl->isGamepad()) {
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
        if (playerList[i] == nullptr) {
            playerList[i] = ctl;
            controllerCount++;
            delay_ms = ctl->isMouse() ? DELAY_MOUSE : DELAY_JOYSTICK;
            setRumble(ctl);
            break;
        }
    }

    if (controllerCount >= PLAYER_LIMIT)
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
        if (playerList[i] == ctl) {
            playerList[i] = nullptr;
            controllerCount--;
            delay_ms = DELAY_JOYSTICK;
            break;
        }
    }

    if (before != controllerCount && controllerCount < PLAYER_LIMIT)
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
        playerList[i] = nullptr;
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
void processJoystick(ControllerPtr ctl, int index) {
    delay_ms = DELAY_JOYSTICK;
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
 * @brief Sends a byte to the MSX system by splitting it into high and low nibbles and setting the appropriate pins.
 *
 * This function waits for the OUT pin of PLAYER_1 to go HIGH, then sets the control pins (RIGHT, LEFT, DOWN, UP)
 * for the high nibble of the character 'c'. After the OUT pin goes LOW, it sets the control pins for the low nibble.
 * If the operation takes too long (based on a timer), the function returns early.
 *
 * @param c The byte to send, split into high and low nibbles.
 * @param index The player index to select the correct set of pins from PLAYER_PINS.
 */
void sendMSX(char c, int index) {
    // Wait for OUT pin to go HIGH
    while (digitalRead(PLAYER_PINS[index][PIN_OUT]) == LOW) {
        if (millis() > timer) return;
    }
    // Set pins for high nibble
    setPin(PLAYER_PINS[index][PIN_RIGHT], !(c & 0x80));
    setPin(PLAYER_PINS[index][PIN_LEFT], !(c & 0x40));
    setPin(PLAYER_PINS[index][PIN_DOWN], !(c & 0x20));
    setPin(PLAYER_PINS[index][PIN_UP], !(c & 0x10));

    // Wait for OUT pin to go LOW
    while (digitalRead(PLAYER_PINS[index][PIN_OUT]) == HIGH) {
        if (millis() > timer) return;
    }
    // Set pins for low nibble
    setPin(PLAYER_PINS[index][PIN_RIGHT], !(c & 0x08));
    setPin(PLAYER_PINS[index][PIN_LEFT], !(c & 0x04));
    setPin(PLAYER_PINS[index][PIN_DOWN], !(c & 0x02));
    setPin(PLAYER_PINS[index][PIN_UP], !(c & 0x01));
}

/**
 * @brief Processes mouse input from a controller and sends corresponding signals.
 *
 * This function reads the current state of the mouse from the given controller,
 * updates the mouse position, sets button states, and sends the appropriate signals
 * to the MSX system. It also handles timing to ensure proper communication and resets
 * mouse deltas after sending.
 *
 * @param ctl   Pointer to the controller object providing mouse input data.
 * @param index Index of the player/controller to process (used for pin selection).
 */
void processMouse(ControllerPtr ctl, int index) {
    delay_ms = DELAY_MOUSE;
    uint16_t buttons = ctl->buttons();
    mouse_x = mouse_x + ctl->deltaX();
    mouse_y = mouse_y + ctl->deltaY();

    setPin(PLAYER_PINS[index][PIN_A], buttons == UNI_MOUSE_BUTTON_LEFT);
    setPin(PLAYER_PINS[index][PIN_B], buttons == UNI_MOUSE_BUTTON_RIGHT);

    timer = millis() + 40;
    sendMSX(-mouse_x, index);
    timer = millis() + 3;
    sendMSX(-mouse_y, index);
    if( millis() < timer ) {
      mouse_x = 0;
      mouse_y = 0;
      timer = millis() + 2;
    } 
    while( digitalRead( PLAYER_PINS[index][PIN_OUT] ) == LOW ) {
      if( millis() > timer )
        break;
    }

    setPin(PLAYER_PINS[index][PIN_UP], false);
    setPin(PLAYER_PINS[index][PIN_DOWN], false);
    setPin(PLAYER_PINS[index][PIN_LEFT], false);
    setPin(PLAYER_PINS[index][PIN_RIGHT], false);
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

    for (int i = 0 ; i < PLAYER_LIMIT ; i++)
        for (int j = 0 ; j < 6 ; j++)
            setPin(PLAYER_PINS[i][j], false);
}

/**
 * @brief Main loop function for processing Bluetooth controllers.
 *
 * This function is repeatedly called by the Arduino runtime. It updates the BP32 Bluetooth stack,
 * processes input from connected controllers, and handles delays between iterations.
 *
 * - If BP32.update() fails, the function waits for a specified delay (if set) and returns early.
 * - Iterates through all possible controller slots (BP32_MAX_GAMEPADS):
 *     - For each connected controller with new data:
 *         - If the controller is a gamepad, calls processJoystick().
 *         - If the controller is a mouse, calls processMouse().
 * - At the end of each loop, waits for a specified delay (if set).
 */
void loop() {
    if (!BP32.update()) {
        if (delay_ms > 0)
            delay(delay_ms);
        return;
    }

    for (int i = 0 ; i < BP32_MAX_GAMEPADS ; i++) {
        auto player = playerList[i];
        if (player && player->isConnected() && player->hasData())
            if (player->isGamepad())
                processJoystick(player, i);
            else if (player->isMouse())
                processMouse(player, i);
    }

    if (delay_ms > 0)
        delay(delay_ms);
}
