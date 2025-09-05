#include <WiFi.h>
#include <Bluepad32.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

enum Pin {
  PIN_UP,
  PIN_DOWN,
  PIN_LEFT,
  PIN_RIGHT,
  PIN_A,
  PIN_B,
  PIN_OUT,
  PIN_LED
};

const int PLAYER_PINS[8] = { 14, 26, 33, 32, 27, 25, 17, 18 };
const int DELAY_MS = 15;
const char MOUSE_SCALE = 15;
const char *OTA_SSID = "BlueBuzz";
const char *OTA_PASS = "12345678";
ControllerPtr player;
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
uint16_t cur_buttons = 0;
int8_t cur_wheel = 0;
int32_t cur_deltaX, cur_deltaY = 0;
unsigned long timer;
AsyncWebServer server(80);
unsigned long ota_progress_millis = 0;
bool led_status = false;

void setPin(int pin, bool pressed);
void setRumble(int duration = 300, uint8_t strengthLeft = 0x80, uint8_t strengthRight = 0x40);
void sendMSX(char c);
void onOTAStart();
void onOTAProgress(size_t current, size_t final);
void onOTAEnd(bool success);
void setLED(bool status);

#pragma region StateMachine

enum State {
  STATE_INIT = 0,
  STATE_PAIRING,
  STATE_CONNECTED,
  STATE_PLAYING_BY_GAMEPAD,
  STATE_PLAYING_BY_KEYBOARD,
  STATE_PLAYING_BY_MOUSE,
  STATE_OTA,
  STATE_ERROR,
  STATE_NULL,
  STATE_COUNT
};

State currentState = STATE_NULL;

void onEnterInit();
void onLoopInit();
void onExitInit();

void onEnterPairing();
void onLoopPairing();
void onExitPairing();

void onEnterConnected();
void onLoopConnected();
void onExitConnected();

void onEnterPlayingByGamepad();
void onLoopPlayingByGamepad();
void onExitPlayingByGamepad();

void onEnterPlayingByKeyboard();
void onLoopPlayingByKeyboard();
void onExitPlayingByKeyboard();

void onEnterPlayingByMouse();
void onLoopPlayingByMouse();
void onExitPlayingByMouse();

void onEnterOTA();
void onLoopOTA();
void onExitOTA();

void onEnterError();
void onLoopError();
void onExitError();

void changeState(State newState) {
  if (newState == currentState) return;

  switch (currentState) {
    case STATE_INIT: onExitInit(); break;
    case STATE_PAIRING: onExitPairing(); break;
    case STATE_CONNECTED: onExitConnected(); break;
    case STATE_PLAYING_BY_GAMEPAD: onExitPlayingByGamepad(); break;
    case STATE_PLAYING_BY_KEYBOARD: onExitPlayingByKeyboard(); break;
    case STATE_PLAYING_BY_MOUSE: onExitPlayingByMouse(); break;
    case STATE_OTA: onExitOTA(); break;
    case STATE_ERROR: onExitError(); break;
    default: break;
  }

  currentState = newState;

  switch (currentState) {
    case STATE_INIT: onEnterInit(); break;
    case STATE_PAIRING: onEnterPairing(); break;
    case STATE_CONNECTED: onEnterConnected(); break;
    case STATE_PLAYING_BY_GAMEPAD: onEnterPlayingByGamepad(); break;
    case STATE_PLAYING_BY_KEYBOARD: onEnterPlayingByKeyboard(); break;
    case STATE_PLAYING_BY_MOUSE: onEnterPlayingByMouse(); break;
    case STATE_OTA: onEnterOTA(); break;
    case STATE_ERROR: onEnterError(); break;
    default: break;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial.availableForWrite())
    delay(100);
  changeState(STATE_INIT);
}

void loop() {
  switch (currentState) {
    case STATE_INIT: onLoopInit(); break;
    case STATE_PAIRING: onLoopPairing(); break;
    case STATE_CONNECTED: onLoopConnected(); break;
    case STATE_PLAYING_BY_GAMEPAD: onLoopPlayingByGamepad(); break;
    case STATE_PLAYING_BY_KEYBOARD: onLoopPlayingByKeyboard(); break;
    case STATE_PLAYING_BY_MOUSE: onLoopPlayingByMouse(); break;
    case STATE_OTA: onLoopOTA(); break;
    case STATE_ERROR: onLoopError(); break;
    default: break;
  }
}

#pragma endregion

// INIT
void onEnterInit() {
  Serial.println("Enter INIT");

  WiFi.mode(WIFI_OFF);
  setCpuFrequencyMhz(80);

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();
  BP32.enableVirtualDevice(false);

  for (int i = 0; i < 7; i++)
    setPin(PLAYER_PINS[i], false);
  pinMode(PLAYER_PINS[PIN_LED], OUTPUT);
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
  player = nullptr;
  BP32.forgetBluetoothKeys();
  BP32.enableNewBluetoothConnections(true);
}
void onLoopPairing() {
  BP32.update();
  setLED(true);
  delay(200);
  setLED(false);
  delay(200);
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
        changeState(STATE_PLAYING_BY_GAMEPAD);
      else if (player->isKeyboard())
        changeState(STATE_PLAYING_BY_KEYBOARD);
      else if (player->isMouse())
        changeState(STATE_PLAYING_BY_MOUSE);
    }
  }
}
void onExitConnected() {
  Serial.println("Exit CONNECTED");
}

// PLAYING BY GAMEPAD
void onEnterPlayingByGamepad() {
  Serial.println("Enter PLAYING_BY_GAMEPAD");
}
void onLoopPlayingByGamepad() {
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
void onExitPlayingByGamepad() {
  Serial.println("Exit PLAYING_BY_GAMEPAD");
}

// PLAYING BY KEYBOARD
void onEnterPlayingByKeyboard() {
  Serial.println("Enter PLAYING_BY_KEYBOARD");
}
void onLoopPlayingByKeyboard() {
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
void onExitPlayingByKeyboard() {
  Serial.println("Exit PLAYING_BY_KEYBOARD");
}

// PLAYING BY MOUSE
void onEnterPlayingByMouse() {
  Serial.println("Enter PLAYING_BY_MOUSE");
}
void onLoopPlayingByMouse() {
  setLED(true);

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

  mouse_x = mouse_x - cur_deltaX;
  mouse_y = mouse_y + cur_deltaY;
  char x = mouse_x * MOUSE_SCALE / 20;
  char y = mouse_y * MOUSE_SCALE / 20;
  bool bA = cur_buttons & UNI_MOUSE_BUTTON_LEFT;
  bool bB = cur_buttons & UNI_MOUSE_BUTTON_RIGHT;
  bool bC = cur_buttons & UNI_MOUSE_BUTTON_MIDDLE;

  Serial.printf("[BLUEBUZZ] X: %d, Y: %d, L: %d, R: %d, M: %d, W: %d\n", x, y, bA, bB, bC, cur_wheel);

  setPin(PLAYER_PINS[PIN_A], bA);
  setPin(PLAYER_PINS[PIN_B], bB);
  timer = millis() + 40;
  sendMSX(x);
  sendMSX(y);

  if (millis() < timer) {
    mouse_x = 0;
    mouse_y = 0;
    timer = millis() + 2;
  }
  while (digitalRead(PLAYER_PINS[PIN_OUT]) == LOW) {
    if (millis() > timer)
      break;
  }

  setPin(PLAYER_PINS[PIN_UP], false);
  setPin(PLAYER_PINS[PIN_DOWN], false);
  setPin(PLAYER_PINS[PIN_LEFT], false);
  setPin(PLAYER_PINS[PIN_RIGHT], false);
}
void onExitPlayingByMouse() {
  Serial.println("Exit PLAYING_BY_MOUSE");
}

// OTA
void onEnterOTA() {
  player = nullptr;
  BP32.forgetBluetoothKeys();
  BP32.enableNewBluetoothConnections(false);
  BP32.enableBLEService(false);

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
  if (!WiFi.softAP(OTA_SSID)) {
    changeState(STATE_ERROR);
    return;
  }
  Serial.printf("SSID : %s\n", OTA_SSID);
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
    player = nullptr;
    Serial.printf("[BLUEBUZZ] disconnected\n");
    if (currentState == STATE_CONNECTED || currentState == STATE_PLAYING_BY_GAMEPAD || currentState == STATE_PLAYING_BY_KEYBOARD || currentState == STATE_PLAYING_BY_MOUSE)
      changeState(STATE_PAIRING);
  }
}

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

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
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
  if (led_status == status)
    return;

  led_status = status;
  digitalWrite(PLAYER_PINS[PIN_LED], led_status ? HIGH : LOW);
}