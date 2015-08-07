#ifndef PARAMETERS_H
#define PARAMETERS_H

class Parameters
{
private:
  enum
  {
    PARAM_TAP_OPEN_POSITION = 0,
    PARAM_TAP_CLOSE_POSITION = 1,
    PARAM_ALARM_HOURS = 2,
    PARAM_ALARM_MINUTES = 3,
    PARAM_FILLING_TIME = 4,
    PARAM_EMPTYING_TIME = 6,
    PARAM_WATERING_TIME = 8
  };
  static byte read(int position) { return EEPROM.read(position); }
  static void write(int position, byte value) { EEPROM.update(position, value); }
  static int readUInt(int position) { unsigned int v = 0; EEPROM.get(position, v); return v; }
  static void write(int position, unsigned int value) { EEPROM.put(position, value); }

public:
  static byte tapOpenPosition() { return read(PARAM_TAP_OPEN_POSITION); }
  static void tapOpenPosition(byte p) { write(PARAM_TAP_OPEN_POSITION, p); }
  static byte tapClosePosition() { return read(PARAM_TAP_CLOSE_POSITION); }
  static void tapClosePosition(byte p) { write(PARAM_TAP_CLOSE_POSITION, p); }
  static byte alarmHours() { return read(PARAM_ALARM_HOURS); }
  static byte alarmMinutes() { return read(PARAM_ALARM_MINUTES); }
  static void alarmTime(byte hours, byte minutes) {
    write(PARAM_ALARM_HOURS, hours);
    write(PARAM_ALARM_MINUTES, minutes);
  }
  static unsigned int fillingTime() { return readUInt(PARAM_FILLING_TIME); }
  static void fillingTime(unsigned int seconds) { write(PARAM_FILLING_TIME, seconds); }
  static unsigned int emptyingTime() { return readUInt(PARAM_EMPTYING_TIME); }
  static void emptyingTime(unsigned int seconds) { write(PARAM_EMPTYING_TIME, seconds); }
  static unsigned int wateringTime() { return readUInt(PARAM_WATERING_TIME); }
  static void wateringTime(unsigned int seconds) { write(PARAM_WATERING_TIME, seconds); }

};

#endif
