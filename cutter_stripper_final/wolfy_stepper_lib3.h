/*
// STEPPER MOTOR UTILITY by wolfy (WIP)
*/

#ifndef wolfy_stepper_lib_h
#define wolfy_stepper_lib_h

#include <Bounce2.h>


#define CW_DIR  0           // clockwise
#define CCW_DIR 1           // counter-clockwise
#define INTRVL  2000        // pulse interval. default:2000 microseconds

class WStepper {

  private:

    // instance pin data
    uint8_t dir_pin;                        // direction pin (OUT)
    uint8_t step_pin;                       // step pulse pin (OUT, ACTIVE HIGH)
    uint8_t sleep_pin;                      // sleep pin (OUT, ACTIVE LOW)
        
    uint8_t limit_pin;                      // limit switch pin (IN)

    // declare a button for the limit switch
    Bounce LIMIT_BUTTON;

    // running+sleeping logic
    bool is_running = false;
    bool is_sleep = false;
    bool curr_dir = CW_DIR;
        
    // step counter logic
    int step_counter;   // note: going to be DOUBLE the assigned number of steps in a step() call
    int i_step;         // incrementer var for FOR loop
    bool tog = false;   // toggle the actual output pin

    // timer logic
    unsigned long prev_time = 0;     // previous time marker: set to micros() on update()
    unsigned long curr_time;         // 

    void pollLimitSwitch() {
      if(limit_pin >= 0) {
        LIMIT_BUTTON.update();
        if(LIMIT_BUTTON.fell()) {
          //is_running = false;
          //step_counter = 0;
          //i_step = 0;
          i_step = step_counter;
          return;
        }
      }
    }
        

  public:
      
    WStepper(uint8_t dir_pin_, uint8_t step_pin_, uint8_t sleep_pin_ = -1, uint8_t limit_pin_ = -1) {
            
      // set all the pins for this instance
      this->dir_pin = dir_pin_;
      this->step_pin = step_pin_;
      this->sleep_pin = sleep_pin_;
      this->limit_pin = limit_pin_;

      // if using a limit switch, build its button
      if(limit_pin >= 0) {
        LIMIT_BUTTON = Bounce();
      }
    }

    void init() {
      // set pinMode for the 3 (or 2) OUT pins
      pinMode(dir_pin, OUTPUT);
      pinMode(step_pin, OUTPUT);
      if(sleep_pin >= 0) {
        pinMode(sleep_pin, OUTPUT);
      }
      // limit switch button init
      if(limit_pin >= 0) {
        LIMIT_BUTTON.attach(limit_pin, INPUT); // <----------------- CHECK CHECK CHECK
        LIMIT_BUTTON.interval(5);
      }

      // set OUT pins to default states
      digitalWrite(dir_pin, curr_dir); // default: clockwise
      digitalWrite(step_pin, LOW);     // default: not stepping
      if(sleep_pin >= 0) {
        digitalWrite(sleep_pin, HIGH);    // deafult: HIGH = not asleep
      }

    }


    void step(bool dir_steps, int num_steps) {

      // if asleep, wake up
      if(is_sleep == true) {
        digitalWrite(sleep_pin, HIGH);     // sleep mode off = HIGH
        is_sleep = false;
      }

      // if already running a step routine, do nothing
      if(is_running == true) {
        return;
      } else {
        digitalWrite(dir_pin, dir_steps);
        curr_dir = dir_steps;             // set direction
        i_step = 0;                       // set i to 0
        step_counter = 2 * num_steps;     // one cycle = ON and OFF, so num_steps * 2
        curr_time = micros();             // grab current time
        is_running = true;                // set is_running to true (START)
        return;
      }
    }

    void update() {

      
      
      if(is_running == true) {

        pollLimitSwitch();
        
        curr_time = micros();
                
        if(i_step < step_counter) {
          if(curr_time - prev_time >= INTRVL) {
            prev_time = curr_time;
            digitalWrite(step_pin, !tog);
            tog = !tog;
            i_step++;
          }
        } else {
          is_running = false;
          step_counter = 0;
          i_step = 0;
        }
      }
    }

    void sleep() {
      if(is_sleep == false && is_running == false) {
        digitalWrite(sleep_pin, LOW);   // LOW = sleep mode on, HIGH = sleep mode off
        is_sleep = true;
      }
    }

    bool isRunning() {
      return is_running;
    }

    bool currentDirection() {
      return curr_dir;
    }

    void stepAndWait(bool dir_steps, int num_steps) {
      this->step(dir_steps, num_steps);
      while(is_running) {
        this->update();
      }
    }
    
    void stepWaitSleep(bool dir_steps, int num_steps) {
      this->step(dir_steps, num_steps);
      while(is_running) {
        this->update();
      }
      this->sleep();
    }



};

#endif
