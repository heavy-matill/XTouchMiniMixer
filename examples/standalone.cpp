/*
 *******************************************************************************
 * 16 Channel controller for Behringer xTouchMini MIDI USB controller.
 *******************************************************************************
 */
// XTouch in Standard Mode
// All buttons set to default (hold LayerA+B before connecting to power to
// reset)
#include <Arduino.h>
#include <xTouchMiniMixer.hpp>

//#define MY_DEBUG  // comment to not run debug

#ifndef MY_DEBUG
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#else
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#endif

// delay constants
// delay after encoder is not touched anymore 1000ms
#define DLY_COOL 1000
// delay after sending Midi command to XTouch device 5ms
#define DLY_MIDI 5

USB Usb;
USBHub Hub(&Usb);
USBH_MIDI Midi(&Usb);
XTouchMiniMixer xTouch(&Midi, &Usb);

void onInitUSB() {
  Serial.print("USB device connected: VID=");
  Serial.print(xTouch.getVID());
  Serial.print(", PID=");
  Serial.println(xTouch.getPID());
  xTouch.visualizeAll();
}

// main
void setup() {
  Serial.begin(115200);
  xTouch.setup();
}
void loop() {
  
  xTouch.update();
}
