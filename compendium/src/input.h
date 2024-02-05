#ifndef INPUT_H
#define INPUT_H

#include "vector.h"

/* 
 * Input is a virtualization of the controller inputs.
 * You access buttons via Input::up, down, left, right, space...
 * But you have to register new buttons in the buttons array
 * so that their just-frame states (up, down) can be cleared
 * at the ends of frames. 
 */
struct Input {
  struct Button {
    bool up = false;
    bool pressed = false;
    bool held = false;
  };

  Button *buttons[6];
  Button up;
  Button down;
  Button left;
  Button right;
  Button space;
  Button lmb;

  bool any_was_pressed;

  v2d cursor;

  Input() {
    buttons[0] = &up;
    buttons[1] = &down;
    buttons[2] = &left;
    buttons[3] = &right;
    buttons[4] = &space;
    buttons[5] = &lmb;
    any_was_pressed = false;
  }

  void AtFrameEnd() {
    /* Clear whatever button was pressed */
    any_was_pressed = false;
    /* Clear the transient states */
    for (Button *button : buttons) {
      if (button->pressed) any_was_pressed = true;
      button->pressed = false;
      button->up = false;
    }
  }
};

#endif