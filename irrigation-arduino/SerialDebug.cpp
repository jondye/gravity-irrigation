#include "SerialDebug.h"
#include <SerialCommand.h>

namespace
{
  static SerialCommand sCmd;

  static void unrecognized_command(const char *s)
  {
    Serial.println("I didn't understand");
    Serial.println(s);
  }
}

void setup_serial(const Command * commands)
{
  debug("Setting up serial");
  Serial.begin(9600);
  for (const Command *command = commands; command->function != NULL; ++command) {
    sCmd.addCommand(command->message, command->function);
  }
  sCmd.setDefaultHandler(unrecognized_command);
}

void serial_tick()
{
  sCmd.readSerial();
}

int next_param()
{
  return atoi(sCmd.next());
}
