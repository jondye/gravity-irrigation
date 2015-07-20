#include <EEPROM.h>
#include <SerialCommand.h>
#include <Servo.h>
#include <Time.h>
#include "Tap.h"

enum
{
  SERVO_PIN = 9,
  POWER_PIN = 7,
  PUMP_SUPPLY_PIN = A0,
  SERVO_SUPPLY_PIN = A1

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

void power_on()
{
  digitalWrite(POWER_PIN, 1);
}

void power_off()
{
  digitalWrite(POWER_PIN, 0);
}

float pump_supply()
{
  return analogRead(PUMP_SUPPLY_PIN) * (12.0 / 1023.0);
}

float servo_supply()
{
  return analogRead(SERVO_SUPPLY_PIN) * (5.0 / 1023.0);
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

void set_time()
{
  const int years = atoi(sCmd.next());
  const int months = atoi(sCmd.next());
  const int days = atoi(sCmd.next());
  const int hours = atoi(sCmd.next());
  const int minutes = atoi(sCmd.next());
  const int seconds = atoi(sCmd.next());
  String message("Setting time to\n");
  message += String(years) + String("-") + String(months) + String("-") + String(days);
  message += String(" ");
  message += String(hours) + String(":") + String(minutes) + String(":") + String(seconds);
  output_message(message.c_str());
  setTime(hours, minutes, seconds, days, months, years);
}

void get_time()
{
  String message("Time now is\n");
  message += year() + String("-") + month() + String("-") + day();
  message += " ";
  message += hour() + String(":") + minute() + String(":") + second();
  output_message(message.c_str());
}

void power_status()
{
  Serial.print("12v: ");
  Serial.println(pump_supply());
}

void unrecognized_command(const char *s)
{
  Serial.println("I didn't understand");
  Serial.println(s);
}

void setup_serial()
{
  sCmd.addCommand("opentap", open_tap);
  sCmd.addCommand("closetap", close_tap);
  sCmd.addCommand("settapopen", set_tap_open);
  sCmd.addCommand("settapclose", set_tap_close);
  sCmd.addCommand("settime", set_time);
  sCmd.addCommand("gettime", get_time);
  sCmd.addCommand("poweron", power_on);
  sCmd.addCommand("poweroff", power_off);
  sCmd.addCommand("powerstatus", power_status);
  sCmd.setDefaultHandler(unrecognized_command);
}

void load_params()
{
  tap.open_position(EEPROM.read(PARAM_TAP_OPEN_POSITION));
  tap.close_position(EEPROM.read(PARAM_TAP_CLOSE_POSITION));
}

void setup()
{
  Serial.begin(9600);

  setup_serial();
  load_params();
  setup_power();

  Serial.println("Please set the time");
}

void loop()
{
  sCmd.readSerial();
  tap.tick();
}
