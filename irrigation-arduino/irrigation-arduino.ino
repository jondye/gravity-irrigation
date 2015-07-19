#include <EEPROM.h>
#include <SerialCommand.h>
#include <Servo.h>
#include <Time.h>
#include "Tap.h"

enum
{
  PARAM_TAP_OPEN_POSITION = 0,
  PARAM_TAP_CLOSE_POSITION = 1
};

SerialCommand sCmd;
Tap tap(9);

void output_message(const char * message)
{
  Serial.println(message);
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
  const int position = atoi(sCmd.next());
  EEPROM.update(PARAM_TAP_OPEN_POSITION, position);
  tap.open_position(position);
  String message("Tap open pos\nset to ");
  message += String(position);
  output_message(message.c_str());
}

void set_tap_close()
{
  const int position = atoi(sCmd.next());
  EEPROM.update(PARAM_TAP_CLOSE_POSITION, position);
  tap.close_position(position);
  String message("Tap close pos\nset to ");
  message += String(position);
  output_message(message.c_str());
}

void unrecognized_command(const char *s)
{
  Serial.println("I didn't understand");
  Serial.println(s);
}

void setup()
{
  Serial.begin(9600);
  sCmd.addCommand("OPEN_TAP", open_tap);
  sCmd.addCommand("CLOSE_TAP", close_tap);
  sCmd.addCommand("SET_TAP_OPEN", set_tap_open);
  sCmd.addCommand("SET_TAP_CLOSE", set_tap_close);
  sCmd.setDefaultHandler(unrecognized_command);
  tap.open_position(EEPROM.read(PARAM_TAP_OPEN_POSITION));
  tap.close_position(EEPROM.read(PARAM_TAP_CLOSE_POSITION));
  Serial.println("Ready");
}

void loop()
{
  sCmd.readSerial();
  tap.tick();
}
