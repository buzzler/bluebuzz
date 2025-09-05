#include <WiFi.h>
#include <Bluepad32.h>
#include <esp_sleep.h>
#define INACTIVITY_TIMEOUT 120000

enum PinIndex {
  PIN_UP,
  PIN_DOWN,
  PIN_LEFT,
  PIN_RIGHT,
  PIN_A,
  PIN_B,
  PIN_OUT,
  PIN_LED
};

const int PLAYER_PINS[7] = {14, 26, 33, 32, 27, 25, 17, 18}; // ESP32 WROOM 32
//const int PLAYER_PINS[7] = {23, 19, 18, 5, 22, 21, 33, 32}; // TinyPICO
ControllerPtr player;

int delay_ms = 15;
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
char scale = 15;
uint16_t cur_buttons = 0;
int8_t cur_wheel = 0;
int32_t cur_deltaX, cur_deltaY = 0;
unsigned long timer;
unsigned long lastActivity = 0;

/**
 * @brief Sets the specified pin to either OUTPUT LOW or INPUT based on the pressed state.
 *
 * This function configures the given pin as an OUTPUT and sets it to LOW if the
 * pressed parameter is true. If pressed is false, the pin is configured as an INPUT.
 *
 * @param pin The pin number to configure.
 * @param pressed A boolean indicating whether the pin should be set to OUTPUT LOW (true) or INPUT (false).
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
 * @brief Triggers a rumble effect on the connected controller.
 *
 * This function activates the rumble feature of the connected controller for a specified duration
 * and strength. It checks if a controller is connected and is a gamepad before attempting to play
 * the rumble effect.
 *
 * @param duration The duration of the rumble effect in milliseconds (default is 300 ms).
 * @param strengthLeft The strength of the left rumble motor (default is 0x80).
 * @param strengthRight The strength of the right rumble motor (default is 0x40).
 */
void setRumble(int duration = 300, uint8_t strengthLeft = 0x80, uint8_t strengthRight = 0x40) {
    if (player && player->isConnected() && player->isGamepad()) {
        player->playDualRumble(0, duration, strengthLeft, strengthRight);
    }
}


/**
 * @brief Handles the connection of a Bluetooth controller.
 *
 * This function is called when a new controller is connected. It assigns the
 * connected controller to the player variable if it is currently null, disables
 * new Bluetooth connections, and triggers a rumble effect to indicate successful connection.
 *
 * @param ctl Pointer to the connected controller.
 */
void onConnectedController(ControllerPtr ctl) {
    if (player == nullptr) {
        player = ctl;
        Serial.printf("[BLUEBUZZ] connected\n");
        BP32.enableNewBluetoothConnections(false);
        setRumble();
    }
}

/**
 * @brief Handles the disconnection of a Bluetooth controller.
 *
 * This function is called when a controller is disconnected. If the disconnected
 * controller is the current player, it sets the player variable to null and
 * enables new Bluetooth connections.
 *
 * @param ctl Pointer to the disconnected controller.
 */
void onDisconnectedController(ControllerPtr ctl) {
    if (player == ctl) {
        player = nullptr;
        Serial.printf("[BLUEBUZZ] disconnected\n");
        BP32.enableNewBluetoothConnections(true);
    }
}


/**
 * @brief Forgets all paired Bluetooth controllers and resets the player state.
 *
 * This function clears the current player controller, forgets all stored Bluetooth keys,
 * and enables new Bluetooth connections to allow pairing with new controllers.
 */
void onForgetAllControllers() {
    player = nullptr;
    BP32.forgetBluetoothKeys();
    BP32.enableNewBluetoothConnections(true);
}

/**
 * @brief Processes joystick input from a connected gamepad controller.
 *
 * This function reads the current state of the joystick from the given controller,
 * updates the corresponding MSX output pins based on button presses and joystick movements,
 * and manages the state of directional and action buttons. It also handles turbo functionality
 * for action buttons A and B.
 */
void processJoystick() {
    uint8_t dpad = player->dpad();
    int32_t axisX = player->axisX();
    int32_t axisY = player->axisY();
    uint16_t buttons = player->buttons();
    uint16_t misc = player->miscButtons();

    if ((misc & MISC_BUTTON_SELECT) && (misc & MISC_BUTTON_START)) {
        onForgetAllControllers();
        return;
    }

    bool up = (dpad & DPAD_UP) || axisY < -127;
    bool down = (dpad & DPAD_DOWN) || axisY > 127;
    bool left = (dpad & DPAD_LEFT) || axisX < -127;
    bool right = (dpad & DPAD_RIGHT) || axisX > 127;
    bool a = buttons & BUTTON_A;
    bool b = buttons & BUTTON_B;
    bool x = buttons & BUTTON_X;
    bool y = buttons & BUTTON_Y;
    bool ls = buttons & BUTTON_SHOULDER_L;
    bool rs = buttons & BUTTON_SHOULDER_R;
    bool lt = buttons & BUTTON_TRIGGER_L;
    bool rt = buttons & BUTTON_TRIGGER_R;

    if (up != press_up) {
        setPin(PLAYER_PINS[PIN_UP], up);
        press_up = up;
    }

    if (down != press_down) {
        setPin(PLAYER_PINS[PIN_DOWN], down);
        press_down = down;
    }
    
    if (left != press_left) {
        setPin(PLAYER_PINS[PIN_LEFT], left);
        press_left = left;
    }

    if (right != press_right) {
        setPin(PLAYER_PINS[PIN_RIGHT], right);
        press_right = right;
    }

    if (ls != press_ls) {
        if (press_ls) {
            turbo_a = turbo_a - 1;
            if (turbo_a < 1) {
                turbo_a = 1;
                setRumble();
            }
        }
        press_ls = ls;
    }

    if (lt != press_lt) {
        if (press_lt) {
            turbo_a = turbo_a + 1;
            if (turbo_a > 10) {
                turbo_a = 10;
                setRumble();
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
        setPin(PLAYER_PINS[PIN_A], a);
        press_a = a;
    }

    if (rs != press_rs) {
        if (press_rs) {
            turbo_b = turbo_b - 1;
            if (turbo_b < 0) {
                turbo_b = 0;
                setRumble();
            }
        }
        press_rs = rs;
    }

    if (rt != press_rt) {
        if (press_rt) {
            turbo_b = turbo_b + 1;
            if (turbo_b > 10) {
                turbo_b = 10;
                setRumble();
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
        setPin(PLAYER_PINS[PIN_B], b);
        press_b = b;
    }
}

/**
 * @brief Sends a character to the MSX system using the defined pin protocol.
 *
 * This function waits for the OUT pin to go HIGH, then sets the directional pins
 * according to the high nibble of the character. It then waits for the OUT pin to
 * go LOW before setting the directional pins according to the low nibble of the character.
 *
 * @param c The character to send to the MSX system.
 */
void sendMSX(char c) {
    // Wait for OUT pin to go HIGH
    while (digitalRead(PLAYER_PINS[PIN_OUT]) == LOW) {
        if (millis() > timer) return;
    }
    // Set pins for high nibble
    setPin(PLAYER_PINS[PIN_RIGHT], !(c & 0x80));
    setPin(PLAYER_PINS[PIN_LEFT], !(c & 0x40));
    setPin(PLAYER_PINS[PIN_DOWN], !(c & 0x20));
    setPin(PLAYER_PINS[PIN_UP], !(c & 0x10));

    // Wait for OUT pin to go LOW
    while (digitalRead(PLAYER_PINS[PIN_OUT]) == HIGH) {
        if (millis() > timer) return;
    }
    // Set pins for low nibble
    setPin(PLAYER_PINS[PIN_RIGHT], !(c & 0x08));
    setPin(PLAYER_PINS[PIN_LEFT], !(c & 0x04));
    setPin(PLAYER_PINS[PIN_DOWN], !(c & 0x02));
    setPin(PLAYER_PINS[PIN_UP], !(c & 0x01));
}

/**
 * @brief Processes mouse input from a connected mouse controller.
 *
 * This function reads the current state of the mouse from the given controller,
 * updates the corresponding MSX output pins based on button presses and mouse movements,
 * and manages the state of the mouse buttons. It also handles scaling of mouse movement
 * and ensures proper timing for sending data to the MSX system.
 *
 * @param buttons The current state of mouse buttons.
 * @param wheel The scroll wheel movement.
 * @param deltaX The change in X position.
 * @param deltaY The change in Y position.
 */
void processMouse(uint16_t buttons, int8_t wheel, int32_t deltaX, int32_t deltaY) {
    mouse_x = mouse_x - deltaX;
    mouse_y = mouse_y + deltaY;
    char x = mouse_x * scale / 20;
    char y = mouse_y * scale / 20;
    bool bA = buttons & UNI_MOUSE_BUTTON_LEFT;
    bool bB = buttons & UNI_MOUSE_BUTTON_RIGHT;
    bool bC = buttons & UNI_MOUSE_BUTTON_MIDDLE;

    Serial.printf("[BLUEBUZZ] X: %d, Y: %d, L: %d, R: %d, M: %d, W: %d\n", x, y, bA, bB, bC, wheel);

    setPin(PLAYER_PINS[PIN_A], bA);
    setPin(PLAYER_PINS[PIN_B], bB);
    timer = millis() + 40;
    sendMSX(x);
    sendMSX(y);
    
    if( millis() < timer ) {
      mouse_x = 0;
      mouse_y = 0;
      timer = millis() + 2;
    } 
    while( digitalRead( PLAYER_PINS[PIN_OUT] ) == LOW ) {
      if( millis() > timer )
        break;
    }

    setPin(PLAYER_PINS[PIN_UP], false);
    setPin(PLAYER_PINS[PIN_DOWN], false);
    setPin(PLAYER_PINS[PIN_LEFT], false);
    setPin(PLAYER_PINS[PIN_RIGHT], false);
}

/**
 * @brief Processes keyboard input from a connected keyboard controller.
 *
 * This function reads the current state of the keyboard from the given controller,
 * updates the corresponding MSX output pins based on key presses, and manages the
 * state of directional and action buttons.
 */
void processKeyboard() {
    bool up = player->isKeyPressed(Keyboard_UpArrow) || player->isKeyPressed(Keyboard_W);
    bool down = player->isKeyPressed(Keyboard_DownArrow) || player->isKeyPressed(Keyboard_S);
    bool left = player->isKeyPressed(Keyboard_LeftArrow) || player->isKeyPressed(Keyboard_A);
    bool right = player->isKeyPressed(Keyboard_RightArrow) || player->isKeyPressed(Keyboard_D);
    bool a = player->isKeyPressed(Keyboard_N) || player->isKeyPressed(Keyboard_Spacebar) || player->isKeyPressed(Keyboard_Enter);
    bool b = player->isKeyPressed(Keyboard_M) || player->isKeyPressed(Keyboard_Escape);

    if (up != press_up) {
        setPin(PLAYER_PINS[PIN_UP], up);
        press_up = up;
    }

    if (down != press_down) {
        setPin(PLAYER_PINS[PIN_DOWN], down);
        press_down = down;
    }
    
    if (left != press_left) {
        setPin(PLAYER_PINS[PIN_LEFT], left);
        press_left = left;
    }

    if (right != press_right) {
        setPin(PLAYER_PINS[PIN_RIGHT], right);
        press_right = right;
    }

    if (a != press_a) {
        setPin(PLAYER_PINS[PIN_A], a);
        press_a = a;
    }

    if (b != press_b) {
        setPin(PLAYER_PINS[PIN_B], b);
        press_b = b;
    }
}

/**
 * @brief Initializes the BlueBuzz system and configures pins.
 *
 * This function is called once at the start of the program. It initializes
 * serial communication, configures WiFi settings, sets CPU frequency,
 * initializes the BP32 Bluetooth stack, and configures the player pins.
 */
void setup() {
    Serial.begin(115200);
    Serial.println("[BLUEBUZZ] READY!");

    WiFi.mode(WIFI_OFF);
    setCpuFrequencyMhz(80);

    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.forgetBluetoothKeys();
    BP32.enableVirtualDevice(false);

    for (int i = 0 ; i < 7 ; i++)
        setPin(PLAYER_PINS[i], false);
    pinMode(PLAYER_PINS[PIN_LED], OUTPUT);
}

/**
 * @brief Main loop that handles controller input and updates MSX output pins.
 *
 * This function runs continuously after setup. It checks for connected controllers,
 * processes input from the connected controller (mouse, joystick, or keyboard),
 * and updates the MSX output pins accordingly. It also manages inactivity timeout
 * and puts the ESP32 into light sleep mode when inactive.
 */
void loop() {
    if (!player || !player->isConnected()) {
        digitalWrite(PLAYER_PINS[PIN_LED], HIGH);
        delay(200);
        digitalWrite(PLAYER_PINS[PIN_LED], LOW);
        delay(200);
        return;
    } else {
        digitalWrite(PLAYER_PINS[PIN_LED], HIGH);
    }

    if (player && player->isMouse() && player->isConnected()) {
        if (BP32.update()) {
            cur_buttons = player->buttons();
            cur_wheel = player->scrollWheel();
            cur_deltaX = player->deltaX();
            cur_deltaY = player->deltaY();
        } else {
            cur_wheel = 0;
            cur_deltaX = 0;
            cur_deltaY = 0;
        }
        processMouse(cur_buttons, cur_wheel, cur_deltaX, cur_deltaY);
        return;
    }

    if (!BP32.update()) {
        delay(delay_ms);
        return;
    }

    if (player && player->isConnected()) {
        if (player->hasData()) {
            if (player->isGamepad())
                processJoystick();
            else if (player->isKeyboard())
                processKeyboard();
            lastActivity = millis();
        } else if (millis() - lastActivity > INACTIVITY_TIMEOUT) {
            esp_sleep_enable_timer_wakeup(100000);
            esp_light_sleep_start();
        }
    }
    delay(delay_ms);
}
