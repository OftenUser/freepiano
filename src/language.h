#pragma once

#define FP_LANG_AUTO       -1
#define FP_LANG_ENGLISH     0
#define FP_LANG_SCHINESE    1
#define FP_LANG_COUNT       2

// Language string def
#define STR_ENGLISH(id, str)  id,
#define STR_SCHINESE(id, str) 
enum StringIDs {
  FP_IDS_EMPTY,
#include "language_strdef.h"
  FP_IDS_COUNT,
};
#undef STR_ENGLISH
#undef STR_SCHINESE

// Initialize languages
void lang_init();

// Select current language
void lang_set_current(int languageid);

// Get current language
int lang_get_current();

// Load str
const char* lang_load_string(uint uid);

// Load lang string array
const char** lang_load_string_array(uint uid);

// Format lang str
int lang_format_string(char *buff, size_t size, uint strid, ...);

// Open text
int lang_text_open(uint textid);

// Readline
int lang_text_readline(char *buff, size_t size);

// Close text
void lang_text_close();

// Set last error
void lang_set_last_error(const char *format, ...);

// Set last error
void lang_set_last_error(uint id, ...);

// Get last error
const char * lang_get_last_error();

// Localize dialog

void lang_localize_dialog(HWND hwnd);
