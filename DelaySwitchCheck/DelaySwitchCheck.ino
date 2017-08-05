void setup() {
  // put your setup code here, to run once:
  pinMode(A0, INPUT);
  Serial.begin(2400);
}

void loop() {
  // put your main code here, to run repeatedly:
  analogRead(A0);
  Serial.println(analogRead(A0));
}
