#include "Power.h"
#include "SerialDebug.h"
#include <Arduino.h>

Power::Power(int c, int p, int s)
  : controlPin(c)
  , pumpPin(p)
  , servoPin(s)
{
}

void Power::setup()
{
  debug("Setting up power");
  digitalWrite(controlPin, 0);
  pinMode(controlPin, OUTPUT);
}

void Power::on()
{
  digitalWrite(controlPin, 1);
}

void Power::off()
{
  digitalWrite(controlPin, 0);
}

float Power::pumpSupply()
{
  return analogRead(pumpPin) * (12.0 / 1023.0);
}

float Power::servoSupply()
{
  return analogRead(servoPin) * (5.0 / 1023.0);
}
