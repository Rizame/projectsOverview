#include <QTRSensors.h>
#include <Servo.h>
#include "Wire.h"
#include "MPU6050.h"

Servo myservo;

MPU6050 mpu;

QTRSensors qtr;


const int RPWM_PIN = 5;  // PWM pin for motor speed control
const int LPWM_PIN = 6;  // PWM pin for motor direction control


// Define motor speeds
const int OLd_RAMP_SPEED = 90;
const int RAMP_SPEED = 72;

const int OLD_SPEED = 27;
const int OBSTACLE_SPEED = 22;
const int DEFAULT_SPEED = OLD_SPEED;

// const int RAMP_DOWN_SPEED = 38;


const int SLIGHT_LEFT = 105;


int MOTOR_SPEED = 0;  // Adjust this value to control the speed of the motor
const int STOP_SPEED = 0;
//TODO change threshold and test it
const int WHITE_THRESHOLD = 250;

uint16_t position;

const int trigPin = A2;
const int echoPin = A1;

const int buzzerPin = A3;

const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];

//MPU value
int16_t ax;

unsigned long startTime = 0;             // Variable to keep track of the time when the threshold is exceeded
const unsigned long RampDuration = 400;  // Duration of ramp in ms

int melody[] = {
  523, 659, 784, 1047,
  523, 1047, 1319, 1568,
  1047, 988, 880, 784,
  698, 659, 587, 523
};
int noteDurations[] = {
  8, 8, 8, 8,
  8, 8, 8, 8,
  4, 4, 4, 4,
  4, 4, 4, 4
};

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  myservo.attach(11);
  myservo.write(90);
  // Set motor control pins as outputs
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);

  //initialize mpu
  Wire.begin();
  mpu.initialize();

  // configure the sensors
  qtr.setTypeRC();
  qtr.setTimeout(10000);
  qtr.setSensorPins((const uint8_t[]){ 3, 4, 12, 2, 7, 8, 9, 10 }, SensorCount);
  //qtr.setEmitterPin(2);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // turn on Arduino's LED to indicate we are in calibration mode

  // analogRead() takes about 0.1 ms on an AVR.
  // 0.1 ms per sensor * 4 samples per sensor read (default) * 6 sensors
  // * 10 reads per calibrate() call = ~24 ms per calibrate() call.
  // Call calibrate() 400 times to make calibration take about 10 seconds.
  for (uint16_t i = 0; i < 50; i++) {
    qtr.calibrate();
  }
  digitalWrite(LED_BUILTIN, LOW);  // turn off Arduino's LED to indicate we are through with calibration

  // print the calibration minimum values measured when emitters were on
  for (uint8_t i = 0; i < SensorCount; i++) {
    Serial.print(qtr.calibrationOn.minimum[i]);
    Serial.print(' ');
  }
  Serial.println();

  // print the calibration maximum values measured when emitters were on
  for (uint8_t i = 0; i < SensorCount; i++) {
    Serial.print(qtr.calibrationOn.maximum[i]);
    Serial.print(' ');
  }
  Serial.println();
  Serial.println();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);



  MOTOR_SPEED = DEFAULT_SPEED;
}

void loop() {


  runMotor(MOTOR_SPEED);
  float forwardDist = calcDistance(echoPin, trigPin);

  if (forwardDist < 21 && forwardDist > 15) {
    evadeObstacle();
  }

  //Serial.print("Distance in cm: ");
  //Serial.println(distance);
  trackLine();


  // for (uint8_t i = 0; i < SensorCount; i++) {
  //   Serial.print(sensorValues[i]);
  //   Serial.print('\t');
  // }
  // Serial.println(" ");
  // Serial.println(position);


  calcAngle();

  if (checkRamp()) {
    int state = 0;
    myservo.write(90);

    while (state != 3) {

      if (state == 0) {
        runMotor(RAMP_SPEED);
        startTime = 0;
        while (state == 0) {
          calcAngle();
          Serial.print(state);
          Serial.print("\t");
          Serial.print(ax);
          Serial.println();
          if (checkForward(35)) {
            Serial.println("CHANGE TO STATE 1");
            state = 1;
          }
        }
      }

      if (state == 1) {
        startTime = 0;
        // runMotor(0);
        // delay(300);
        runMotor(DEFAULT_SPEED - 10);
        while (state == 1) {
          calcAngle();
          Serial.print(state);
          Serial.print("\t");
          Serial.print(ax);
          Serial.println();
          if (checkDown()) {
            state = 2;
          }
        }
      }

      if (state == 2) {
        stopMotor();
        // runMotor(STOP_SPEED);
        startTime = 0;
        while (state == 2) {
          calcAngle();
          Serial.print(state);
          Serial.print("\t");
          Serial.print(ax);
          Serial.println();
          if (checkForward(50)) {
            state = 3;
          }
        }
      }
      // Ensure MOTOR_SPEED is reset when state reaches 3
      if (state == 3) {
        MOTOR_SPEED = DEFAULT_SPEED;
        runMotor(MOTOR_SPEED);
      }
    }
  }
}

float calcDistance(int echo, int trig) {
  digitalWrite(trig, LOW); 
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);//emit ultra sound wave
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  float duration = pulseIn(echo, HIGH); //receive the duration of a reflected ultra sound wave
  float distance = (duration * 0.0343) / 2; //The speed of sound is approximately 343 meters per second,
  //or 0.0343 centimeters per microsecond. Multiply by it to conver to cm.
  return distance; //Finally divide by 2 as it is the distance that sound wave took traveling both directions.
}

void calcAngle() {
  ax = mpu.getAccelerationX();
  ax = map(ax, -17000, 17000, 0, 255);  // X axis data
}



void evadeObstacle() {
  myservo.write(90);
  runMotor(0);
  delay(150);
  runMotor(-DEFAULT_SPEED);
  Serial.println("back");
  delay(1000);
  runMotor(0);
  delay(150);
  myservo.write(45);
  runMotor(DEFAULT_SPEED-2);
  Serial.println("Right");
  delay(1500);
  myservo.write(135);
  Serial.println("Left");
  delay(2000);
  myservo.write(90);
  runMotor(-DEFAULT_SPEED);
  Serial.println("back");
  delay(800);
  runMotor(0);
  delay(150);
  runMotor(DEFAULT_SPEED - 3);
  Serial.println("left to line");
  myservo.write(105);
  position = qtr.readLineWhite(sensorValues);
  while (isFullyOnBlack()) {
    position = qtr.readLineWhite(sensorValues);
  }
  runMotor(STOP_SPEED);
  trackLine();
  delay(400);
  trackLine();
  runMotor(DEFAULT_SPEED);
}



bool checkDown() {
  unsigned long currentTime = millis();  // Get the current time in milliseconds

  if (ax < 95) {
    if (startTime == 0) {
      startTime = currentTime;  // Record the time when the threshold is first exceeded
    }
    if (currentTime - startTime >= 10) {
      return true;  // Execute the desired code if the condition is met
    }
  } else {
    startTime = 0;  // Reset the start time if the variable drops below the threshold
  }
  return false;
}

bool checkForward(int forwDuration) {
  unsigned long currentTime = millis();  // Get the current time in milliseconds

  if (ax > 95 && ax < 140) {
    if (startTime == 0) {
      startTime = currentTime;  // Record the time when the threshold is first exceeded
    }
    if (currentTime - startTime >= forwDuration) {
      return true;  // Execute the desired code if the condition is met
    }
  } else {
    startTime = 0;  // Reset the start time if the variable drops below the threshold
  }
  return false;
}



bool checkRamp() {
  unsigned long currentTime = millis();  // Get the current time in milliseconds

  if (ax > 137) {
    if (startTime == 0) {
      startTime = currentTime;  // Record the time when the threshold is first exceeded
    }
    if (currentTime - startTime >= RampDuration) {
      return true;  // Execute the desired code if the condition is met
    }
  } else {
    startTime = 0;  // Reset the start time if the variable drops below the threshold
  }
  return false;
}



// Function to control the motor speed
void runMotor(int speed) {
  if (speed >= 0) { //if speed >= 0 moving forward
    digitalWrite(RPWM_PIN, LOW); //turn off backward direction
    analogWrite(LPWM_PIN, speed); //set the speed to forward direction pin
  }  
  else { //if speed < 0 moving backwards
    digitalWrite(LPWM_PIN, LOW);//turn off forward direction
    analogWrite(RPWM_PIN, -speed);//set the speed to backward direction pin
  }
}

void stopMotor() {
  // Set motor direction (assuming you're using an H-Bridge)
  // For simplicity, let's assume LPWM_PIN always needs to be LOW for forward direction
  digitalWrite(LPWM_PIN, LOW);
  analogWrite(RPWM_PIN, 21);
}

void playMusic(){
  for (int thisNote = 0; thisNote < sizeof(melody) / sizeof(melody[0]); thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(buzzerPin, melody[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(buzzerPin);
  }
}

bool isFullyOnWhite() {
  static unsigned long whiteLineStartTime = 0;
  bool allWhite = true;

  for (int i = 0; i < SensorCount; i++) {
    if (sensorValues[i] > WHITE_THRESHOLD) {
      allWhite = false;
      whiteLineStartTime = 0; 
      break;
    }
  }

  if (allWhite) {
    if (whiteLineStartTime == 0) {
      whiteLineStartTime = millis();
    }
    else if (millis() - whiteLineStartTime >= 25) {
      runMotor(STOP_SPEED);
        playMusic();
        Serial.println("FULL");
        while (true) {
          delay(1000);
        }
    }
  }

  return false;
}

bool isFullyOnBlack() {
  int count = 0;
  for (int i = 0; i < SensorCount; i++) {
    Serial.println(sensorValues[i]);
    Serial.print(" black sensor value");
    if (sensorValues[i] < 700) {
      count++;
    }
  }
  if (count > 2) return false; /// usually > 2
  return true;
}



void trackLine() {
  position = qtr.readLineWhite(sensorValues);
  if (isFullyOnWhite()) {
    MOTOR_SPEED = STOP_SPEED;
    runMotor(MOTOR_SPEED);
    Serial.println("FULL STOP");
  }
  float servoangle = (float)position / 7000;
  servoangle = servoangle * 90;
  servoangle = 45 + servoangle;
  myservo.write(servoangle);
}