void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop() {
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    Serial1.println(data);
  }
  if (Serial1.available()) {
    String data = Serial1.readStringUntil('\n');
    Serial.println(data);
  }
}
