#include "xTouchMiniMixer.hpp"

XTouchMiniMixer::XTouchMiniMixer(USBH_MIDI *new_pUSBH_MIDI, USB *new_pUSB)
    : pUSBH_MIDI(new_pUSBH_MIDI), pUSB(new_pUSB) {}

void XTouchMiniMixer::setup() {
  DEBUG_PRINTLN("initializing XTouch");
  setupDebuggingCallbacks();
  setupMidi();
}

void XTouchMiniMixer::setupMidi() {
  if (pUSB->Init() == -1) {
    while (1)
      ;  // halt if (pUSB->Init()==-1
  }
  DEBUG_PRINTLN("initialized pUSB");
}

void XTouchMiniMixer::update() {
  pUSB->Task();
  if (&pUSBH_MIDI) {
    MIDI_poll();
  }
  updateCooldowns();
  sendBuf();
}

void XTouchMiniMixer::visualizeAll() {
  visualizeMuteLeds();
  visualizeControlMode();
  visualizeRotaryValues();
}
void XTouchMiniMixer::setValueMute(uint8_t id, bool val) {
  setMuteBit(id, val);
  if (idInActualLayer(id)) return visualizeMuteLed(id);
}
void XTouchMiniMixer::setValueFade(uint8_t id, uint8_t val) {
  DEBUG_PRINT("setValueFade id=");
  DEBUG_PRINT(id);
  DEBUG_PRINT(" val=");
  DEBUG_PRINTLN(val);
  faders[id] = val;
  if (idInActualLayer(id) && (st_control == 0))
    return visualizeHotRotaryValue(id % 8);
}
void XTouchMiniMixer::setValuePan(uint8_t id, uint8_t val) {
  pans[id] = val;
  if (idInActualLayer(id) && (st_control == 1))
    return visualizeHotRotaryValue(id % 8);
}
void XTouchMiniMixer::setValueGain(uint8_t id, uint8_t val) {
  gains[id] = val;
  if (idInActualLayer(id) && (st_control == 2))
    return visualizeHotRotaryValue(id % 8);
}
void XTouchMiniMixer::setValueMix(uint8_t id, uint8_t val, uint8_t bus) {
  mixes[bus - 1][id] = val;
  if (idInActualLayer(id) && (bus == getBusMinCh()))
    return visualizeHotRotaryValue(id % 8);
}

void XTouchMiniMixer::setValueColor(uint8_t id, uint8_t val) {
  colors[id] = val;
  // if (idInActualLayer(id)) return visualizeColor(id);
}

void XTouchMiniMixer::visualizeRotaryValues(uint8_t id_start, uint8_t id_end) {
  if (st_control == 0) {  // faders
    for (; id_start <= id_end; id_start++) {
      setRotary(id_start + st_layer * 10, faders[id_start + st_layer * 8]);
    }
  } else if (st_control == 1) {  // pans
    for (; id_start <= id_end; id_start++) {
      setRotary(id_start + st_layer * 10, pans[id_start + st_layer * 8]);
    }
  } else if (st_control == 2) {  // gains
    for (; id_start <= id_end; id_start++) {
      setRotary(id_start + st_layer * 10, gains[id_start + st_layer * 8]);
    }
  } else {  // mixes, set to values of minimum bus
    for (; id_start <= id_end; id_start++) {
      setRotary(id_start + st_layer * 10,
                mixes[getBusMinCh() - 1][id_start + st_layer * 8]);
    }
  }
}

void XTouchMiniMixer::visualizeHotRotaryValue(uint8_t id) {
  if (!cooldownActive(id)) return visualizeRotaryValue(id);
}
void XTouchMiniMixer::visualizeRotaryValue(uint8_t id) {
  return visualizeRotaryValues(id, id);
}

void XTouchMiniMixer::sendMidiData(uint8_t cmd, uint8_t id, uint8_t val) {
  DEBUG_PRINT("sendMidiData cmd=");
  DEBUG_PRINT(cmd);
  DEBUG_PRINT(" id=");
  DEBUG_PRINT(id);
  DEBUG_PRINT(" val=");
  DEBUG_PRINTLN(val);
  uint8_t buf[3];
  buf[0] = cmd;
  buf[1] = id;
  buf[2] = val;
  appendToBuf(buf);
}

void XTouchMiniMixer::appendToBuf(uint8_t *buf_new) {
  buf_midi[j_buf++] = buf_new[0];
  buf_midi[j_buf++] = buf_new[1];
  buf_midi[j_buf++] = buf_new[2];
  if (j_buf == sizeof(buf_midi) / sizeof(uint8_t)) {
    j_buf = 0;
  }
  DEBUG_PRINT("midi idx i: ");
  DEBUG_PRINT(i_buf);
  DEBUG_PRINT(", j: ");
  DEBUG_PRINTLN(j_buf);
}

void XTouchMiniMixer::sendBuf() {
  if (i_buf != j_buf) {
    if (time_last > (time_midi + DLY_MIDI)) {
      time_midi = time_last;
      uint8_t buf_send[3];
      buf_send[0] = buf_midi[i_buf++];
      buf_send[1] = buf_midi[i_buf++];
      buf_send[2] = buf_midi[i_buf++];
      if (i_buf == sizeof(buf_midi) / sizeof(uint8_t)) {
        i_buf = 0;
      }
      pUSBH_MIDI->SendData(buf_send);
    }
  }
}

void XTouchMiniMixer::setRotary(uint8_t id, uint8_t val) {
  // CC: B0 | channel 10 = BA and sets the actual value of the encoder
  sendMidiData(midi::ControlChange | 10, id + 1, val);
}

void XTouchMiniMixer::setRotaryStyle(uint8_t id, uint8_t val) {
  // CC: B0 | channel 00 = BA and sets the style of the encoder
  sendMidiData(midi::ControlChange, id + 1, val);
}

void XTouchMiniMixer::setButtonLed(uint8_t id, uint8_t val) {
  // 0x90 = 144 = NOTE_ON
  // https://www.woodbrass.com/images/woodbrass/BEHRINGER+XTOUCH+MINI.PDF
  sendMidiData(midi::NoteOn, id, val);
}

void XTouchMiniMixer::visualizeMuteLeds() {
  for (uint8_t id = 0; id < 16; id++) {
    // internally checked if i is in actual layer
    visualizeMuteLed(id);
  }
}

void XTouchMiniMixer::visualizeMuteLed(uint8_t id) {
  // visualize others anyway
  // TBD
  if (idInActualLayer(id)) {
    id %= 8;
    // ids for lower row 8-15, bitmask depending on st_layer 0-7 or 8-15
    setButtonLed(id + 8, getMuteValue(id + st_layer * 8));
  }
}

bool XTouchMiniMixer::getMuteValue(uint8_t i) { return (mutes & (1 << i)) > 0; }
void XTouchMiniMixer::setMuteBit(uint8_t i, bool val) {
  if (val)  // set bit
    mutes |= (1 << i);
  else  // clear bit
    mutes &= ~(1 << i);
}

void XTouchMiniMixer::visualizeControlMode() {
  // Buttons
  if (st_control <= 3) {  // only light up a active state
    for (uint8_t i = 0; i < 8; i++) {
      setButtonLed(i, (st_control >> i) & 1);
    }
  } else {  // disable first two leds
    setButtonLed(0, 0);
    setButtonLed(1, 0);
    for (uint8_t i = 2; i < 8; i++) {  // blink all monitoring states
      setButtonLed(i, 2 - ((st_control >> i) & 1));
    }
  }
  // Rotaries
  if (st_control == 1) {  // pan -> pan style
    for (uint8_t i = 0; i < 8; i++) {
      setRotaryStyle(i, 4);
    }
  } else if (st_control == 2) {  // gain -> single style for higher accuracy
    for (uint8_t i = 0; i < 8; i++) {
      setRotaryStyle(i, 1);
    }
  } else {  // spread style for anything else
    for (uint8_t i = 0; i < 8; i++) {
      setRotaryStyle(i, 2);
    }
  }
}

void XTouchMiniMixer::onButtonUp(uint8_t id) {
  DEBUG_PRINT("Button up id=");
  DEBUG_PRINTLN(id);

  // get information about layer
  handleNewLayerState(id >= 24);

  if (16 <= id && id < 24) {
    visualizeMuteLed(id - 16);  // ??? reset it because touch enables it?
    return muteChannelCallback(id - 15, getMuteValue(id - 16));
  }
  if (40 <= id && id < 48) {
    visualizeMuteLed(id - 40);
    return muteChannelCallback(id - 31, getMuteValue(id - 32));
  }

  if ((8 <= id && id < 16) | (32 <= id && id < 40)) {
    // switch to states gain pan etc. independent of layer
    handleControlStateInput(id % 8);
  }
}

void XTouchMiniMixer::onEncoderMoved(uint8_t ch, uint8_t val) {
  // get information about layer and load into st_layer
  DEBUG_PRINTLN("onEncoderMoved");
  handleNewLayerState(ch >= 10);
  switch (ch) {
    case 9:  // main fader
      onSliderMoved(val, main);
      break;
    case 10:  // aux fader
      onSliderMoved(val, aux);
      break;
    default:
      if (ch > 10) {
        ch -= 2;
      }
      enableCooldown((ch - 1) % 8);
      if (st_control == 0) {
        fadeChannelCallback(ch, val);
      } else if (st_control == 1) {
        panChannelCallback(ch, val);
      } else if (st_control == 2) {
        gainChannelCallback(ch, val);
      } else {
        // set to values of minimum bus
        for (uint8_t bus = 1; bus <= 6; bus++) {
          if (st_control & (1 << (1 + bus))) {
            mixChannelCallback(ch, bus, val);
          }
        }
      }
  }
}

void XTouchMiniMixer::onSliderMoved(uint8_t val, int8_t aux_main) {
  enableCooldown(8);
  if (hw_slider < 0) {  // hw position unknown, never been initialized
    hw_slider = val;
    return;
  }
  int8_t old_sign = slider_sign;
  if (val > hw_slider) {
    slider_sign = 1;
  } else {
    slider_sign = -1;
  }
  if (old_sign != slider_sign) {
    // reset start value

    hw_slider_strt = hw_slider;
    aux_main_strt = aux_main;
  }
  if (st_layer) {
    fadeAuxCallback(
        linearizedValue(val - hw_slider_strt, hw_slider_strt, aux_main_strt));

  } else {
    fadeMainCallback(
        linearizedValue(val - hw_slider_strt, hw_slider_strt, aux_main_strt));
  }
  hw_slider = val;
}

int8_t XTouchMiniMixer::linearizedValue(int8_t delta_hw, int8_t hw0,
                                        int8_t y0) {
  float delta_y;
  // calculate slope via linearization from actual position to max/min
  if (delta_hw > 0) {
    // inf due to hw0 = 127 will not happen because then delta_hw cannot be >
    // 0
    delta_y = (127.0 - y0) / (127.0 - hw0) * delta_hw;
    if ((delta_y <= 1.0))  // move at least 1 in positive direction
      return y0 + 1;
  } else {
    // inf due to hw0 = 0 will not happen because then delta_hw cannot be < 0
    delta_y = (float)y0 / (float)hw0 * delta_hw;
    if ((delta_y >= -1.0))  // move at least 1 in negative direction
      return y0 - 1;
  }
  return y0 + delta_y;
}

void XTouchMiniMixer::handleControlStateInput(uint8_t button_control) {
  disableAllCooldowns();
  uint8_t new_control = 1 << button_control;
  if (st_control <= 3) {
    // actual mode is either 0: fade, 1: pan, 2: gain
    if (new_control == st_control) {
      // active state was pushed
      // deactivate and return to 0 = fade control
      st_control = 0;
    } else {
      // switched to new control mode
      st_control = new_control;
    }
  } else {
    // actual mode is monitor 1-6
    if (new_control > 3) {
      // new input is monitoring
      // XOR actual and new monitoring
      st_control ^= new_control;
    } else {
      // switch to non monitoring related mode
      st_control = new_control;
    }
  }
  visualizeControlMode();
  visualizeRotaryValues();
}

void XTouchMiniMixer::handleNewLayerState(uint8_t new_layer) {
  if (st_layer != new_layer) {
    disableAllCooldowns();
    st_layer = new_layer;
    visualizeControlMode();
    visualizeRotaryValues();
    visualizeMuteLeds();
  }
}

void XTouchMiniMixer::blinkAllButtons() {
  for (uint8_t i = 0; i < 16; i++) {
    setButtonLed(i, 2);
  }
}

uint8_t XTouchMiniMixer::getBusMinCh() {
  if (st_control <= 3) {
    return 0;
  }
  uint8_t temp_st = st_control;
  uint8_t bus_min = 6;
  while (temp_st <<= 1) {
    bus_min--;
  }
  return bus_min;
}

bool XTouchMiniMixer::idInActualLayer(uint8_t i) { return st_layer ^ (i < 8); }

// Poll USB MIDI Controler and send to serial MIDI
void XTouchMiniMixer::MIDI_poll() {
  uint8_t bufMidi[MIDI_EVENT_PACKET_SIZE];
  uint16_t rcvd;

  if (pUSBH_MIDI->RecvData(&rcvd, bufMidi) == 0) {
    switch (bufMidi[0]) {
      case 0x08:  //  Button up
        onButtonUp(bufMidi[2]);
        break;
      case 0x09:  //  Button down
        break;
      case 0x0B:  // Encoder moved
        onEncoderMoved(bufMidi[2], bufMidi[3]);
        break;
      default:  // print all
        break;
    }
  }
}

void XTouchMiniMixer::disableAllCooldowns() {
  for (uint8_t i = 0; i < (sizeof(cooldowns) / sizeof(*cooldowns)); i++) {
    cooldowns[i] = time_last;
  }
  executeCooldownTask(8);
}

bool XTouchMiniMixer::cooldownActive(uint8_t id, unsigned long time_act) {
  if (!time_act) {
    time_act = time_last;
  }
  return cooldowns[id] > time_act;
}
void XTouchMiniMixer::enableCooldown(uint8_t id) {
  cooldowns[id] = time_last + DLY_COOL;
}
void XTouchMiniMixer::updateCooldowns() {
  unsigned long time_new = millis();
  for (uint8_t i = 0; i < (sizeof(cooldowns) / sizeof(*cooldowns)); i++) {
    if (cooldownActive(i) & !cooldownActive(i, time_new)) {
      executeCooldownTask(i);
    }
  }
  time_last = time_new;
}
void XTouchMiniMixer::executeCooldownTask(uint8_t id) {
  if (id == 8) {  // main/aux
    slider_sign = 0;
    if (st_layer) {
      aux_main_strt = aux;
    } else {
      aux_main_strt = main;
    }
  } else {  // rotary
    visualizeHotRotaryValue(id + 1);
  }
}

void XTouchMiniMixer::muteChannelPrintln(uint8_t ch, uint8_t val) {
  Serial.print("muteChannelCallback ch=");
  Serial.print(ch);
  Serial.print(", val=");
  Serial.println(val);
}
void XTouchMiniMixer::fadeChannelPrintln(uint8_t ch, uint8_t val) {
  Serial.print("fadeChannelCallback ch=");
  Serial.print(ch);
  Serial.print(", val=");
  Serial.println(val);
}
void XTouchMiniMixer::panChannelPrintln(uint8_t ch, uint8_t val) {
  Serial.print("panChannelCallback ch=");
  Serial.print(ch);
  Serial.print(", val=");
  Serial.println(val);
}
void XTouchMiniMixer::gainChannelPrintln(uint8_t ch, uint8_t val) {
  Serial.print("gainChannelCallback ch=");
  Serial.print(ch);
  Serial.print(", val=");
  Serial.println(val);
}
void XTouchMiniMixer::mixChannelPrintln(uint8_t ch, uint8_t bus, uint8_t val) {
  Serial.print("mixChannelCallback ch=");
  Serial.print(ch);
  Serial.print(", bus=");
  Serial.print(bus);
  Serial.print(", val=");
  Serial.println(val);
}
void XTouchMiniMixer::fadeMainPrintln(uint8_t val) {
  Serial.print("fadeMainCallback val=");
  Serial.println(val);
}
void XTouchMiniMixer::fadeAuxPrintln(uint8_t val) {
  Serial.print("fadeAuxCallback val=");
  Serial.println(val);
}