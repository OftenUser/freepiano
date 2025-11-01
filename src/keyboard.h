#pragma once
#include "midi.h"

// Keymap enum callback
struct keymap_enum_callback {
  virtual void operator () (const char *value) = 0;
};

// Initialize keyboard
int keyboard_init();

// Shutdown keyboard system
void keyboard_shutdown();

// Enable or disable keyboard
void keyboard_enable(bool enable);

// Enum keymap
void keyboard_enum_keymap(keymap_enum_callback &callback);

// Reset keyboard
void keyboard_reset();

// Key event
void keyboard_send_event(int code, int keydown);

// Keyboard event
void keyboard_update(double time_elapsed);

// Get keyboard status
byte keyboard_get_status(byte code);
