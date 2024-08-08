#include <Arduino.h>
#include <Servo.h>
#include <AccelStepper.h>
#include <string.h>
#include <stdlib.h>
#include "DCMotor.h"

#define READ_BUFFER_SIZE 25

//Servos
#define SERVO 1
#define STEPPER 2
#define DC_MOTOR 3
#define WAIT 4

#define NumOfServos 4
const int ServoPins[] = {A0, A1, A2, A3};
Servo myServos[NumOfServos];
const int srvMapmin[] = {180, 30, 30, 80};
const int srvMapmax[] = {30, 180, 180, 140};

//Steppers
#define NUM_OF_STEPPERS 4
const int stepperScalingFactor = 16; //scale full-scale stepperRange from 0-100
const int defaultSpeed = 1200;
const int defaultAccel = 4000;
bool StepperDirection[] = {false, true, false, false};
bool stepperContantSpeedMode[NUM_OF_STEPPERS];
AccelStepper mySteppers[] = 
{
  AccelStepper(AccelStepper::DRIVER, 4, 5),
  AccelStepper(AccelStepper::DRIVER, 6, 7),
  AccelStepper(AccelStepper::DRIVER, 8, 9),
  AccelStepper(AccelStepper::DRIVER, 10, 12)
};

//Motors
#define NUM_OF_MOTORS 1
DCMotor myDCMotors[] = {DCMotor(3,11)};
 
// function declarations:
void ReadBytes();
void parseCommand(char *input);

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  //Steppers
  for(int i = 0; i<NUM_OF_STEPPERS; i++)
  {
    mySteppers[i].setMaxSpeed(defaultSpeed);
    mySteppers[i].setAcceleration(defaultAccel);
    mySteppers[i].setEnablePin(2);
    mySteppers[i].setPinsInverted(StepperDirection[i], false, true);
    stepperContantSpeedMode[i] = false;
  }

  //Motors
  //TCCR2B = TCCR2B & (B11111000 | B00000001); // for PWM frequency of 31372.55 Hz Not working
  for(int i = 0; i<NUM_OF_MOTORS; i++)
  {
    myDCMotors[i].setSpeed(0, true);
  }
}//End setup

void loop() {
  // put your main code here, to run repeatedly:
  ReadBytes();
  for(int i=0; i<NUM_OF_STEPPERS; i++)
  {
    if(stepperContantSpeedMode[i])
    {
      mySteppers[i].runSpeed();
    }
    else
    {
      mySteppers[i].run();
    }
  }
} //End loop


//FUNCTIONS
void ReadBytes()
{
  static char buffer[READ_BUFFER_SIZE];
  static int index = 0;
  if(Serial.available())
  {
    int inChar = Serial.read();
    Serial.print(char(inChar)); //debug
    if(index > READ_BUFFER_SIZE-1)
    {
      index = 0;
      Serial.println("ERROR Buffer overflow");
    }
    if(inChar == 0xA || inChar == '#')
    {
      buffer[index] = 0; //add nullchar
      Serial.print("received following string: ");
      Serial.println(buffer);
      index = 0;
      parseCommand(buffer);
    }
    else
    {
      buffer[index] = inChar;
      index++;
    }
  }
} //End readBytes


struct parameter 
  {
    char name;
    int value;
    bool exists;
  };

  #define TYPE 0
  #define ID 1
  #define POS 2
  #define SPEED 3
  #define DISABLE 4
  #define ACCEL 5
  #define SLEEP 6

void parseCommand(char *input)
{
  struct parameter parameters[] = {
    {'T',-999,false}, //Type
    {'I',-999,false}, //ID
    {'P',-999,false}, //Pos
    {'S',-999,false}, //speed
    {'D',-999,false}, //Disable
    {'A',-999,false}, //accel
    {'W',-999,false} //sleep
    };

  char *ptr;
  for (unsigned int i = 0; i < sizeof(parameters) / sizeof(parameters[0]); i++) {
        ptr = strchr(input, parameters[i].name);
        if(ptr != NULL)
        {
          parameters[i].value = atoi(ptr+1);
          parameters[i].exists = true;
        }
    }

  if(parameters[TYPE].exists)
  {
    Serial.print("\nType = ");
    switch (parameters[TYPE].value)
    {
    case SERVO:
    Serial.print("Servo");
      if(parameters[POS].exists && parameters[ID].exists)
      {
        int sp = constrain(parameters[POS].value-1, 0, 100);
        sp = map(sp, 0, 100, srvMapmin[parameters[ID].value-1], srvMapmax[parameters[ID].value-1]);
        Serial.print("\tRawPosition = ");
        Serial.println(sp);
        myServos[parameters[ID].value-1].attach(ServoPins[parameters[ID].value-1]);
        myServos[parameters[ID].value-1].write(sp);
      }
      else if(parameters[DISABLE].exists)
      {
        if(parameters[ID].exists)
        {
          myServos[parameters[ID].value-1].detach();
        }
        else
        {
          for(int i = 0; i<NumOfServos; i++)
          {
            myServos[i].detach();
          }
        }
      }
      break;

    case STEPPER:
    Serial.print("Stepper");
    if(parameters[SPEED].exists) //Update speed
    {
      if(parameters[ID].exists)
      {
         mySteppers[parameters[ID].value-1].setMaxSpeed(parameters[SPEED].value);
        if(!parameters[POS].exists) //speed control mode
        {
          stepperContantSpeedMode[parameters[ID].value-1] = true;
          mySteppers[parameters[ID].value-1].setSpeed(parameters[SPEED].value);
        }
      }
      else
      {
        for(int i=0; i<NUM_OF_STEPPERS; i++)
        {
          mySteppers[i].setMaxSpeed(parameters[SPEED].value);
        }
      }
    }
    if(parameters[ACCEL].exists) //Update acceleration
    {
      if(parameters[ID].exists)
      {
         mySteppers[parameters[ID].value-1].setAcceleration(parameters[ACCEL].value);
      }
      else
      {
        for(int i=0; i<NUM_OF_STEPPERS; i++)
        {
          mySteppers[i].setAcceleration(parameters[ACCEL].value);
        }
      }
    }

    if(parameters[POS].exists && parameters[ID].exists) //move stepper
      {
        Serial.print("\tPosition = ");
        Serial.println(parameters[POS].value);
        stepperContantSpeedMode[parameters[ID].value-1] = false;
        mySteppers[parameters[ID].value-1].enableOutputs();
        mySteppers[parameters[ID].value-1].moveTo(parameters[POS].value*stepperScalingFactor);
      }

    if(parameters[DISABLE].exists)
    {
      Serial.println("Disable Steppers");
      mySteppers[0].disableOutputs();
    }

    break;

    case DC_MOTOR:
    Serial.print("DC-motor");
    if (parameters[DISABLE].exists)
    {
      myDCMotors[parameters[ID].value - 1].setSpeed(0, true); //Disable motor breaking
    }
    else if (parameters[SPEED].exists)
    {
      myDCMotors[parameters[ID].value - 1].setSpeed(parameters[SPEED].value, false);
    }
    else
    {
      Serial.println("No Arguments");
    }
    break;

    case WAIT:
    if(parameters[SLEEP].exists)
    {
      int sleep = constrain(parameters[SLEEP].value, 0, 10000);
      Serial.print("Waits for ");
      Serial.print(sleep);
      Serial.println(" ms");
      delay(sleep);
    }
    break;

    default:
      Serial.println("\nERROR Unknown type");
      break;
    }
  }
} //End parseCommand