#ifndef TAP_H
#define TAP_H

class Tap
{
private:
  static const int HOLD_TIME = 1000;

public:
  Tap(byte pin)
    : state_(Stationary)
    , servo_()
    , pin_(pin)
    , open_position_(0)
    , close_position_(180)
    , endTime_(0)
  {
  }

  void open()
  {
    endTime_ = now() + HOLD_TIME;
    servo_.attach(pin_);
    servo_.write(open_position_);
    state_ = Opening;
  }

  void close()
  {
    endTime_ = now() + HOLD_TIME;
    servo_.attach(pin_);
    servo_.write(close_position_);
    state_ = Closing;
  }

  void close_position(byte p)
  {
    close_position_ = p;
  }

  void open_position(byte p)
  {
    open_position_ = p;
  }

  void tick()
  {
    switch(state_) {
      case Stationary:
        break;
      case Opening:
        if (now() >= endTime_) {
          servo_.detach();
          state_ = Stationary;
        }
        break;
      case Closing:
        if (now() >= endTime_) {
          servo_.detach();
          state_ = Stationary;
        }
        break;
    }
  }

private:
  enum State {Stationary, Opening, Closing};
  State state_;
  Servo servo_;
  const byte pin_;
  byte open_position_;
  byte close_position_;
  time_t endTime_;
};

#endif
