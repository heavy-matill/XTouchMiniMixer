#pragma once

#ifndef DLY_MIDI
// delay after sending Midi command to XTouch device 5ms
#define DLY_MIDI 5
#endif
#ifndef DLY_COOL
// delayed after encoder is not touched anymore
#define DLY_COOL 1000
#endif

#include <Arduino.h>
#include <MIDI.h>
#include <stdint.h>
#include <usbh_midi.h>
#include <usbhub.h>

class XTouchMiniMixer {
 public:
  XTouchMiniMixer(){};
  // initialize with pointers to Midi host device USBH_MIDI and USB
  XTouchMiniMixer(USBH_MIDI *p, USB *pUSB);
  // states
  uint8_t st_layer, st_control;
  // mixer values
  uint8_t faders[16], gains[16], pans[16], mixes[6][16], colors[16];
  int8_t aux, aux_main_strt, main, main_strt, hw_slider = -1, hw_slider_strt;
  int8_t slider_sign;
  uint16_t mutes;
  // tasking 8 cooldowns for rotaries and 1 for main/aux
  unsigned long cooldowns[9], time_last, time_midi, time_init;
  // midi data
  uint8_t buf_midi[255];  // 3*85
  uint8_t i_buf, j_buf;

  void setup();
  void setupMidi();
  void update();

  void visualizeAll();

  // setters to set local mixer values
  void setValueMute(uint8_t id, bool val);
  void setValueFade(uint8_t id, uint8_t val);
  void setValuePan(uint8_t id, uint8_t val);
  void setValueGain(uint8_t id, uint8_t val);
  void setValueMix(uint8_t id, uint8_t val, uint8_t bus);
  void setValueMix1(uint8_t id, uint8_t val) { return setValueMix(id, val, 1); }
  void setValueMix2(uint8_t id, uint8_t val) { return setValueMix(id, val, 2); }
  void setValueMix3(uint8_t id, uint8_t val) { return setValueMix(id, val, 3); }
  void setValueMix4(uint8_t id, uint8_t val) { return setValueMix(id, val, 4); }
  void setValueMix5(uint8_t id, uint8_t val) { return setValueMix(id, val, 5); }
  void setValueMix6(uint8_t id, uint8_t val) { return setValueMix(id, val, 6); }

  void setValueAux(uint8_t val) {
    aux = val;
    if (!cooldownActive(8)) executeCooldownTask(8);
  };
  void setValueMain(uint8_t val) {
    main = val;
    if (!cooldownActive(8)) executeCooldownTask(8);
  };
  void setValueColor(uint8_t id, uint8_t val);

  // visualizations set to certain values or update to whats stored in mixer
  // values
  void visualizeRotaryValues(uint8_t id_start = 0, uint8_t id_end = 7);
  void visualizeHotRotaryValue(uint8_t id);
  void visualizeRotaryValue(uint8_t id);
  // private
  void sendMidiData(uint8_t cmd, uint8_t id, uint8_t val);
  void appendToBuf(uint8_t *buf_new);
  void sendBuf();

  void setRotary(uint8_t id, uint8_t val);
  void setRotaryStyle(uint8_t id, uint8_t val);
  void setButtonLed(uint8_t id, uint8_t val);

  // button visualizations
  void visualizeMuteLeds();
  void visualizeMuteLed(uint8_t id);
  // private
  bool getMuteValue(uint8_t i);
  void setMuteBit(uint8_t i, bool val);

  // control mode visualization
  void visualizeControlMode();

  // input callbacks
  void onButtonUp(uint8_t id);
  void onEncoderMoved(uint8_t ch, uint8_t val);
  void onSliderMoved(uint8_t val, int8_t aux_main);
  static int8_t linearizedValue(int8_t delta_hw, int8_t hw0, int8_t y0);

  // state functions
  void handleControlStateInput(uint8_t button_control);

  void handleNewLayerState(uint8_t new_layer);

  void blinkAllButtons();

  uint8_t getBusMinCh();
  bool idInActualLayer(uint8_t i);
  // Midi related
  void MIDI_poll();

  // cooldown related
  void disableAllCooldowns();
  bool cooldownActive(uint8_t id, unsigned long time_act = 0);
  void enableCooldown(uint8_t id);
  void updateCooldowns();
  void executeCooldownTask(uint8_t id);

  uint16_t getVID() { return pUSBH_MIDI->idVendor(); }
  uint16_t getPID() { return pUSBH_MIDI->idProduct(); }

  void attachOnInitUSB(void (*p_func_usb)()) { pUSBH_MIDI->attachOnInit(p_func_usb);};
  void setupDebuggingCallbacks() {
    muteChannelCallback = muteChannelPrintln;
    fadeChannelCallback = fadeChannelPrintln;
    panChannelCallback = panChannelPrintln;
    gainChannelCallback = gainChannelPrintln;
    mixChannelCallback = mixChannelPrintln;
    fadeMainCallback = fadeMainPrintln;
    fadeAuxCallback = fadeAuxPrintln;
  }
  void (*muteChannelCallback)(uint8_t, uint8_t) = nullptr;
  void (*fadeChannelCallback)(uint8_t, uint8_t) = nullptr;
  void (*panChannelCallback)(uint8_t, uint8_t) = nullptr;
  void (*gainChannelCallback)(uint8_t, uint8_t) = nullptr;
  void (*mixChannelCallback)(uint8_t, uint8_t, uint8_t) = nullptr;
  void (*fadeMainCallback)(uint8_t) = nullptr;
  void (*fadeAuxCallback)(uint8_t) = nullptr;

 private:
  static void muteChannelPrintln(uint8_t ch, uint8_t val);
  static void fadeChannelPrintln(uint8_t ch, uint8_t val);
  static void panChannelPrintln(uint8_t ch, uint8_t val);
  static void gainChannelPrintln(uint8_t ch, uint8_t val);
  static void mixChannelPrintln(uint8_t ch, uint8_t bus, uint8_t val);
  static void fadeMainPrintln(uint8_t val);
  static void fadeAuxPrintln(uint8_t val);

 protected:
  USBH_MIDI *pUSBH_MIDI;
  USB *pUSB;
  // static void (*onInitUSBCallback)() = nullptr;
};