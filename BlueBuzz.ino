#include <WiFi.h>
#include <Bluepad32.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#pragma region Utility

#define DELAY_MS 15
#define MOUSE_SCALE 15
#define PIN_UP_GPIO 14
#define PIN_DOWN_GPIO 26
#define PIN_LEFT_GPIO 33
#define PIN_RIGHT_GPIO 32
#define PIN_A_GPIO 27
#define PIN_B_GPIO 25
#define PIN_OUT_GPIO 17
#define PIN_LED_GPIO 18

/**
 * @struct ButtonState
 * @brief Represents the state of various buttons on a controller.
 *
 * Each member variable corresponds to a specific button and indicates
 * whether that button is currently pressed (true) or not (false).
 *
 * Members:
 * - up:    State of the "up" button.
 * - down:  State of the "down" button.
 * - left:  State of the "left" button.
 * - right: State of the "right" button.
 * - a:     State of the "A" button.
 * - b:     State of the "B" button.
 * - ls:    State of the left stick button.
 * - rs:    State of the right stick button.
 * - lt:    State of the left trigger button.
 * - rt:    State of the right trigger button.
 */
struct ButtonState {
  bool up = false;
  bool down = false;
  bool left = false;
  bool right = false;
  bool a = false;
  bool b = false;
  bool ls = false;
  bool rs = false;
  bool lt = false;
  bool rt = false;
};

/**
 * @struct TurboState
 * @brief Represents the state of turbo functionality with counters and values.
 *
 * This structure holds two integer values (`a` and `b`) and their corresponding
 * counters (`a_counter` and `b_counter`). It can be used to track and manage
 * the state of two separate turbo-related variables and their activity counts.
 *
 * Members:
 *   int a         - Value for turbo state A (default: 5)
 *   int b         - Value for turbo state B (default: 5)
 *   int a_counter - Counter for turbo state A activations (default: 0)
 *   int b_counter - Counter for turbo state B activations (default: 0)
 */
struct TurboState {
  int a = 5;
  int b = 5;
  int a_counter = 0;
  int b_counter = 0;
};

/**
 * @struct MouseState
 * @brief Represents the current state of a mouse device.
 *
 * This structure holds information about the mouse's position, button states,
 * scroll wheel movement, and accumulated movement deltas.
 *
 * Members:
 *   x       - The current X position offset (relative, signed 8-bit).
 *   y       - The current Y position offset (relative, signed 8-bit).
 *   buttons - Bitmask representing the state of mouse buttons (16 bits).
 *   wheel   - The scroll wheel movement (signed 8-bit).
 *   deltaX  - Accumulated X movement (signed 32-bit).
 *   deltaY  - Accumulated Y movement (signed 32-bit).
 */
struct MouseState {
  char x = 0;
  char y = 0;
  uint16_t buttons = 0;
  int8_t wheel = 0;
  int32_t deltaX = 0;
  int32_t deltaY = 0;
};

ControllerPtr player;
ButtonState press;
TurboState turbo;
MouseState mouse;
AsyncWebServer server(80);

/**
 * @brief Sets the state of a specified pin based on the pressed parameter.
 * 
 * @param pin The number of the pin to set.
 * @param pressed If true, sets the pin to HIGH; if false, sets the pin to LOW.
 */
void setPin(int pin, bool pressed);

/**
 * @brief Activates the rumble (vibration) feature for a specified duration and strength.
 *
 * @param duration Duration of the rumble in milliseconds. Default is 300 ms.
 * @param strengthLeft Strength of the left motor (0x00 to 0xFF). Default is 0x80.
 * @param strengthRight Strength of the right motor (0x00 to 0xFF). Default is 0x40.
 */
void setRumble(int duration = 300, uint8_t strengthLeft = 0x80, uint8_t strengthRight = 0x40);

/**
 * @brief Sends a single data byte to the MSX device at a specified time.
 *
 * This function transmits the given data byte to the MSX system, using the provided timer value
 * to schedule or synchronize the transmission as needed.
 *
 * @param dataByte The data byte to be sent to the MSX device.
 * @param timer The timer value (in microseconds or milliseconds, depending on implementation)
 *              used to control the timing of the data transmission.
 */
void sendMSXDataByte(char dataByte, unsigned long timer);

/**
 * @brief Callback function invoked when the OTA (Over-The-Air) update process starts.
 *
 * This function is typically used to perform any initialization or setup required
 * before the OTA update begins, such as notifying the user or preparing resources.
 */
void onOTAStart();

/**
 * @brief Callback function to report OTA (Over-The-Air) update progress.
 *
 * This function is called periodically during an OTA update to provide
 * progress information.
 *
 * @param current The current number of bytes that have been transferred.
 * @param final The total number of bytes to be transferred.
 */
void onOTAProgress(size_t current, size_t final);

/**
 * @brief Callback function called when an OTA (Over-The-Air) update ends.
 * 
 * @param success Indicates whether the OTA update was successful (true) or failed (false).
 */
void onOTAEnd(bool success);

/**
 * @brief Sets the status of the LED.
 * 
 * @param status If true, turns the LED on; if false, turns it off.
 */
void setLED(bool status);

#pragma endregion

#pragma region StateMachine

enum State {
  STATE_INIT = 0,
  STATE_PAIRING,
  STATE_CONNECTED,
  STATE_GAMEPAD,
  STATE_KEYBOARD,
  STATE_MOUSE,
  STATE_OTA,
  STATE_ERROR,
  STATE_NULL,
  STATE_COUNT
};

State currentState = STATE_NULL;

/**
 * @brief Handles actions to perform when entering the initialization state.
 *
 * This function is called when the system transitions into the initialization
 * state. It is responsible for setting up any necessary resources or
 * configurations required before normal operation can begin.
 */
void onEnterInit();

/**
 * @brief Initializes resources or states required at the beginning of each loop iteration.
 *
 * This function should be called at the start of the main loop to perform any necessary
 * setup or reinitialization tasks that need to occur before the main loop logic executes.
 */
void onLoopInit();

/**
 * @brief Handles cleanup or transition tasks when exiting the initialization state.
 *
 * This function should be called when the system is leaving the initialization phase.
 * Implement any necessary resource deallocation, state resets, or preparation for the next state here.
 */
void onExitInit();

/**
 * @brief Handles actions to be performed when entering pairing mode.
 *
 * This function is called when the device transitions into Bluetooth pairing mode.
 * It should initialize any necessary resources or update the device state to allow pairing.
 */
void onEnterPairing();

/**
 * @brief Handles the pairing logic during the main loop execution.
 *
 * This function is intended to be called within the main loop to manage
 * Bluetooth pairing operations. It checks for pairing requests and processes
 * them accordingly.
 */
void onLoopPairing();

/**
 * @brief Handles the actions to be performed when exiting pairing mode.
 *
 * This function is called when the device transitions out of pairing mode.
 * It should perform any necessary cleanup or state changes required after pairing.
 */
void onExitPairing();

/**
 * @brief Handles actions to perform when the device enters the connected state.
 *
 * This function is called when a successful connection is established.
 * Implement any initialization or state changes required upon connection here.
 */
void onEnterConnected();

/**
 * @brief Callback function that is called when a loop connection is established.
 *
 * This function is intended to handle any initialization or actions required
 * when the loop connection event occurs.
 */
void onLoopConnected();

/**
 * @brief Handles actions to perform when exiting the connected state.
 *
 * This function is called when the device transitions out of the connected state.
 * Implement any necessary cleanup or state management tasks here.
 */
void onExitConnected();

/**
 * @brief Handles the transition or initialization required when entering Gamepad mode.
 *
 * This function is called to perform any setup or state changes necessary
 * when the system switches to Gamepad mode. It may configure hardware,
 * update internal states, or notify other components of the mode change.
 */
void onEnterGamepadMode();

/**
 * @brief Handles the main loop logic when the device is operating in Gamepad mode.
 *
 * This function should be called repeatedly in the main loop to process
 * gamepad-related input, output, and communication tasks.
 *
 * @note Ensure that the device is properly initialized in Gamepad mode before calling this function.
 */
void onLoopGamepadMode();

/**
 * @brief Handles the actions required when exiting the gamepad mode.
 *
 * This function should be called to perform any necessary cleanup or state changes
 * when the device transitions out of gamepad mode.
 */
void onExitGamepadMode();

/**
 * @brief Handles the transition or initialization required when entering Keyboard mode.
 */
void onEnterKeyboardMode();

/**
 * @brief Handles the main loop logic when the device is operating in Keyboard mode.
 */
void onLoopKeyboardMode();

/**
 * @brief Handles the actions required when exiting the keyboard mode.
 */
void onExitKeyboardMode();

/**
 * @brief Handles the transition or initialization required when entering Mouse mode.
 */
void onEnterMouseMode();

/**
 * @brief Handles the main loop logic when the device is operating in Mouse mode.
 */
void onLoopMouseMode();

/**
 * @brief Handles the actions required when exiting the mouse mode.
 */
void onExitMouseMode();

/**
 * @brief Handles the transition or initialization required when entering OTA mode.
 */
void onEnterOTA();

/**
 * @brief Handles the main loop logic when the device is operating in OTA mode.
 */
void onLoopOTA();

/**
 * @brief Handles the actions required when exiting the OTA mode.
 */
void onExitOTA();

/**
 * @brief Handles the transition or initialization required when entering Error mode.
 */
void onEnterError();

/**
 * @brief Handles the main loop logic when the device is operating in Error mode.
 */
void onLoopError();

/**
 * @brief Handles the actions required when exiting the Error mode.
 */
void onExitError();

/**
 * @brief Changes the current state of the state machine to a new state.
 *
 * This function handles the transition between states by calling the appropriate
 * exit and entry functions for the current and new states, respectively.
 *
 * @param newState The new state to transition to.
 */
void changeState(State newState) {
  if (newState == currentState) return;

  switch (currentState) {
    case STATE_INIT: onExitInit(); break;
    case STATE_PAIRING: onExitPairing(); break;
    case STATE_CONNECTED: onExitConnected(); break;
    case STATE_GAMEPAD: onExitGamepadMode(); break;
    case STATE_KEYBOARD: onExitKeyboardMode(); break;
    case STATE_MOUSE: onExitMouseMode(); break;
    case STATE_OTA: onExitOTA(); break;
    case STATE_ERROR: onExitError(); break;
    default: break;
  }

  currentState = newState;

  switch (currentState) {
    case STATE_INIT: onEnterInit(); break;
    case STATE_PAIRING: onEnterPairing(); break;
    case STATE_CONNECTED: onEnterConnected(); break;
    case STATE_GAMEPAD: onEnterGamepadMode(); break;
    case STATE_KEYBOARD: onEnterKeyboardMode(); break;
    case STATE_MOUSE: onEnterMouseMode(); break;
    case STATE_OTA: onEnterOTA(); break;
    case STATE_ERROR: onEnterError(); break;
    default: break;
  }
}

/**
 * @brief The main setup function for initializing the system.
 */
void setup() {
  Serial.begin(115200);
  while (!Serial.availableForWrite())
    delay(100);
  changeState(STATE_INIT);
}

/**
 * @brief The main loop function that runs continuously after setup.
 *
 * This function checks the current state and calls the corresponding loop
 * handler function for that state.
 */
void loop() {
  switch (currentState) {
    case STATE_INIT: onLoopInit(); break;
    case STATE_PAIRING: onLoopPairing(); break;
    case STATE_CONNECTED: onLoopConnected(); break;
    case STATE_GAMEPAD: onLoopGamepadMode(); break;
    case STATE_KEYBOARD: onLoopKeyboardMode(); break;
    case STATE_MOUSE: onLoopMouseMode(); break;
    case STATE_OTA: onLoopOTA(); break;
    case STATE_ERROR: onLoopError(); break;
    default: break;
  }
}

#pragma endregion

#pragma region StateMachine Implementation

// INIT
void onEnterInit() {
  Serial.println("Enter INIT");

  WiFi.mode(WIFI_OFF);
  setCpuFrequencyMhz(80);

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();
  BP32.enableVirtualDevice(false);

  setPin(PIN_UP_GPIO, false);
  setPin(PIN_DOWN_GPIO, false);
  setPin(PIN_LEFT_GPIO, false);
  setPin(PIN_RIGHT_GPIO, false);
  setPin(PIN_A_GPIO, false);
  setPin(PIN_B_GPIO, false);
  setPin(PIN_OUT_GPIO, false);
  pinMode(PIN_LED_GPIO, OUTPUT);
}
void onLoopInit() {
  changeState(STATE_PAIRING);
}
void onExitInit() {
  Serial.println("Exit INIT");
}

// PAIRING
void onEnterPairing() {
  Serial.println("Enter PAIRING");
  if (player && player->isConnected())
    player->disconnect();
  player = nullptr;
  BP32.forgetBluetoothKeys();
  BP32.enableNewBluetoothConnections(true);
}
void onLoopPairing() {
  BP32.update();
  setLED(true);
  delay(100);
  setLED(false);
  delay(100);
}
void onExitPairing() {
  Serial.println("Exit PAIRING");
  BP32.enableNewBluetoothConnections(false);
}

// CONNECTED
void onEnterConnected() {
  Serial.println("Enter CONNECTED");
}
void onLoopConnected() {
  setLED(true);

  if (!BP32.update()) {
    delay(DELAY_MS);
    return;
  }

  if (player && player->isConnected()) {
    if (player->hasData()) {
      if (player->isGamepad())
        changeState(STATE_GAMEPAD);
      else if (player->isKeyboard())
        changeState(STATE_KEYBOARD);
      else if (player->isMouse())
        changeState(STATE_MOUSE);
    }
  }
}
void onExitConnected() {
  Serial.println("Exit CONNECTED");
}

// GAMEPAD
void onEnterGamepadMode() {
  Serial.println("Enter GAMEPAD");
}
void onLoopGamepadMode() {
  setLED(true);

  if (!BP32.update()) {
    delay(DELAY_MS);
    return;
  }

  if (!(player->hasData()))
    return;

  uint8_t dpad = player->dpad();
  int32_t axisX = player->axisX();
  int32_t axisY = player->axisY();
  uint16_t buttons = player->buttons();
  uint16_t misc = player->miscButtons();

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

  if ((misc & MISC_BUTTON_SELECT) && (misc & MISC_BUTTON_START)) {
    if (up)
      changeState(STATE_PAIRING);
    else if (down)
      changeState(STATE_OTA);
  }

  if (up != press.up) {
    setPin(PIN_UP_GPIO, up);
    press.up = up;
  }

  if (down != press.down) {
    setPin(PIN_DOWN_GPIO, down);
    press.down = down;
  }

  if (left != press.left) {
    setPin(PIN_LEFT_GPIO, left);
    press.left = left;
  }

  if (right != press.right) {
    setPin(PIN_RIGHT_GPIO, right);
    press.right = right;
  }

  if (ls != press.ls) {
    if (press.ls) {
      turbo.a = turbo.a - 1;
      if (turbo.a < 1) {
        turbo.a = 1;
        setRumble();
      }
    }
    press.ls = ls;
  }

  if (lt != press.lt) {
    if (press.lt) {
      turbo.a = turbo.a + 1;
      if (turbo.a > 10) {
        turbo.a = 10;
        setRumble();
      }
    }
    press.lt = lt;
  }

  if (x && turbo.a > 1) {
    turbo.a_counter++;
    int interval = 11 - turbo.a;
    if (turbo.a_counter >= 11 - turbo.a) {
      a = !press.a;
      turbo.a_counter = 0;
    } else {
      a = press.a;
    }
  } else {
    turbo.a_counter = 0;
  }

  if (a != press.a) {
    setPin(PIN_A_GPIO, a);
    press.a = a;
  }

  if (rs != press.rs) {
    if (press.rs) {
      turbo.b = turbo.b - 1;
      if (turbo.b < 0) {
        turbo.b = 0;
        setRumble();
      }
    }
    press.rs = rs;
  }

  if (rt != press.rt) {
    if (press.rt) {
      turbo.b = turbo.b + 1;
      if (turbo.b > 10) {
        turbo.b = 10;
        setRumble();
      }
    }
    press.rt = rt;
  }

  if (y && turbo.b > 1) {
    turbo.b_counter++;
    int interval = 11 - turbo.b;
    if (turbo.b_counter >= interval) {
      b = !press.b;
      turbo.b_counter = 0;
    } else {
      b = press.b;
    }
  } else {
    turbo.b_counter = 0;
  }

  if (b != press.b) {
    setPin(PIN_B_GPIO, b);
    press.b = b;
  }
}
void onExitGamepadMode() {
  Serial.println("Exit GAMEPAD");
}

// KEYBOARD
void onEnterKeyboardMode() {
  Serial.println("Enter KEYBOARD");
}
void onLoopKeyboardMode() {
  setLED(true);

  if (!BP32.update()) {
    delay(DELAY_MS);
    return;
  }

  if (!(player->hasData()))
    return;

  bool up = player->isKeyPressed(Keyboard_UpArrow) || player->isKeyPressed(Keyboard_W);
  bool down = player->isKeyPressed(Keyboard_DownArrow) || player->isKeyPressed(Keyboard_S);
  bool left = player->isKeyPressed(Keyboard_LeftArrow) || player->isKeyPressed(Keyboard_A);
  bool right = player->isKeyPressed(Keyboard_RightArrow) || player->isKeyPressed(Keyboard_D);
  bool a = player->isKeyPressed(Keyboard_N) || player->isKeyPressed(Keyboard_Spacebar) || player->isKeyPressed(Keyboard_Enter);
  bool b = player->isKeyPressed(Keyboard_M) || player->isKeyPressed(Keyboard_Escape);

  if (up != press.up) {
    setPin(PIN_UP_GPIO, up);
    press.up = up;
  }

  if (down != press.down) {
    setPin(PIN_DOWN_GPIO, down);
    press.down = down;
  }

  if (left != press.left) {
    setPin(PIN_LEFT_GPIO, left);
    press.left = left;
  }

  if (right != press.right) {
    setPin(PIN_RIGHT_GPIO, right);
    press.right = right;
  }

  if (a != press.a) {
    setPin(PIN_A_GPIO, a);
    press.a = a;
  }

  if (b != press.b) {
    setPin(PIN_B_GPIO, b);
    press.b = b;
  }
}
void onExitKeyboardMode() {
  Serial.println("Exit KEYBOARD");
}

// MOUSE
void onEnterMouseMode() {
  Serial.println("Enter MOUSE");
}
void onLoopMouseMode() {
  static unsigned long timer = 0;
  setLED(true);

  if (BP32.update()) {
    mouse.buttons = player->buttons();
    mouse.wheel = player->scrollWheel();
    mouse.deltaX = player->deltaX();
    mouse.deltaY = player->deltaY();
  } else {
    mouse.wheel = 0;
    mouse.deltaX = 0;
    mouse.deltaY = 0;
  }

  mouse.x = mouse.x - mouse.deltaX;
  mouse.y = mouse.y + mouse.deltaY;
  char x = mouse.x * MOUSE_SCALE / 20;
  char y = mouse.y * MOUSE_SCALE / 20;
  bool bA = mouse.buttons & UNI_MOUSE_BUTTON_LEFT;
  bool bB = mouse.buttons & UNI_MOUSE_BUTTON_RIGHT;
  bool bC = mouse.buttons & UNI_MOUSE_BUTTON_MIDDLE;

  Serial.printf("[BLUEBUZZ] X: %d, Y: %d, L: %d, R: %d, M: %d, W: %d\n", x, y, bA, bB, bC, mouse.wheel);

  setPin(PIN_A_GPIO, bA);
  setPin(PIN_B_GPIO, bB);
  timer = millis() + 40;
  sendMSXDataByte(x, timer);
  sendMSXDataByte(y, timer);

  if (millis() < timer) {
    mouse.x = 0;
    mouse.y = 0;
    timer = millis() + 2;
  }
  while (digitalRead(PIN_OUT_GPIO) == LOW) {
    if (millis() > timer)
      break;
  }

  setPin(PIN_UP_GPIO, false);
  setPin(PIN_DOWN_GPIO, false);
  setPin(PIN_LEFT_GPIO, false);
  setPin(PIN_RIGHT_GPIO, false);
}
void onExitMouseMode() {
  Serial.println("Exit MOUSE");
}

// OTA
void onEnterOTA() {
  if (player && player->isConnected())
    player->disconnect();
  player = nullptr;
  BP32.forgetBluetoothKeys();
  BP32.enableNewBluetoothConnections(false);
  BP32.enableBLEService(false);

  for (int i = 0; i < 5; i++) {
    setLED(true);
    delay(400);
    setLED(false);
    delay(400);
  }
  delay(1600);

  IPAddress local_ip(192, 168, 1, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);

  Serial.println("Enter OTA");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAPConfig(local_ip, gateway, subnet)) {
    changeState(STATE_ERROR);
    return;
  }
  if (!WiFi.softAP("BlueBuzz")) {
    changeState(STATE_ERROR);
    return;
  }
  Serial.printf("SSID : %s\n", WiFi.softAPSSID());
  Serial.printf("AP : %s\n", WiFi.softAPIP().toString());
  Serial.printf("connect to http://%s/update\n", WiFi.softAPIP().toString());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/update");
  });
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  server.begin();
  Serial.println("HTTP server started");
}
void onLoopOTA() {
  ElegantOTA.loop();
  setLED(true);
  delay(200);
  setLED(false);
  delay(1000);
}
void onExitOTA() {
  Serial.println("Exit OTA");
}

// ERROR
void onEnterError() {
  Serial.println("Enter ERROR");
}
void onLoopError() {
  changeState(STATE_PAIRING);
}
void onExitError() {
  Serial.println("Exit ERROR");
}

#pragma endregion

#pragma region Utility Implementation

void setPin(int pin, bool pressed) {
  if (pressed) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  } else {
    pinMode(pin, INPUT);
  }
}

void setRumble(int duration, uint8_t strengthLeft, uint8_t strengthRight) {
  if (player && player->isConnected() && player->isGamepad()) {
    player->playDualRumble(0, duration, strengthLeft, strengthRight);
  }
}

void onConnectedController(ControllerPtr ctl) {
  if (player == nullptr) {
    player = ctl;
    Serial.printf("[BLUEBUZZ] connected\n");
    setRumble();
    changeState(STATE_CONNECTED);
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  if (player == ctl) {
    if (player && player->isConnected())
      player->disconnect();
    player = nullptr;
    Serial.printf("[BLUEBUZZ] disconnected\n");
    if (currentState == STATE_CONNECTED || currentState == STATE_GAMEPAD || currentState == STATE_KEYBOARD || currentState == STATE_MOUSE)
      changeState(STATE_PAIRING);
  }
}

void sendMSXDataByte(char dataByte, unsigned long timer) {
  // Wait for OUT pin to go HIGH
  while (digitalRead(PIN_OUT_GPIO) == LOW) {
    if (millis() > timer) return;
  }
  // Set pins for high nibble
  setPin(PIN_RIGHT_GPIO, !(dataByte & 0x80));
  setPin(PIN_LEFT_GPIO, !(dataByte & 0x40));
  setPin(PIN_DOWN_GPIO, !(dataByte & 0x20));
  setPin(PIN_UP_GPIO, !(dataByte & 0x10));

  // Wait for OUT pin to go LOW
  while (digitalRead(PIN_OUT_GPIO) == HIGH) {
    if (millis() > timer) return;
  }
  // Set pins for low nibble
  setPin(PIN_RIGHT_GPIO, !(dataByte & 0x08));
  setPin(PIN_LEFT_GPIO, !(dataByte & 0x04));
  setPin(PIN_DOWN_GPIO, !(dataByte & 0x02));
  setPin(PIN_UP_GPIO, !(dataByte & 0x01));
}

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  static unsigned long ota_progress_millis = 0;
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

void setLED(bool status) {
  static bool led_status = false;
  if (led_status == status)
    return;

  led_status = status;
  digitalWrite(PIN_LED_GPIO, led_status ? HIGH : LOW);
}

#pragma endregion