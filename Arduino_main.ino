#include "Wire.h"
#include <SoftwareSerial.h>
#include <MPU6050_light.h>

#define ENA 9
#define ENB 10
#define IN1 2
#define IN2 3
#define IN3 4
#define IN4 12
#define TRIG 11
#define ECHO 6
#define PUSH_BUTTON 13
#define SOUND_SPEED 0.034 / 2
#define IRA A0
#define txPin 8
#define rxPin 7

//Global Variables
int buttonState = 0;
int buttonNew = 0;
int buttonOld = 1;
int offset = 30;
int turn_counter = 1;
String value;


//Declare MPU9250
MPU6050 mpu(Wire);
unsigned long timer = 0;
int inital_angle = 0;
int current_angle = 0;
int target_angle = 0;

typedef enum {STRAIGHT, CORRECT, LEFT_TURN, RIGHT_TURN, STOP, MANUAL} tractorStates; //Declares states of FSM
tractorStates state = STOP;

//Declare Bluetooth module
//Pin setup for bluetooth module
  SoftwareSerial ble(rxPin,txPin); //RX , TX

void setup()
{
  // put your setup code here, to run once:
  pinMode(ENA, OUTPUT); //PWM Pin for Motor 1 Left
  pinMode(ENB, OUTPUT); //PWM Pin for Motor 2 Right
  pinMode(IN1, OUTPUT); //Motor 1 Pin 1 Left Wheel
  pinMode(IN2, OUTPUT); //Motor 1 Pin 2 Left Wheel
  pinMode(IN3, OUTPUT); //Motor 2 Pin 1 Right Wheel
  pinMode(IN4, OUTPUT); //Motor 2 Pin 2 Right Wheel
  pinMode(PUSH_BUTTON, INPUT);//Push button pin
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(IRA, INPUT);

  
  
  
  Serial.begin(9600);
  Wire.begin();
  mpu.begin();
  mpu.calcOffsets();

  //bluetooth serial begin
  ble.begin(9600);
  
  analogWrite(ENA,180);
  analogWrite(ENB,182);
}

void loop() {
  // put your main code here, to run repeatedly:

  //Reading from bluetooth
  

  mpu.update(); // gathers new X,Y,Z angles
  current_angle = mpu.getAngleZ();

 // Print outs for testing
 // Serial.println(current_angle);
 // Serial.println(target_angle);
  Serial.println(analogRead(IRA));
 
  //Calling button_state function to check button status if pressed
  button_state();

  //Main switch statement that handles all the cases
  switch (state)
  {
      case STRAIGHT: //Tractor will drive forward
      stop_button();
      emergency_stop();
      gui_stop();
      correction();
      turn();
      break;

    case STOP: //If tractor transistions to stop state tractor will stop moving
      start_button();
      break;

    case LEFT_TURN:
      left_turn();
      break;

    case RIGHT_TURN:
      right_turn();
        break;
  }
}

  //Drives Forward
  void Forward(int In1, int In2)
{
  digitalWrite(In2, HIGH);
  digitalWrite(In1, LOW);
}

//Drives backwards
void Backward(int In1, int In2)
{
  digitalWrite(In2, LOW);
  digitalWrite(In1, HIGH);
}

//Stops tractor
void Stop(int In1, int In2)
{
  digitalWrite(In1, LOW);
  digitalWrite(In2, LOW);
}

//Function for stop button. if the button state is low the tractor stops
void stop_button()
{
  Forward(IN1, IN2); //Left Wheel
  Forward(IN3, IN4); //Right Wheel

  //TODO: Integrate Gyroscope for error detection
  if (buttonState == LOW)
  {
    state = STOP;
    ble.println("Stop");
  }
}

//function for start button. If button state is high the tractor drives
void start_button()
{
  Stop(IN1, IN2); //Temp turned off for testing
  Stop(IN3, IN4);
  if (buttonState == HIGH)
  {
    state = STRAIGHT;
    inital_angle = mpu.getAngleZ();
    target_angle = inital_angle - 90;
    ble.println("Start");
    ble.println("Straight");
  }

  /*
   * If user has sent Start command through bluetooth the
   * tractor will change back to start state, set a new
   * inital angle value for staying striaght, and 
   * changing the buttonState to HIGH to ensure
   * buttonState continuity. Also writes a trip report
   * Statement
    */
  if(ble.available() > 0)
  {
    String value = ble.readString();
    if(value == "start")
    {
      state = STRAIGHT;
      buttonState = HIGH;
      ble.write("Start");
      ble.write("Straight");
    }
    if(value == "s")
    {
      buttonState = HIGH;
      mpu.update();
      inital_angle = mpu.getAngleZ();
      ble.println("Start");
      ble.println("Straight");
      state = STRAIGHT;
    }
  }
}

/* function to calculate the distance between tractor and any obstacle in the way. The US sensor measures the duration it takes a wave to travel and get reflected by an obstacle.
   In this function we then calculate the distance between the tractor and the obstacle by multiplying the wave's speed (speed of sound) by the duration. The function returns the distance
*/
int Distance()
{
  long duration = 0;
  int distance = 0;

  //clears the trigger pin
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  //sets the trigger pin to high state for 10 ms
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  //Reads the Echo pin and returns the sound wave travel timein ms
  duration = pulseIn(ECHO, HIGH);

  //Calculating the distance
  distance = duration * SOUND_SPEED;
  return distance;
}

/* Function that handles emergency stops. We store the distance value returned from Distance() function into a variable called distance. Whenever the distance returned
   is less than 30 we force the tractor to stop
*/
void emergency_stop()
{
  int distance = Distance();
  if (distance < 30)
  {
    state = STOP;
    buttonState = LOW;
    ble.write("Stop");
  }
}

/*Function that handles tractor correction. We calculate the difference between the initial angle and current angle. Whenever the difference exceeds 2 we
  we make an analog write to adjust speeds of motor for the tractor to turn slightly to the right/left depending on the correction needed.
*/
void correction()
{
  int difference = current_angle - inital_angle;

  //If-else below controls the correction of the tractor to make sure it goes straight
  if (difference >= 2)
  {
    analogWrite(ENA, 180 - offset);
    analogWrite(ENB, 181 + offset);
    Serial.print("CORRECT LEFT \n");
  }
  else
  {
    if (difference <= -2)
    {
      analogWrite(ENA, 180 + offset);
      analogWrite(ENB, 181 - offset);
      Serial.print("CORRECT RIGHT \n");
    }
    else
    {
      if (difference < 2 && difference > -2)
      {
        analogWrite(ENA, 180);
        analogWrite(ENB, 181);
        Serial.print("NEUTRAL\n");
      }
    }
  }
}


void turn()
{

  if (analogRead(IRA) >= 700)
  {
    if (turn_counter == 1 || turn_counter == 2)
    {
      state = LEFT_TURN;
      target_angle = inital_angle - 90;
      turn_counter++;
      
      //Writes turn to terminal
      ble.write("Left Turn");
    }
    else
    {
      if (turn_counter == 3 || turn_counter == 4)
      {
        state = RIGHT_TURN;
        target_angle = inital_angle + 90;

        //Writes turn to terminal
        ble.write("Right Turn");
      }
      if (turn_counter == 4)
      {
        turn_counter = 1;
      }
      else
      {
        turn_counter++;
      }
    }
  }
}

//Function that updates our push button state
void button_state()
{

  buttonNew = digitalRead(PUSH_BUTTON); //Determines buttonState on a toggle basis
  if (buttonOld == 0 && buttonNew == 1) {
    if (buttonState == 0) {
      buttonState = 1;
    }
    else {
      buttonState = 0;
    }
  }

  buttonOld = buttonNew; //End of push button toggle code
}

void left_turn()
{
  analogWrite(ENA,108);
  analogWrite(ENB,110);
  Stop(IN1, IN2);
  Forward(IN3, IN4);
  //if(target_angle-1 <= current_angle && current_angle <= target_angle+1)
  if(target_angle == current_angle)
  {
   Stop(IN1,IN2);
   Stop(IN3,IN4);
   state = STRAIGHT;
   mpu.update();
   delay(20);
   inital_angle = mpu.getAngleZ();
  }
}

void right_turn()
{
  analogWrite(ENA,108);
  analogWrite(ENB,110);
  Forward(IN1, IN2);
  Stop(IN3, IN4);
  if(target_angle == current_angle)
  {
   Stop(IN1,IN2);
   Stop(IN3,IN4);
   state = STRAIGHT;
   mpu.update();
   delay(20);
   inital_angle = mpu.getAngleZ();
  }
}

void gui_stop()
{
  String value;
  if(ble.available() > 0)
  {
    value = ble.readString();
    if(value == "stop")
    {
      state = STOP;
      buttonState = LOW;
      ble.write("Stop");
    }
  }
}
