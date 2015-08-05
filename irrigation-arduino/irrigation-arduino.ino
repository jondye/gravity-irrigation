#include <EEPROM.h>
#include <SerialCommand.h>
#include <Servo.h>
#include <Time.h>
#include <Wire.h>  
#include <DS3232RTC.h>
#include "Tap.h"

enum
{
  SERVO_PIN = 3,
  POWER_PIN = 2,
  BUTT_NOT_EMPTY_PIN = 6,
  TANK_NOT_FULL_PIN = 8,
  PUMP_SUPPLY_PIN = A1,
  SERVO_SUPPLY_PIN = A3

};

enum
{
  PARAM_TAP_OPEN_POSITION = 0,
  PARAM_TAP_CLOSE_POSITION = 1
};

SerialCommand sCmd;
Tap tap(SERVO_PIN);

void output_message(const char * message)
{
  Serial.println(message);
}

void setup_power()
{
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
  output_message(message.c_str());
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
  output_message(message.c_str());
}

void open_tap()
{
  output_message("Opening tap");
  tap.open();
}

void close_tap()
{
  output_message("Closing tap");
  tap.close();
}

void set_tap_open()
{
  const byte position = atoi(sCmd.next());
  EEPROM.update(PARAM_TAP_OPEN_POSITION, position);
  tap.open_position(position);
  String message("Tap open pos\nset to ");
  message += String(position);
  output_message(message.c_str());
}

void set_tap_close()
{
  const byte position = atoi(sCmd.next());
  EEPROM.update(PARAM_TAP_CLOSE_POSITION, position);
  tap.close_position(position);
  String message("Tap close pos\nset to ");
  message += String(position);
  output_message(message.c_str());
}

void get_time()
{
  char message[28];
  snprintf(
    message, sizeof(message), "Time is %04u-%02u-%02uT%02u:%02u:%02u", 
    year(), month(), day(), hour(), minute(), second());
  output_message(message);
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
      output_message("Time not set");
      break;
    case timeSet:
      output_message("Time set");
      break;
    case timeNeedsSync:
      output_message("Time needs sync");
      break;
  }
}

void unrecognized_command(const char *s)
{
  Serial.println("I didn't understand");
  Serial.println(s);
}

void load_params()
{
  tap.open_position(EEPROM.read(PARAM_TAP_OPEN_POSITION));
  tap.close_position(EEPROM.read(PARAM_TAP_CLOSE_POSITION));
}

enum State {
  WAITING,
  POWER_ON,
  FILLING,
  ERROR
};

State state = WAITING;
long endTime = 0;

bool timeout()
{
  return (long)(millis() - endTime) >= 0;
}

void setState(State s, long timeout)
{
  endTime = millis() + timeout;
  state = s;
}

void start_watering()
{
  output_message("Watering...\nPowering on");
  power_on();
  setState(POWER_ON, 5000);
}

void setup_serial()
{
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
  sCmd.setDefaultHandler(unrecognized_command);
}

void setup_clock()
{
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet) {
    output_message("Time not set");
  } else {
    get_time();
  }
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

  switch (state) {
    case WAITING:
      break;
    case POWER_ON:
      if (pump_supply() > 10.0 && servo_supply() > 4.0) {
        output_message("Watering...\nFilling tank");
        tap.close();
        setState(FILLING, 1800000);
      } else if (timeout()) {
        output_message("ERROR: failed\nto power on");
        setState(ERROR, 0);
      }
      break;
    case FILLING:
      break;
    case ERROR:
      power_off();
      break;
  }
}
