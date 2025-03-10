#include <TFT.h>
#include <SPI.h>
#include <Stepper.h>
#include <Servo.h>

#define cs   48
#define dc   44
#define rst  46
#define led  9

TFT TFTscreen = TFT(cs, dc, rst);
Stepper stepper1(2048, 22, 24, 26, 28);
Stepper stepper2(2048, 30, 32, 34, 36);
Servo servo;
String wasteType = "N";
String confP = "3.5";
int initialPos1 = 0;
int initialPos2 = 0;
int initialPos3 = 0;

void setup() {
  Serial1.begin(115200);
  TFTscreen.begin();
  TFTscreen.background(0, 0, 0);
  TFTscreen.setTextSize(2);
  pinMode(led, OUTPUT);
  analogWrite(led, 255);
  stepper1.setSpeed(15);
  stepper2.setSpeed(15);
  servo.attach(10);
  servo.write(initialPos3);
  updateDisplay();
}

void updateDisplay() {
  TFTscreen.background(0, 0, 0);
  TFTscreen.stroke(255, 255, 255);
  TFTscreen.text("W: ", 0, 0);
  TFTscreen.text(wasteType.c_str(), 40, 0);
  TFTscreen.text("ConfP: ", 0, 30);
  TFTscreen.text(confP.c_str(), 60, 30);
}

void loop() {
  if (Serial1.available()) {
    String command = Serial1.readStringUntil('\n');
    if (command.startsWith("D:")) {
      command = command.substring(2);
      int comma1 = command.indexOf(",");
      int comma2 = command.lastIndexOf(",");
      int deg1 = command.substring(0, comma1).toInt();
      int deg2 = command.substring(comma1 + 1, comma2).toInt();
      int deg3 = command.substring(comma2 + 1).toInt();
      stepper1.step((deg1 - initialPos1) * 2048 / 360);
      stepper2.step((deg2 - initialPos2) * 2048 / 360);
      servo.write(deg3);
      initialPos1 = deg1;
      initialPos2 = deg2;
      initialPos3 = deg3;
    } else if (command.startsWith("W:")) {
      command = command.substring(2);
      int comma1 = command.indexOf(",");
      int comma2 = command.indexOf(",", comma1 + 1);
      int comma3 = command.indexOf(",", comma2 + 1);
      int comma4 = command.lastIndexOf(",");
      wasteType = command.substring(0, comma1);
      confP = command.substring(comma1 + 1, comma2);
      int deg1 = command.substring(comma2 + 1, comma3).toInt();
      int deg2 = command.substring(comma3 + 1, comma4).toInt();
      int deg3 = command.substring(comma4 + 1).toInt();
      stepper1.step((deg1 - initialPos1) * 2048 / 360);
      stepper2.step((deg2 - initialPos2) * 2048 / 360);
      servo.write(deg3);
      initialPos1 = deg1;
      initialPos2 = deg2;
      initialPos3 = deg3;
      delay(2000);
      stepper1.step(-deg1 * 2048 / 360);
      stepper2.step(-deg2 * 2048 / 360);
      servo.write(0);
      initialPos1 = 0;
      initialPos2 = 0;
      initialPos3 = 0;
      updateDisplay();
    } else if (command == "R") {
      stepper1.step(-initialPos1 * 2048 / 360);
      stepper2.step(-initialPos2 * 2048 / 360);
      servo.write(0);
      initialPos1 = 0;
      initialPos2 = 0;
      initialPos3 = 0;
    }
  }
}