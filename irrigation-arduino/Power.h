#ifndef POWER_H
#define POWER_H

class Power
{
public:
  Power(int controlPin, int pumpSupplyPin, int servoSupplyPin);
  void setup();
  void on();
  void off();
  float pumpSupply();
  float servoSupply();

private:
  const int controlPin;
  const int pumpPin;
  const int servoPin;
};

#endif
