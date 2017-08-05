#include <LoadStore.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>

Adafruit_MCP23017 mcp;
LoadStore loadStore;

const int RELAY_FIRE_STATE = LOW, RELAY_NOFIRE_STATE = HIGH;

const int UNARMED_SWITCH_PIN = 13;
const int ARMED_STATE_UPDATE = 500;
int armedStateUpdateTimer = 0;

const long relayHoldTime = 700;
unsigned long lastMillis, sinceLastMillis;

uint16_t states = 0;
long holds[16];

const int DELAY_PRESETS[6] = { 0, 500, 1000, 3000, 5000, 10000 };

const byte PRIMARYCOMMANDMASK =   0b10000000;
const byte SECONDARYCOMMANDMASK = 0b01110000;
const byte PARAMETERMASK =        0b00001111;

const byte COMMAND_UNLOAD = 0b00000000;  // "0"
const byte COMMAND_SEND = 0b01000000;    // "4"
const byte COMMAND_CLEAR = 0b01010000;   // "5"
const byte COMMAND_HALT = 0b00100000;    // "2"
const byte COMMAND_INSTANT = 0b00110000; // "3"
const byte COMMAND_FIRE = 0b01110000;    // "7"

void setup() {
  lastMillis = 0;
  sinceLastMillis = 0;
  
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  
  mcp.begin(0);
  for (int i = 0; i < 16; i++) {
    mcp.pinMode(i, OUTPUT);
    mcp.digitalWrite(i, RELAY_NOFIRE_STATE);
  }

  for (int i = 0; i < 16; i++) {
    holds[i] = relayHoldTime;
  }

  pinMode(UNARMED_SWITCH_PIN, INPUT_PULLUP);

  Serial.begin(2400);
}

void loop() {
  sinceLastMillis = millis() - lastMillis;
  lastMillis = millis();

  armedStateUpdateTimer += sinceLastMillis;
  if (armedStateUpdateTimer >= ARMED_STATE_UPDATE) {
    armedStateUpdateTimer -= ARMED_STATE_UPDATE;
    Serial.write((digitalRead(UNARMED_SWITCH_PIN) == HIGH) ? 0xA1 : 0xA0);
  }

  while (Serial.available()) {
    byte command = Serial.read();
    byte primary = (command & PRIMARYCOMMANDMASK);
    byte secondary = (command & SECONDARYCOMMANDMASK);
    byte parameter = (command & PARAMETERMASK);
    if (primary) { // LOAD
      loadStore.setLoad(parameter, secondary);
    } else { // Some other command
      if (secondary == COMMAND_UNLOAD) {
        loadStore.clearLoad(parameter);
      }
      if (secondary == COMMAND_SEND) {
        for (int i = 0; i < 8; i++) {
          Serial.write(loadStore.getByte(i));
        }
      }
      if (secondary == COMMAND_CLEAR) {
        loadStore.clearAll();
      }
      if (secondary == COMMAND_HALT) {
        for (int i = 0; i < 16; i++) {
          holds[i] = relayHoldTime;
        }
      }
      if (secondary == COMMAND_INSTANT) {
        holds[parameter] = 0;
      }
      if (secondary == COMMAND_FIRE) {
        for (int i = 0; i < 16; i++) {
          if (loadStore.getLoadState(i)) {
            holds[i] = -DELAY_PRESETS[loadStore.getLoadDelayPreset(i)];
          }
        }
        loadStore.clearAll();
      }
    }
  }

  for (int i = 0; i < 16; i++) {
    if (holds[i] > relayHoldTime) {
      holds[i] = relayHoldTime;
    }
    if (holds[i] == relayHoldTime) {
      mcp.digitalWrite(i, RELAY_NOFIRE_STATE);
    }
    if (holds[i] < relayHoldTime) {
      if (holds[i] >= 0) {
        mcp.digitalWrite(i, RELAY_FIRE_STATE);
      }
      holds[i] += sinceLastMillis;
    }
  }
}

