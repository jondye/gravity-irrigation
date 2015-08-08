#include <EEPROM.h>
#include <Servo.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Wire.h>  
#include <DS3232RTC.h>
#include "Tap.h"
#include "Parameters.h"
#include "SerialDebug.h"
#include "Power.h"

enum PinFunctions
{
  POWER_PIN = 2,
  SERVO_PIN = 3,
  ENCODER_PIN_A = 4,
  LCD_LED_PIN = 5,
  BUTT_NOT_EMPTY_PIN = 6,
  ENCODER_PIN_B = 7,
  TANK_NOT_FULL_PIN = 8,
  BUTTON_PIN_A = 12,
  BUTTON_PIN_B = 13,
  PUMP_SUPPLY_PIN = A1,
  SERVO_SUPPLY_PIN = A3
};

Tap tap(SERVO_PIN);
Power power(POWER_PIN, PUMP_SUPPLY_PIN, SERVO_SUPPLY_PIN);

void power_status()
{
  String message("12v: ");
  message += power.pumpSupply();
  message += " 5v: ";
  message += power.servoSupply();
  debug(message.c_str());
}

void setup_sensors()
{
  debug("Setting sensors");
  pinMode(BUTT_NOT_EMPTY_PIN, INPUT);
  pinMode(TANK_NOT_FULL_PIN, INPUT);
}

void sensor_state()
{
  String message("BUTT: ");
  if (power.pumpSupply() < 10) {
    message += "  unknown";
  } else if (digitalRead(BUTT_NOT_EMPTY_PIN)) {
    message += "not empty";
  } else {
    message += "    empty";
  }
  message += " TANK: ";
  if (power.pumpSupply() < 10) {
    message += " unknown";
  } else if (digitalRead(TANK_NOT_FULL_PIN)) {
    message += "not full";
  } else {
    message += "    full";
  }
  debug(message.c_str());
}

void set_tap_open()
{
  const byte position = next_param();
  Parameters::tapOpenPosition(position);
  tap.open_position(position);
  String message("Tap open position set to ");
  message += String(position);
  debug(message.c_str());
}

void set_tap_close()
{
  const byte position = next_param();
  Parameters::tapClosePosition(position);
  tap.close_position(position);
  String message("Tap close position set to ");
  message += String(position);
  debug(message.c_str());
}

void get_time()
{
  char message[28];
  snprintf(
    message, sizeof(message), "Time is %04u-%02u-%02uT%02u:%02u:%02u", 
    year(), month(), day(), hour(), minute(), second());
  debug(message);
}

void set_time()
{
  TimeElements time = {};
  time.Year = next_param() - 1970;
  time.Month = next_param();
  time.Day = next_param();
  time.Hour = next_param();
  time.Minute = next_param();
  time.Second = next_param();
  const time_t tt = makeTime(time);
  RTC.set(tt);
  setTime(tt);
}

void time_status()
{
  switch(timeStatus())
  {
    case timeNotSet:
      debug("Time not set");
      break;
    case timeSet:
      debug("Time set");
      break;
    case timeNeedsSync:
      debug("Time needs sync");
      break;
  }
}

void setup_clock()
{
  debug("Setting up RTC");
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet) {
    debug("Time not set");
  } else {
    get_time();
  }
}

class Context;

class State
{
public:
  virtual void enter(Context &) {}
  virtual void exit(Context &) {}
  virtual void timeout(Context &) {}
  virtual void alarm(Context &) {}
  virtual void powerGood(Context &) {}
  virtual void buttEmpty(Context &) {}
  virtual void tankFull(Context &) {}
};

class Context
{
  State * state;
  unsigned long endTime;

public:
  Context(State &start) : state(&start), endTime(0) {}
  void tick()
  {
    if ((long)(endTime - millis()) < 0)
    {
      state->timeout(*this);
    }
  }

  void setState(State &s, unsigned long timeout)
  {
    endTime = millis() + timeout;
    state->exit(*this);
    state = &s;
    state->enter(*this);
  }

  void alarm() { state->alarm(*this); }
  void powerGood() { state->powerGood(*this); }
  void buttEmpty() { state->buttEmpty(*this); }
  void tankFull() { state->tankFull(*this); }
};

class Waiting : public State
{
  void alarm(Context &);
} waiting;

class PoweringOn : public State
{
  void enter(Context &);
  void powerGood(Context &);
  void timeout(Context &);
} poweringOn;

class Filling : public State
{
  void enter(Context &);
  void buttEmpty(Context &context);
  void tankFull(Context &context);
  void timeout(Context &constext);
} filling;

class Watering : public State
{
  void enter(Context &);
  void timeout(Context &);
  void exit(Context &);
} watering;

class Error : public State
{
} error;

void Waiting::alarm(Context &context)
{
  context.setState(poweringOn, 5000);
}

void PoweringOn::enter(Context &context)
{
  debug("PoweringOn::enter");
  power.on();
}

void PoweringOn::powerGood(Context &context)
{
  debug("PoweringOn::powerGood");
  // TODO this should be the filling time
  context.setState(filling, 10000);
}

void PoweringOn::timeout(Context &context)
{
  debug("PoweringOn::timeout");
  power.off();
  debug("Failed power on");
  context.setState(error, 0);
}

void Filling::enter(Context &context)
{
  debug("Filling::enter");
  tap.close();
}

void Filling::buttEmpty(Context &context)
{
  debug("Filling::buttEmpty");
  // TODO this should be the emptying time
  context.setState(watering, 10000);
}

void Filling::tankFull(Context &context)
{
  debug("Filling::tankFull");
  // TODO this should be the watering time!
  context.setState(watering, 10000);
}

void Filling::timeout(Context &context)
{
  debug("Filling::timeout");
  // TODO this should be the emptying time
  context.setState(watering, 10000);
}

void Watering::enter(Context &context)
{
  debug("Watering::enter");
  tap.open();
}

void Watering::timeout(Context &context)
{
  debug("Watering::timeout");
  context.setState(waiting, 0);
}

void Watering::exit(Context &context)
{
  debug("Watering::exit");
  power.off();
}

Context context(waiting);

void start_watering()
{
  context.alarm();
}

void load_params()
{
  debug("Loading parameters");
  tap.open_position(Parameters::tapOpenPosition());
  tap.close_position(Parameters::tapClosePosition());
  Alarm.alarmRepeat(
      Parameters::alarmHours(),
      Parameters::alarmMinutes(),
      0,
      &start_watering);
}

void set_alarm()
{
  const byte hours = next_param();
  const byte minutes = next_param();
  Parameters::alarmTime(hours, minutes);
  Alarm.alarmRepeat(hours, minutes, 0, &start_watering);
  char message[20];
  snprintf(message, sizeof(message), "Alaram set to %2u:%02u", (unsigned int)hours, (unsigned int)minutes);
  debug(message);
}


void open_tap() { tap.open(); }
void close_tap() { tap.close(); }
void power_on() { power.on(); }
void power_off() { power.off(); }

void setup()
{
  Command commands[] = 
  {
    {"opentap", &open_tap},
    {"closetap", &close_tap},
    {"settapopen", &set_tap_open},
    {"settapclose", &set_tap_close},
    {"settime", &set_time},
    {"gettime", &get_time},
    {"timestatus", &time_status},
    {"poweron", &power_on},
    {"poweroff", &power_off},
    {"powerstatus", &power_status},
    {"sensors", &sensor_state},
    {"water", &start_watering},
    {"setalarm", &set_alarm},
    {0, 0}
  };
  setup_serial(commands);
  setup_clock();
  power.setup();
  load_params();
  setup_sensors();
}

void loop()
{
  serial_tick();
  tap.tick();
  context.tick();

  if (power.pumpSupply() > 10.0 && power.servoSupply() > 4.0) {
    context.powerGood();
  }
  if (!digitalRead(BUTT_NOT_EMPTY_PIN)) {
    context.buttEmpty();
  }
  if (!digitalRead(TANK_NOT_FULL_PIN)) {
    context.tankFull();
  }

  Alarm.delay(10);
}
