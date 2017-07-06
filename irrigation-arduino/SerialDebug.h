#ifndef SERIALDEBUG_H
#define SERIALDEBUG_H

#include <Arduino.h>

inline void debug(const char *message)
{
  Serial.println(message);
}

typedef void (*Callback)();
struct Command
{
  const char * message;
  Callback function;
};

void setup_serial(const Command * commands);
void serial_tick();
int next_param();


#endif
