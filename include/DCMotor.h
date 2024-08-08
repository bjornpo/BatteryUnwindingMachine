#ifndef DCMotor_h
#define DCMotor_h

#include <Arduino.h>

class DCMotor
{
  private:
  int in1Pin;
  int in2Pin;
  int speed;
  bool coast;

  public:
  DCMotor(int in1, int in2)
  {
    in1Pin = in1;
    in2Pin = in2;
    pinMode(in1Pin, OUTPUT);
    pinMode(in2Pin, OUTPUT);
    speed = 0;
    coast = true;
    //up pwm frequency
  }

  void setSpeed(int spd, bool cst = true)
  {
    speed = constrain(spd, -255, 255);
    coast = cst;
    Serial.print("PWM_SP = ");
    Serial.println(255-abs(speed));
    if(speed > 0)
    {
      digitalWrite(in1Pin, HIGH);
      analogWrite(in2Pin, 255-abs(speed));
    }
    else if(speed < 0)
    {
      digitalWrite(in2Pin, HIGH);
      analogWrite(in1Pin, 255-abs(speed));
    }
    else if(coast)
    {
      digitalWrite(in1Pin, LOW);
      digitalWrite(in2Pin, LOW);
    }
    else
    {
      digitalWrite(in1Pin, HIGH);
      digitalWrite(in2Pin, HIGH);
    }
  }
};

#endif