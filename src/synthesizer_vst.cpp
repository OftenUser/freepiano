#include "pch.h"
#include <winreg.h>

#include "vst/aeffect.h"
#include "vst/aeffectx.h"

#include "synthesizer_vst.h"
#include "display.h"
#include "config.h"
#include "export.h"
#include "output_wasapi.h"

// global vst effect instance
static AEffect *effect = NULL;

// global vst effect module.
static HMODULE module = NULL;

// effect editor window
static HWND editor_window = NULL;

// midi event buffer
static VstMidiEvent midi_event_buffer[256];

// midi event count
static uint midi_event_count = 0;

// vsti midi lock
static thread_lock_t vsti_thread_lock;

// effect process parameters
static float effect_processing = 0;
static float effect_samplerate = 0;
static uint effect_blocksize = 0;
static bool effect_show_editor = true;
static float *temp_output = NULL;

// -----------------------------------------------------------------------------------------
// vsti functions
// -----------------------------------------------------------------------------------------
static VstIntPtr VSTCALLBACK HostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {
  VstIntPtr result = 0;

  switch (opcode) {
   case audioMasterVersion:
     result = kVstVersion;
     break;

   case audioMasterCurrentId:       ///< [return value]: current unique identifier on shell plug-in  @see AudioEffect::getCurrentUniqueId
     break;

   case audioMasterIdle:            ///< no arguments  @see AudioEffect::masterIdle
     break;

   case DECLARE_VST_DEPRECATED(audioMasterWantMidi):
     return 1;
     break;

   case audioMasterGetTime:
     break;

   case audioMasterProcessEvents:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterIOChanged:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case DECLARE_VST_DEPRECATED(audioMasterNeedIdle):
     return 1;
     break;

   case audioMasterSizeWindow:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterGetSampleRate:
     switch (config_get_current_output_type()) {
     case OUTPUT_TYPE_WASAPI:
       result = wasapi_get_samplerate();
       break;
       
     default:
       result = 44100;
       break;
     }
     break;

   case audioMasterGetBlockSize:
     result = 32;
     break;

   case audioMasterGetInputLatency:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterGetOutputLatency:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;


   case audioMasterGetCurrentProcessLevel:
     if (export_rendering()) {
       return kVstProcessLevelOffline;
     } else {
       return kVstProcessLevelRealtime;
     }
     break;

   case audioMasterGetAutomationState:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterOfflineStart:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterOfflineRead:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterOfflineWrite:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterOfflineGetCurrentPass:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterOfflineGetCurrentMetaPass:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterGetVendorString:
     if (ptr) {
       strcpy((char *)ptr, APP_NAME);
       return 1;
     }
     break;

   case audioMasterGetProductString:
     if (ptr) {
       strcpy((char *)ptr, APP_NAME);
       return 1;
     }
     break;

   case audioMasterGetVendorVersion:
     return APP_VERSION;
     break;

   case audioMasterVendorSpecific:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterCanDo:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterGetLanguage:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterGetDirectory:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterUpdateDisplay:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterBeginEdit:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterEndEdit:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterOpenFileSelector:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   case audioMasterCloseFileSelector:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;

   default:
     printf("PLUG> HostCallback (Opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void>(value), ptr, opt);
     break;
  }

  return result;
}

void check_effect_properties(AEffect *effect) {
  printf("HOST> Gathering properties...\n");

  char effectName[256] = {0};
  char vendorString[256] = {0};
  char productString[256] = {0};

  effect->dispatcher(effect, effGetEffectName, 0, 0, effectName, 0);
  effect->dispatcher(effect, effGetVendorString, 0, 0, vendorString, 0);
  effect->dispatcher(effect, effGetProductString, 0, 0, productString, 0);

  printf("Name = %s\nVendor = %s\nProduct = %s\n\n", effectName, vendorString, productString);

  printf("numPrograms = %d\nnumParams = %d\nnumInputs = %d\nnumOutputs = %d\n\n",
         effect->numPrograms, effect->numParams, effect->numInputs, effect->numOutputs);

  // Iterate programs...
  for (VstInt32 progIndex = 0; progIndex < effect->numPrograms; progIndex++) {
    char progName[256] = {0};
    if (!effect->dispatcher(effect, effGetProgramNameIndexed, progIndex, 0, progName, 0)) {
      effect->dispatcher(effect, effSetProgram, 0, progIndex, 0, 0);        // Note: Old program not restored here!
      effect->dispatcher(effect, effGetProgramName, 0, 0, progName, 0);
    }
    printf("Program %03d: %s\n", progIndex, progName);
  }

  printf("\n");

  // Iterate parameters...
  for (VstInt32 paramIndex = 0; paramIndex < effect->numParams; paramIndex++) {
    char paramName[256] = {0};
    char paramLabel[256] = {0};
    char paramDisplay[256] = {0};

    effect->dispatcher(effect, effGetParamName, paramIndex, 0, paramName, 0);
    effect->dispatcher(effect, effGetParamLabel, paramIndex, 0, paramLabel, 0);
    effect->dispatcher(effect, effGetParamDisplay, paramIndex, 0, paramDisplay, 0);
    float value = effect->getParameter(effect, paramIndex);

    printf("Param %03d: %s [%s %s] (normalized = %f)\n", paramIndex, paramName, paramDisplay, paramLabel, value);
  }

  printf("\n");

  // Can-do nonsense...
  static const char *canDos[] = {
    "receiveVstEvents",
    "receiveVstMidiEvent",
    "midiProgramNames"
  };

  for (VstInt32 canDoIndex = 0; canDoIndex < sizeof (canDos) / sizeof (canDos[0]); canDoIndex++) {
    printf("Can do %s... ", canDos[canDoIndex]);
    VstInt32 result = (VstInt32)effect->dispatcher(effect, effCanDo, 0, 0, (void *)canDos[canDoIndex], 0);
    switch (result) {
     case 0: printf("Don't know"); break;
     case 1: printf("Yes"); break;
     case -1: printf("Definitely not!"); break;
     default: printf("?????");
    }
    printf("\n");
  }

  printf("\n");
}

// Load plugin
int vsti_load_plugin(const char *path) {
  thread_lock lock(vsti_thread_lock);

  // Unload plugin first
  vsti_unload_plugin();

  typedef AEffect * (*PluginEntryProc)(audioMasterCallback audioMaster);

  // Load library
  module = LoadLibrary(path);

  if (module == NULL)
    return -1;

  // Get effect constructor.
  PluginEntryProc mainProc = 0;
  mainProc = (PluginEntryProc)GetProcAddress((HMODULE)module, "VSTPluginMain");

  if (!mainProc)
    mainProc = (PluginEntryProc)GetProcAddress((HMODULE)module, "main");

  if (!mainProc)
    return -1;

  // Create effect instance
  effect = mainProc(HostCallback);

  if (effect && (effect->magic != kEffectMagic))
    effect = NULL;

  if (effect == NULL)
    return NULL;

  // Open effect
  effect->dispatcher(effect, effOpen, 0, NULL, 0, 0);
  effect->dispatcher(effect, effBeginSetProgram, 0, NULL, 0, 0);
  effect->dispatcher(effect, effSetProgram, 0, 0, 0, 0);
  effect->dispatcher(effect, effEndSetProgram, 0, NULL, 0, 0);

  // Reset effect flags
  effect_processing = 0;

#ifdef _DEBUG
  // Check effect properties
  // check_effect_properties(effect);
#endif

  // Show editor
  vsti_show_editor(effect_show_editor);

  return 0;
}

// Unload plugin
void vsti_unload_plugin() {
  thread_lock lock(vsti_thread_lock);

  // Destroy effect window
  if (editor_window) {
    DestroyWindow(editor_window);
    editor_window = NULL;
  }

  // Close effect
  if (effect) {
    effect->dispatcher(effect, effClose, 0, NULL, 0, 0);
    effect = NULL;
  }

  // Unload effect DLL
  if (module) {
    FreeLibrary(module);
    module = NULL;
  }

  if (temp_output) {
    delete[] temp_output;
    temp_output = NULL;
  }
}


// Send MIDI event to
void vsti_send_midi_event(byte data1, byte data2, byte data3, byte data4) {
  thread_lock lock(vsti_thread_lock);

  if (midi_event_count < ARRAY_COUNT(midi_event_buffer)) {
    VstMidiEvent &e = midi_event_buffer[midi_event_count++];

    e.type = kVstMidiType;
    e.byteSize = sizeof(VstMidiEvent);
    e.deltaFrames = 0;
    e.flags = kVstMidiEventIsRealtime;
    e.noteLength = 0;
    e.noteOffset = 0;
    e.noteOffVelocity = 0;
    e.midiData[0] = data1;
    e.midiData[1] = data2;
    e.midiData[2] = data3;
    e.midiData[3] = data4;
    e.detune = 0;
    e.noteOffVelocity = 0;
    e.reserved1 = 0;
    e.reserved2 = 0;
  }
}

// Stop output
void vsti_stop_process() {
  thread_lock lock(vsti_thread_lock);

  if (effect) {
    effect->dispatcher(effect, effMainsChanged, 0, 0, 0, 0);
  }
}

void vsti_update_config(float samplerate, uint blocksize) {
  thread_lock lock(vsti_thread_lock);

  if (effect) {
    if (effect_processing == 0 ||
        effect_samplerate != samplerate ||
        effect_blocksize != blocksize) {
      effect_processing = 1;
      effect_samplerate = samplerate;
      effect_blocksize = blocksize;

      effect->dispatcher(effect, effMainsChanged, 0, 0, 0, 0);
      effect->dispatcher(effect, effSetSampleRate, 0, 0, 0, samplerate);
      effect->dispatcher(effect, effSetBlockSize, 0, blocksize, 0, 0);
      effect->dispatcher(effect, effMainsChanged, 0, 1, 0, 0);

      if (temp_output)
        delete [] temp_output;
      temp_output = new float[blocksize];
    }
  }
}

// Process
void vsti_process(float *left, float *right, uint buffer_size) {
  thread_lock lock(vsti_thread_lock);

  if (effect && effect_processing) {
    // Process events
    struct MoreEvents : VstEvents {
      VstEvent *data[256];
    }
    buffer;

    buffer.numEvents = midi_event_count;
    buffer.reserved = 0;

    for (uint i = 0; i < midi_event_count; i++)
      buffer.events[i] = (VstEvent *)&midi_event_buffer[i];

    // Process events
    effect->dispatcher(effect, effProcessEvents, 0, 0, &buffer, 0);

    // Clear processed events
    midi_event_count = 0;

    // Clear data
    memset(left, 0, buffer_size * sizeof(float));
    memset(right, 0, buffer_size * sizeof(float));

    // TODO: VSTi has its own buffer count
    float *outputs[64] = { left, right };
    float *inputs[64] = {0};

    for (int i = 2; i < effect->numOutputs; i++)
      outputs[i] = temp_output;

    for (int i = 0; i < effect->numInputs; i++)
      inputs[i] = temp_output;

    effect->dispatcher(effect, effStartProcess, 0, 0, 0, 0);
    effect->processReplacing(effect, inputs, outputs, buffer_size);
    effect->dispatcher(effect, effStopProcess, 0, 0, 0, 0);

    return;
  }

  memset(left, 0, buffer_size * sizeof(float));
  memset(right, 0, buffer_size * sizeof(float));
}

// -----------------------------------------------------------------------------------------
// VSTi editor functions
// -----------------------------------------------------------------------------------------
// VST window proc
static LRESULT CALLBACK vst_editor_wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
   case WM_CREATE:
     SetTimer(hWnd, 1, 20, 0);
     break;

   case WM_TIMER: {
     AEffect *effect = (AEffect *)GetWindowLong(hWnd, GWL_USERDATA);

     if (effect) {
       effect->dispatcher(effect, DECLARE_VST_DEPRECATED(effIdle), 0, 0, 0, 0);
       effect->dispatcher(effect, effEditIdle, 0, 0, 0, 0);
     }
   }
   break;

   case WM_CLOSE:
     effect_show_editor = false;
     DestroyWindow(hWnd);
     break;

   case WM_DESTROY: {
     AEffect *effect = (AEffect *)GetWindowLong(hWnd, GWL_USERDATA);

     if (effect)
       effect->dispatcher(effect, effEditClose, 0, 0, 0, 0);
     editor_window = NULL;
   }
   break;

   default:
     return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }

  return 0;
}


// Create main window
static HWND create_effect_window(AEffect *effect) {
  static bool initialized = false;

  if (!initialized) {
    // Register window class
    WNDCLASSEX wc = { sizeof(wc), 0 };
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = &vst_editor_wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "FreePianoVstEffect";
    wc.hIconSm = NULL;

    RegisterClassEx(&wc);

    initialized = true;
  }

  char effectName[256] = {0};
  effect->dispatcher(effect, effGetEffectName, 0, 0, effectName, 0);

  // Create window
  HWND hwnd = CreateWindow("FreePianoVstEffect", effectName, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);

  if (hwnd) {
    SetWindowLong(hwnd, GWL_USERDATA, (LONG)effect);

    ERect *eRect = 0;
    effect->dispatcher(effect, effEditOpen, 0, 0, hwnd, 0);
    effect->dispatcher(effect, effEditGetRect, 0, 0, &eRect, 0);

    if (eRect) {
      int width = eRect->right - eRect->left;
      int height = eRect->bottom - eRect->top;
      uint style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

      RECT rect;
      SetRect(&rect, 0, 0, width, height);
      AdjustWindowRect(&rect, GetWindowLong(hwnd, GWL_STYLE), FALSE);

      width = rect.right - rect.left;
      height = rect.bottom - rect.top;

      SetWindowPos(hwnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE);
      ShowWindow(hwnd, SW_SHOW);
    }

  }

  return hwnd;
}

// Is show eidtor
bool vsti_is_show_editor() {
  return effect_show_editor;
}

// Show effect editor
void vsti_show_editor(bool show) {
  thread_lock lock(vsti_thread_lock);

  // Set flag
  effect_show_editor = show;

  // No effect loaded
  if (!effect)
    return;

  // Effect has no editor
  if ((effect->flags & effFlagsHasEditor) == 0)
    return;

  if (show) {
    // Create effect window
    if (!editor_window) {
      editor_window = create_effect_window(effect);
    }
  } else   {
    // Destroy effect window
    if (editor_window) {
      DestroyWindow(editor_window);
      editor_window = NULL;
    }
  }
}

static void search_plugins(const char *path, vsti_enum_callback &callback) {
  char buffer[256] = {0};
  _snprintf(buffer, sizeof(buffer), "%s\\*.dll", path);

  WIN32_FIND_DATAA data;
  HANDLE finddata;

  finddata = FindFirstFileA(buffer, &data);

  if (finddata != INVALID_HANDLE_VALUE) {
    do {
      _snprintf(buffer, sizeof(buffer), "%s\\%s", path, data.cFileName);
      callback(buffer);
    } while (FindNextFileA(finddata, &data));
  }

  FindClose(finddata);

  // Search subdirectoies
  _snprintf(buffer, sizeof(buffer), "%s\\*", path);
  finddata = FindFirstFileA(buffer, &data);

  if (finddata != INVALID_HANDLE_VALUE) {
    do {
      if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (strcmp(data.cFileName, ".") == 0 ||
            strcmp(data.cFileName, "..") == 0)
          continue;

        _snprintf(buffer, sizeof(buffer), "%s\\%s", path, data.cFileName);
        search_plugins(buffer, callback);
      }
    } while (FindNextFileA(finddata, &data));
  }

  FindClose(finddata);
}

// VST enum plugins
void vsti_enum_plugins(vsti_enum_callback &callback) {
  HKEY key = HKEY_LOCAL_MACHINE;

  char buff[256];
  config_get_media_path(buff, sizeof(buff), "");
  search_plugins(buff, callback);

  if (ERROR_SUCCESS == RegOpenKeyEx(key, "SOFTWARE", 0, KEY_READ, &key) &&
      ERROR_SUCCESS == RegOpenKeyEx(key, "VST", 0, KEY_READ, &key)) {
    char buffer[256] = {0};
    DWORD size = sizeof(buffer);

    if (ERROR_SUCCESS == RegQueryValueExA(key, "VSTPluginsPath", NULL, NULL, (byte *)buffer, &size)) {
      search_plugins(buffer, callback);
    }
  }
}


// Is instrument loaded
bool vsti_is_instrument_loaded() {
  thread_lock lock(vsti_thread_lock);
  return effect != NULL;

}
