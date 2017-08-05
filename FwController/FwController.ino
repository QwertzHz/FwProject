#include <LoadStore.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>

#define MODE_INSTANT LOW
#define MODE_PROGRAM HIGH

const int POWER_LED_PIN = 6;
const int MODE_SWITCH_PIN = 5;
const int FIRE_BUTTON_PIN = 2;
const int FIRE_LED_PIN = 3;
const int ARMED_LED_PIN = 7;
const int DELAY_SWITCH_PIN = A0;
const int RX_LED_PIN = 8;
const int TX_LED_PIN = 9;

long connectionTime = 0;
const long CONNECTION_TIMEOUT = 2000;

Adafruit_MCP23017 mcpInput, mcpLed;
int mcpInputDebounced[16] = { LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW };
int mcpInputDebouncedLast[16] = { LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW };
int mcpInputLast[16] = { LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW };
unsigned long lastDebounceTime[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned long debounceDelay = 100;
LoadStore loadStore;

bool leds[16];
bool fireButtonState = false;
bool launcherIsArmed = false;

const int delaySwitchSettings[6] = { 0, 229, 459, 688, 918, 1023 };
const int delaySwitchSettingPadding = 50;

int txLedTimer = 0, rxLedTimer = 0;
int xLedHold = 25;

const int PRIMARYCOMMANDPLACE = 7;
const int SECONDARYCOMMANDPLACE = 4;
const int PARAMETERPLACE = 0;

const byte COMMAND_UNLOAD = 0b000;
const byte COMMAND_SEND = 0b100;
const byte COMMAND_CLEAR = 0b101;
const byte COMMAND_HALT = 0b010;
const byte COMMAND_INSTANT = 0b011;
const byte COMMAND_FIRE = 0b111;

int lastMillis = 0, sinceLastMillis = 0;

void setup() {
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  pinMode(11, OUTPUT);
  digitalWrite(11, HIGH);
  pinMode(A1, OUTPUT);
  digitalWrite(A1, HIGH);

  pinMode(POWER_LED_PIN, OUTPUT);
  digitalWrite(POWER_LED_PIN, HIGH);
  pinMode(DELAY_SWITCH_PIN, INPUT);
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
  pinMode(FIRE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(FIRE_LED_PIN, OUTPUT);
  digitalWrite(FIRE_LED_PIN, HIGH);
  pinMode(ARMED_LED_PIN, OUTPUT);
  digitalWrite(ARMED_LED_PIN, LOW);
  pinMode(TX_LED_PIN, OUTPUT);
  digitalWrite(TX_LED_PIN, LOW);
  pinMode(RX_LED_PIN, OUTPUT);
  digitalWrite(RX_LED_PIN, LOW);

  mcpInput.begin(0);
  for (int i = 0; i < 16; i++) {
    mcpInput.pinMode(i, INPUT);
  }
  mcpLed.begin(1);
  for (int i = 0; i < 16; i++) {
    mcpLed.pinMode(i, OUTPUT);
  }

  for (int i = 0; i < 16; i++) {
    leds[i] = false;
  }

  Serial.begin(2400);
}

void loop() {
  sinceLastMillis = millis() - lastMillis;
  lastMillis = millis();

  connectionTime += sinceLastMillis;
  digitalWrite(POWER_LED_PIN, HIGH);
  if (connectionTime >= CONNECTION_TIMEOUT) {
    digitalWrite(POWER_LED_PIN, (lastMillis % 250 > 125) ? HIGH : LOW);
    digitalWrite(ARMED_LED_PIN, (lastMillis % 250 < 125) ? HIGH : LOW);
  }
  
  // Debouncing inputs
  for (int i = 0; i < 16; i++) {
    int reading = mcpInput.digitalRead(i);
    if (reading != mcpInputLast[i]) {
      lastDebounceTime[i] = millis();
    }
    if (millis() - lastDebounceTime[i] > debounceDelay) {
      mcpInputDebounced[i] = reading;
    }
    mcpInputLast[i] = reading;
  }
  
  if (txLedTimer > 0) {
    digitalWrite(TX_LED_PIN, HIGH);
    txLedTimer -= sinceLastMillis;
  }
  if (txLedTimer < 0) {
    txLedTimer = 0;
  }
  if (txLedTimer == 0) {
    digitalWrite(TX_LED_PIN, LOW);
  }
  if (rxLedTimer > 0) {
    digitalWrite(RX_LED_PIN, HIGH);
    rxLedTimer -= sinceLastMillis;
  }
  if (rxLedTimer < 0) {
    rxLedTimer = 0;
  }
  if (rxLedTimer == 0) {
    digitalWrite(RX_LED_PIN, LOW);
  }
  
  while (Serial.available()) {
    rxLed();
    byte in = Serial.read();
    if (in == 0xA0 || in == 0xA1) {
      connectionTime = 0;
      if (in == 0xA0) launcherIsArmed = false;
      if (in == 0xA1) launcherIsArmed = true;
    }
  }
  digitalWrite(ARMED_LED_PIN, launcherIsArmed ? HIGH : LOW);
  
  if (digitalRead(MODE_SWITCH_PIN) == MODE_INSTANT) {
    digitalWrite(FIRE_LED_PIN, LOW);
    runInstant();
  } else {
    digitalWrite(FIRE_LED_PIN, HIGH);
    runProgram();
  }

  for (int i = 0; i < 16; i++) {
    mcpInputDebouncedLast[i] = mcpInputDebounced[i];
  }
}

void runInstant() {
  for (int i = 0; i < 16; i++) {
    if (mcpInputDebounced[i] == LOW && mcpInputDebouncedLast[i] == HIGH) {
      if (!leds[i]) {
        leds[i] = true;
        mcpLed.digitalWrite(i, HIGH);
        txLed();
        Serial.write((COMMAND_INSTANT<<SECONDARYCOMMANDPLACE) + i);
      }
    } else {
      if (leds[i]) {
        leds[i] = false;
      }
      if (mcpInputDebounced[i] == HIGH) {
        mcpLed.digitalWrite(i, LOW);
      }
    }
  }
}

void runProgram() {
  for (int i = 0; i < 16; i++) {
    if (mcpInputDebounced[i] == LOW && mcpInputDebouncedLast[i] == HIGH) {
      if (!loadStore.getLoadState(i)) {
        loadStore.setLoad(i, delaySwitchSettings[delaySwitchSet()]);
      } else {
        loadStore.clearLoad(i);
      }
    }
    if (loadStore.getLoadState(i)) {
      mcpLed.digitalWrite(i, HIGH);
    } else {
      mcpLed.digitalWrite(i, LOW);
    }
  }
  
  if (digitalRead(FIRE_BUTTON_PIN) == LOW) {
    if (!fireButtonState) {
      fireButtonState = true;
      digitalWrite(FIRE_LED_PIN, LOW);
      txLed();
      sendLoads();
      bool fired;
      do {
        fired = true;
        while (Serial.available()) {
          Serial.read();
        }
        txLed();
        Serial.write(COMMAND_SEND<<SECONDARYCOMMANDPLACE);
        while (Serial.available() < 8);
        for (int i = 0; i < 8; i++) {
          rxLed();
          if (Serial.read() != 0b0) {
            fired = false;
          }
        }
        if (!fired) {
          txLed();
          Serial.write(COMMAND_FIRE<<SECONDARYCOMMANDPLACE);
        }
      } while (!fired);
      txLed();
      Serial.write(COMMAND_FIRE<<SECONDARYCOMMANDPLACE);
      loadStore.clearAll();
    }
  } else {
    digitalWrite(FIRE_LED_PIN, HIGH);
    if (fireButtonState) fireButtonState = false;
  }
}

byte delaySwitchSet() {
  int x = analogRead(DELAY_SWITCH_PIN);
  for (int i = 0; i < 6; i++) {
    if (abs(x - delaySwitchSettings[i]) < delaySwitchSettingPadding) {
      return i;
    }
  }
  return 0;
}

void sendLoad(byte load, byte delayPreset) {
  txLed();
  Serial.write((0b1<<PRIMARYCOMMANDPLACE) | (delayPreset<<SECONDARYCOMMANDPLACE) | (load<<PARAMETERPLACE));
}

void sendLoads() {
  if (loadStore.hasNoneLoaded()) return; // Don't try to send loads if there's nothing to send. Duh.
  for (int i = 0; i < 16; i++) {
    if (loadStore.getLoadState(i)) {
      sendLoad(i, loadStore.getLoadDelayPreset(i));
    }
  }
  // At this point, we have sent all of our loads to the launcher.
  
//  bool matched = false;
//  do {
//    matched = true;
//    while (Serial.available()) {
//      Serial.read(); // Take out waiting serial bytes to clean up for the incoming eight
//    }
//    LoadStore launcherLoadStore;
//    txLed();
//    Serial.write(COMMAND_SEND<<SECONDARYCOMMANDPLACE); // Ask the launcher to send its current loads
//    while (Serial.available() < 8); // Wait for all eight bytes of loads
//    for (int i = 0; i < 8; i++) {
//      rxLed();
//      launcherLoadStore.setFromByte(i, Serial.read());
//    }
//    // At this point, we have learned what the launcher has loaded.
//
//    for (int i = 0; i < 16; i++) {
//      if (loadStore.getLoadRaw(i) != launcherLoadStore.getLoadRaw(i)) {
//        matched = false;
//        if (loadStore.getLoadState(i)) {
//          sendLoad(i, loadStore.getLoadDelayPreset(i));
//        } else {
//          txLed();
//          Serial.write(COMMAND_UNLOAD<<SECONDARYCOMMANDPLACE | i<<PARAMETERPLACE);
//          loadStore.clearLoad(i);
//        }
//      }
//    }
//    // At this point, we have gone through a verification.
//    // If we detected any discrepancies, repairs were attempted, and we will verify again.
//  } while (!matched);
//  // At this point, we must be perfectly matched with the launcher's loads.
}

void txLed() {
  txLedTimer += xLedHold;
}

void rxLed() {
  rxLedTimer += xLedHold;
}


