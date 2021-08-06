#include "oled_util.h"
#include "wolfy_stepper_lib3.h"
//#include <Bounce2.h>      // button debounce library

// define button input pins
#define SEL_PIN A1
#define  UP_PIN A0
#define DWN_PIN A3
#define  GO_PIN A2

#define CUT_STEPS 140           // rotation of cutter/stripper motor (140/200)
#define MOVE_STEPS 325          // s_motor distance between cut and strip locations
#define MM_CONVERT 0.15382222   // steps to mm factor. steps = mm / MM_CONVERT
#define STRIP_NUDGE 15          // e_motor how far to pull back wire to get a better trip

// build button instances
// SELECT, UP, DOWN, GO
Bounce SEL_BUTTON = Bounce();
Bounce UP_BUTTON  = Bounce();
Bounce DWN_BUTTON = Bounce();
Bounce GO_BUTTON  = Bounce();

// build steper object instances
// args: (direction pin, step pn, sleep pin, limit switch pin = -1)
WStepper c_motor = WStepper(8, 9, 10);
WStepper e_motor = WStepper(5, 6, 7);
WStepper s_motor = WStepper(2, 3, 4, 13);

void setup() {

  //Serial.begin(9600);

  c_motor.init();
  e_motor.init();
  s_motor.init();

  // cheap chinese linear actuator motor pulls too much power when idling, so:
  s_motor.sleep();

  // init buttons with polling interval of 5 milliseconds
  SEL_BUTTON.attach(SEL_PIN, INPUT_PULLUP);
  SEL_BUTTON.interval(5);
  UP_BUTTON.attach(UP_PIN, INPUT_PULLUP);
  UP_BUTTON.interval(5);
  DWN_BUTTON.attach(DWN_PIN, INPUT_PULLUP);
  DWN_BUTTON.interval(5);
  GO_BUTTON.attach(GO_PIN, INPUT_PULLUP);
  GO_BUTTON.interval(5);

  // init screen
  screen.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  // clear buffer
  screen.clearDisplay();

  // draw wolfy logo
  screen.drawBitmap(47, 15, WOLF_BMP, WOLF_W, WOLF_H, 1);
  
  // draw buffer on screen
  screen.display();

  // wait 2.5 seconds
  delay(2500);

}

void loop() {

  // main loop continually checks overall
  // machine state and calls corresponding
  // functions. that is all it does.
  switch(machine_state) {
    case MAIN :
      mainButtonStuff();
      drawMainMenu();
      break;
    case CUT :
      cutButtonStuff();
      drawCutMenu();
      break;
    case RUNNING :
      // run the machine
      // display running screen(s)
      //runningButtonTest();
      fullRoutineTest();
      break;
  }
  

}

// MAIN MENU
void mainButtonStuff() {

  // check all buttons
  SEL_BUTTON.update();
  UP_BUTTON.update();
  DWN_BUTTON.update();
  GO_BUTTON.update();
  
  // toggle L, M, R states
  if(SEL_BUTTON.fell()) {
    switch(main_state) {
      case L :
        main_state = M;
        break;
      case M :
        main_state = R;
        break;
      case R :
        main_state = L;
        break;
    }
  }

  // ++ whichever val is selected, with wrapping
  if(UP_BUTTON.fell()) {
    switch(main_state) {
      case L :
        L_val++;
        if(L_val > 30) {
          L_val = 1;
        }
        break;
      case M :
        M_val += 10;
        if(M_val >= 250) {
          M_val = 10;
        }
        break;
      case R :
        R_val++;
        if(R_val > 30) {
          R_val = 1;
        }
        break;
    }
  }

  // -- whichever val is selected, with wrapping
  if(DWN_BUTTON.fell()) {
    switch(main_state) {
      case L :
        L_val--;
        if(L_val < 1) {
          L_val = 30;
        }
        break;
      case M :
        M_val -= 10;
        if(M_val < 10) {
          M_val = 240;
        }
        break;
      case R :
        R_val--;
        if(R_val < 1) {
          R_val = 30;
        }
        break;
    }
  }

  // from MAIN MENU, the GO button takes us to the CUT MENU
  if(GO_BUTTON.fell()) {
    machine_state = CUT;
  }
}



void cutButtonStuff() {
  
  // check all buttons
  SEL_BUTTON.update();
  UP_BUTTON.update();
  DWN_BUTTON.update();
  GO_BUTTON.update();

  // SELECT button toggles RUN / BACK
  if(SEL_BUTTON.fell()) {
    cut_state = !cut_state;
  }

  // ++ number of wires to be cut (with wrapping)
  if(UP_BUTTON.fell()) {
    N_val++;
    if(N_val >= 251) {
      N_val = 1;
    }
  }

  // -- number of wires to be cut (with wrapping)
  if(DWN_BUTTON.fell()) {
    N_val--;
    if(N_val < 1) {
      N_val = 250;
    }
  }

  // GO button either takes us back to MAIN MENU (default), or
  // begins the actual cutting by taking overall machine state to RUNNING
  if(GO_BUTTON.fell()) {
    switch(cut_state) {
      case RUN :
        //
        machine_state = RUNNING;
        //
        break;
      case BACK :
        machine_state = MAIN;
        break;
    }
  }
}
/*
void runningButtonTest() {

 for(int i=0; i < N_val; i++) {
    extrudeMillimeters(M_val);
    cutWire();
  }

  s_motor.stepWaitSleep(CW_DIR, MOVE_STEPS);
  delay(1000);
  s_motor.stepWaitSleep(CCW_DIR, MOVE_STEPS);
  delay(1000);

  
  
  // return to CUT state when done
  machine_state = CUT;
  
}
*/
void cutWire() {
  c_motor.stepAndWait(CCW_DIR, CUT_STEPS);
  c_motor.stepWaitSleep(CW_DIR, CUT_STEPS);
  delay(10);
}

void stripWire() {
  c_motor.stepAndWait(CCW_DIR, CUT_STEPS);
  e_motor.stepAndWait(CW_DIR, STRIP_NUDGE);
  delay(10);
  e_motor.stepAndWait(CCW_DIR, STRIP_NUDGE);
  c_motor.stepWaitSleep(CW_DIR, CUT_STEPS);
  delay(10);
}

void extrudeSteps(int es) {
  e_motor.stepAndWait(CCW_DIR, es);
}

void extrudeMillimeters(int em) {
  float x = em / MM_CONVERT;
  extrudeSteps(int(x));
}

void fullRoutineTest() {

  for(int i=0; i < N_val; i++) {
    // a. cut any excess material off first
    cutWire();

    // b. move s_motor to strip position
    s_motor.stepWaitSleep(CW_DIR, MOVE_STEPS);
    delay(50);

    // c. extrude L_val
    extrudeMillimeters(L_val);

    // d. strip L_val
    stripWire();

    // e. extrude M_val
    extrudeMillimeters(M_val);

    // f. strip R_val
    stripWire();

    // g. move s_motor back to cut position
    s_motor.stepWaitSleep(CCW_DIR, MOVE_STEPS);
    delay(50);

    // h. extrude R_val
    extrudeMillimeters(R_val);

    // i. cut wire. END
    cutWire();
  }

  // return to CUT state when done
  machine_state = CUT;
}
