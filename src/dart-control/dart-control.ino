#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <Stepper.h>

Adafruit_SSD1306 display(4);

// Arduino constant pins.
#define FLYWHEEL_SPEED_PIN A0  // Analog 0
#define FLYWHEEL_BIAS_PIN  A1  // Analog 1
#define PUSHER_DPS_PIN     A2  // Analog 2
#define PUSHER_BURST_PIN   A3  // Analog 3
#define STEPPER_PIN_A1     4   // Digital 4
#define STEPPER_PIN_A2     5   // Digital 5
#define STEPPER_PIN_B1     6   // Digital 6
#define STEPPER_PIN_B2     7   // Digital 7
#define TRIGGER_PIN        8   // Digital 8
#define LOWER_FLYWHEEL     9   // Digital 9
#define UPPER_FLYWHEEL     10  // Digital 10
#define DISPLAY_PIN        11  // Digital 11
#define PUSHER_HOME_MARKER 12  // Digital 12

// Internal constants.
#define STEPS_PER_ROTATION     200
#define ONE_SECOND             1000
#define FLYWHEEL_MIN_VALUE     1060
#define FLYWHEEL_MAX_VALUE     1860
#define FLYWHEEL_SPIN_TIME     3000
#define CALIBRATION_DELAY_TIME 3000

// External input values.
int flywheelMaxSpeed;
int flywheelUpperBias;
int flywheelLowerBias;
int pusherDps; // Darts per second
int magSize;
int remainingDarts;

// Internal counting values.
int flywheelsTimeRemaining;
int dartsToPush;
int totalDartsFired;

// Internal state.
int  displayId;
bool flywheelsSpinning;

// Objects.
Servo upperFlywheel;
Servo lowerFlywheel;
Stepper pusher(STEPS_PER_ROTATION, STEPPER_PIN_A1, STEPPER_PIN_A2, STEPPER_PIN_B1, STEPPER_PIN_B2);

void info(const char str[]) {
  if (Serial) {
    Serial.print(str);
  }
}

void info(int i) {
  if (Serial) {
    Serial.print(i);
  }
}

void setup() {
  // Assign pins.
  pinMode(STEPPER_PIN_A1, OUTPUT);
  pinMode(STEPPER_PIN_A2, OUTPUT);
  pinMode(STEPPER_PIN_B1, OUTPUT);
  pinMode(STEPPER_PIN_B2, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT);
  upperFlywheel.attach(UPPER_FLYWHEEL);
  lowerFlywheel.attach(LOWER_FLYWHEEL);
  // Start the system.
  Serial.begin(9600);
  startDisplay();
//  homePusher();
//  calibrateFlywheels();
  info("AM-1 is hot, have fun.\n");
}

void startDisplay() {
   display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
   display.clearDisplay();
   display.display();
}

void updateDisplay() {
  if (digitalRead(DISPLAY_PIN) == HIGH) {
    displayId++;
    while (digitalRead(DISPLAY_PIN) == HIGH) {
      delay(200);
    }
  }
  if (displayId > 6) {
    displayId = 0;
  }
  renderDisplay(displayId);
}

void renderDisplay(int id) {
  display.clearDisplay();
  switch (id) {
    case 0:
      // Show remaining darts.
      renderRemainingDarts();
      break;
    case 1:
      // Change Mag Size.
      renderMagSize();
      break;
    case 2:
      // Change Mode.
      renderMode();
      break;
    case 3:
      // Change DPS.
      renderDps();
      break;
    case 4:
      // Change FPS.
      renderFps();
      break;
    case 5:
      // Change Bias.
      renderBias();
      break;
    case 6:
      // Show Info.
      renderInfo();
      break;
  }
  display.display();
}

void renderRemainingDarts() {
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("REMAINING");
  display.setTextSize(3);
  display.setCursor(0, 10);
  display.println(remainingDarts);
}

void renderMagSize() {
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("MAG SIZE");
  display.setTextSize(3);
  display.setCursor(0, 10);
  display.println(magSize);
}

void renderFps() {
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("FPS");
  display.setTextSize(3);
  display.setCursor(0, 10);
  display.println(flywheelMaxSpeed - FLYWHEEL_MIN_VALUE);
}

void renderDps() {
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("DPS");
  display.setTextSize(3);
  display.setCursor(0, 10);
  display.println(pusherDps);
}

void renderBias() {
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("BIAS");
  display.setTextSize(3);
  display.setCursor(0,10);
  display.println(flywheelUpperBias);
  display.setCursor(65, 10);
  display.println(flywheelLowerBias);
}

void renderMode() {
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("BURST MODE");
  display.setTextSize(3);
  display.setCursor(0, 10);
  switch (dartsToPush) {
    case 1:
      display.println("Single");
      break;
    case 2:
      display.println("Double");
      break;
    case 3:
      display.println("Tripple");
      break;
    case 4:
      display.println("Auto");
      break;
  }
}

void renderInfo() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  // Top.
  display.setCursor(0, 0);
  display.print("INFO");
  // Left side.
  display.setCursor(0, 8);
  display.print("SPEED: ");
  display.println(flywheelMaxSpeed - FLYWHEEL_MIN_VALUE);
  display.print("BURST: ");
  display.println(dartsToPush);
  display.print("FIRED: ");
  display.println(totalDartsFired);
  // Right Side.
  display.setCursor(80, 8);
  display.print("DPS:   ");
  display.println(pusherDps);
  display.setCursor(80,16);
  display.print("UB: ");
  display.println(flywheelUpperBias);
  display.setCursor(80, 24);
  display.print("LB: ");
  display.println(flywheelLowerBias);
}

void homePusher() {
  info("Calibration of pusher.\n");
  pusher.step(10); // Move the pusher forward incase it's aready at home.
  byte marker = digitalRead(PUSHER_HOME_MARKER);
  while (marker == HIGH) {
    pusher.step(-1); // Step back and check it's location.
    marker = digitalRead(PUSHER_HOME_MARKER);
  }
//  pusher.setCurrentPosition(0);
  info("Calibration of pusher compeleted.\n");
}

// Used by the Afro ESC 12A Speed Controller.
// This does not work if the Arduino is powered via USB.
void calibrateFlywheels() {
  info("Calibration of flywheels started.\n");
  upperFlywheel.writeMicroseconds(FLYWHEEL_MAX_VALUE);
  lowerFlywheel.writeMicroseconds(FLYWHEEL_MAX_VALUE);
  delay(CALIBRATION_DELAY_TIME);
  upperFlywheel.writeMicroseconds(FLYWHEEL_MIN_VALUE);
  lowerFlywheel.writeMicroseconds(FLYWHEEL_MIN_VALUE);
  delay(CALIBRATION_DELAY_TIME);
  info("Calibration of flywheels compeleted.\n");
}

// Read from potentiometer.
void setFlywheelSpeed() {
  flywheelMaxSpeed = map(analogRead(FLYWHEEL_SPEED_PIN), 0, 1023, FLYWHEEL_MIN_VALUE, FLYWHEEL_MAX_VALUE);
}

// Read from potentiometer.
void setFlywheelBias() {
  int bias = map(analogRead(FLYWHEEL_BIAS_PIN), 0, 1023, -50, 50);
  flywheelUpperBias = bias;
  flywheelLowerBias = bias * -1;
}

// Read from potentiometer.
void setDartsPerSecond() {
  pusherDps = map(analogRead(PUSHER_DPS_PIN), 20, 1000, 1, 4);
  pusher.setSpeed(60 * pusherDps); // Rotations per minute.
}

// Read from potentiometer.
void setDartsToPush() {
  // 1 = Single round.
  // 2 = Double tap.
  // 3 = Three round burst.
  // 4 = Full Auto.
  dartsToPush = map(analogRead(PUSHER_BURST_PIN), 20, 1000, 1, 4);
}

// Read from momentary switch.
bool getTriggerState() {
  return digitalRead(TRIGGER_PIN) == HIGH;
}

void startFlywheels() {
  // Ramp up speed if more than 50%.
  // ...
  // Set the final speed.
  upperFlywheel.writeMicroseconds(flywheelMaxSpeed + flywheelUpperBias);
  lowerFlywheel.writeMicroseconds(flywheelMaxSpeed + flywheelLowerBias);
  flywheelsSpinning = true;
  info("Flywheels started.\n");
}

void stopFlywheels() {
  upperFlywheel.writeMicroseconds(FLYWHEEL_MIN_VALUE);
  lowerFlywheel.writeMicroseconds(FLYWHEEL_MIN_VALUE);
  flywheelsSpinning = false;
  info("Flywheels stopped.\n");
}

void updateFlywheels() {
  // Start the flywheels.
  if (flywheelsTimeRemaining > 0 && flywheelsSpinning == false) {
    startFlywheels();
  }
  // Reduce remaining flywheel time.
  if (flywheelsSpinning == true) {
    flywheelsTimeRemaining--;
    delay(1);
  }
  // If we are out of time stop the flywheels.
  if (flywheelsTimeRemaining <= 0 && flywheelsSpinning == true) {
    stopFlywheels();
  }
}

void updatePusher() {
  if (dartsToPush == 4) {
    autoPusher();
  } else {
    burstPusher(dartsToPush);
  }
}

void burstPusher(int i) {
  for(i; i > 0; i--) {
    pushDart();
  }
  while (getTriggerState()) {
    info("Waiting for the trigger to be released...\n");
    delay(200);
  }
}

void autoPusher() {
  while (getTriggerState()) {
    pushDart();
  }
}

void pushDart() {
  pusher.step(90);
  pusher.step(-90);
  totalDartsFired++;
  info("Dart fired.\n");
}

void loop() {
  if(getTriggerState()) {
    flywheelsTimeRemaining = FLYWHEEL_SPIN_TIME;
    updatePusher();
    infoFiringRequest();
    displayId = 0;
  }
  setFlywheelSpeed();
  setFlywheelBias();
  setDartsPerSecond();
  setDartsToPush();
  updateFlywheels();
  updateDisplay();
}

void infoFiringRequest() {
  info("--------------\n");
  info("Firing Request\n");
  info("--------------\n");
  info("Flywheel speed: ");
  info(flywheelMaxSpeed - FLYWHEEL_MIN_VALUE);
  info("\n");
  info("Upper flywheel bias: ");
  info(flywheelUpperBias);
  info("\n");
  info("Lower flywheel bias: ");
  info(flywheelLowerBias);
  info("\n");
  info("Darts per second: ");
  info(pusherDps);
  info("\n");
  info("Dart burst count: ");
  info(dartsToPush);
  info("\n");
  info("Total darts fired: ");
  info(totalDartsFired);
  info("\n");
}

