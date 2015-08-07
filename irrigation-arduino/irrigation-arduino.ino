#include <EEPROM.h>
#include <SerialCommand.h>
#include <Servo.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Wire.h>  
#include <DS3232RTC.h>
#include "Tap.h"
#include "Parameters.h"

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

SerialCommand sCmd;
Tap tap(SERVO_PIN);

void debug(const char *message)
{
  Serial.print(message);
}

void setup_power()
{
  debug("Setting up power");
  digitalWrite(POWER_PIN, 0);
  pinMode(POWER_PIN, OUTPUT);
}

float pump_supply()
{
  return analogRead(PUMP_SUPPLY_PIN) * (12.0 / 1023.0);
}

float servo_supply()
{
  return analogRead(SERVO_SUPPLY_PIN) * (5.0 / 1023.0);
}

void power_status()
{
  String message("12v: ");
  message += pump_supply();
  message += " 5v: ";
  message += servo_supply();
  debug(message.c_str());
}

void power_on()
{
  digitalWrite(POWER_PIN, 1);
}

void power_off()
{
  digitalWrite(POWER_PIN, 0);
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
  if (pump_supply() < 10) {
    message += "  unknown";
  } else if (digitalRead(BUTT_NOT_EMPTY_PIN)) {
    message += "not empty";
  } else {
    message += "    empty";
  }
  message += " TANK: ";
  if (pump_supply() < 10) {
    message += " unknown";
  } else if (digitalRead(TANK_NOT_FULL_PIN)) {
    message += "not full";
  } else {
    message += "    full";
  }
  debug(message.c_str());
}

void open_tap()
{
  debug("Opening tap");
  tap.open();
}

void close_tap()
{
  debug("Closing tap");
  tap.close();
}

void set_tap_open()
{
  const byte position = atoi(sCmd.next());
  Parameters::tapOpenPosition(position);
  tap.open_position(position);
  String message("Tap open position set to ");
  message += String(position);
  debug(message.c_str());
}

void set_tap_close()
{
  const byte position = atoi(sCmd.next());
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
  time.Year = atoi(sCmd.next()) - 1970;
  time.Month = atoi(sCmd.next());
  time.Day = atoi(sCmd.next());
  time.Hour = atoi(sCmd.next());
  time.Minute = atoi(sCmd.next());
  time.Second = atoi(sCmd.next());
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

void unrecognized_command(const char *s)
{
  Serial.println("I didn't understand");
  Serial.println(s);
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
  power_on();
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
  power_off();
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
  power_off();
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
  const byte hours = atoi(sCmd.next());
  const byte minutes = atoi(sCmd.next());
  Parameters::alarmTime(hours, minutes);
  Alarm.alarmRepeat(hours, minutes, 0, &start_watering);
  char message[20];
  snprintf(message, sizeof(message), "Alaram set to %2u:%02u", (unsigned int)hours, (unsigned int)minutes);
  debug(message);
}

void setup_serial()
{
  debug("Setting up serial");
  sCmd.addCommand("opentap", open_tap);
  sCmd.addCommand("closetap", close_tap);
  sCmd.addCommand("settapopen", set_tap_open);
  sCmd.addCommand("settapclose", set_tap_close);
  sCmd.addCommand("settime", set_time);
  sCmd.addCommand("gettime", get_time);
  sCmd.addCommand("timestatus", time_status);
  sCmd.addCommand("poweron", power_on);
  sCmd.addCommand("poweroff", power_off);
  sCmd.addCommand("powerstatus", power_status);
  sCmd.addCommand("sensors", sensor_state);
  sCmd.addCommand("water", start_watering);
  sCmd.addCommand("setalarm", set_alarm);
  sCmd.setDefaultHandler(unrecognized_command);
}

void setup()
{
  Serial.begin(9600);

  setup_clock();
  setup_serial();
  load_params();
  setup_power();
  setup_sensors();
}

void loop()
{
  sCmd.readSerial();
  tap.tick();
  context.tick();

  if (pump_supply() > 10.0 && servo_supply() > 4.0) {
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
