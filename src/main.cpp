#include "pch.h"
#include "keyboard.h"
#include "display.h"
#include "config.h"
#include "gui.h"
#include "song.h"
#include "export_mp4.h"
#include "language.h"
#include "update.h"

#ifdef _DEBUG
int main()
#else
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
#endif
{
  // SetThreadUILanguage(LANG_ENGLISH);

  // Initialize COM
  if (FAILED(CoInitialize(NULL)))
    return 1;

  // Config initialize
  if (config_init()) {
    MessageBox(NULL, lang_get_last_error(), APP_NAME, MB_OK);
    return 1;
  }

  // Initialize language
  lang_init();

  // Initialize GUI
  if (gui_init()) {
    MessageBox(NULL, lang_get_last_error(), APP_NAME, MB_OK);
    return 1;
  }

  // Initialize display
  if (display_init(gui_get_window())) {
    MessageBox(NULL, lang_get_last_error(), APP_NAME, MB_OK);
    return 1;
  }

  // Initalize keyboard
  if (keyboard_init()) {
    MessageBox(NULL, lang_get_last_error(), APP_NAME, MB_OK);
    return 1;
  }

  // Show GUI
  gui_show();

  // Load default config
  config_load("freepiano.cfg");

  // Check for update
#ifndef _DEBUG
  update_check_async();
#endif

  // Open lyt
  // song_open_lyt("test3.lyt");
  // song_open("D:\\src\\freepiano\\trunk\\data\\song\\kiss the rain.fpm");
  // export_mp4("test.mp4");

  MSG msg;
  while (GetMessage(&msg, NULL, NULL, NULL)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  config_save("freepiano.cfg");

  // Shutdown keyboard
  keyboard_shutdown();

  // Shutdown config
  config_shutdown();

  // Shutdown display
  display_shutdown();

  // Shutdown COM
  CoUninitialize();

  return 0;
}
