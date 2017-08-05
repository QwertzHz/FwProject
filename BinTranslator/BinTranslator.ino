int bits = 0;
byte out = 0;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
}

void loop() {
  while (Serial.available()) {
    if (Serial.read() != 48) {
      out = out | 1<<(7 - bits);
    }
    bits++;
    
    if (bits == 8) {
      Serial1.write(out);
      Serial.print("-> ");
      Serial.print(out);
      Serial.print("\n");
      bits = 0;
      out = 0;
    }
  }

  while (Serial1.available()) {
    byte r = Serial1.read();
    Serial.print("<- 0b");
    Serial.print(String(r, BIN));
    Serial.print(" (");
    Serial.print(String(r, DEC));
    Serial.print(")\n");
  }
}
