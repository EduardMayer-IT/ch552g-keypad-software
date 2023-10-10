/*
  HID Keyboard example


  created 2020
  by Deqing Sun for use with CH55xduino

  This example code is in the public domain.

  modified 2023
  by pakleni
*/
#define KeyPad1 KEY_LEFT_ALT
#define KeyPad2 'w'
#define KeyPad3 'f'
#define KeyPad4 'a'
#define KeyPad5 's'
#define KeyPad6 'd'

// These are done via HIDConsumer
#define RotaryKeypress MEDIA_VOLUME_MUTE
#define RotaryCW MEDIA_VOLUME_UP
#define RotaryCCW MEDIA_VOLUME_DOWN

#ifndef USER_USB_RAM
#error "This example needs to be compiled with a USER USB setting"
#endif

#include "src/userUsbHidMediaKeyboard/USBHIDMediaKeyboard.h"
#include <WS2812.h>

#define BUTTON1_PIN 11
#define BUTTON2_PIN 17
#define BUTTON3_PIN 16
#define BUTTON4_PIN 15
#define BUTTON5_PIN 14
#define BUTTON6_PIN 32
#define EC11D_PIN 33
#define EC11A_PIN 31
#define EC11B_PIN 30
#define LEDPIN 34

// brightness is from 0 to 1
#define BRIGHTNESS 0.3
// color delay controls the speed of the color change
// higher value means slower change
// it is in millis
#define COLOR_DELAY 50
#define NUM_LEDS 6
#define NUM_BYTES (NUM_LEDS * 3)

void handleColor()
{
  static byte mode = 0;
  static byte RValue = 0x00, GValue = 0x00, BValue = 0xff;
  static __xdata uint8_t ledData[NUM_BYTES];
  static unsigned long lastColorChange = 0;

  // The color delay is used for delaying a color change
  if (millis() < lastColorChange + COLOR_DELAY)
  {
    return;
  }

  switch (mode)
  {
  case 0:
    RValue++;
    BValue--;
    if (RValue == 0xff)
    {
      mode = 1;
    }
    break;
  case 1:
    GValue++;
    RValue--;
    if (GValue == 0xff)
    {
      mode = 2;
    }
    break;
  case 2:
    BValue++;
    GValue--;
    if (BValue == 0xff)
    {
      mode = 0;
    }
    break;
  }

  // Send color codes to the daisy chained LEDs
  for (int i = 0; i < 6; i++)
  {
    set_pixel_for_GRB_LED(ledData, i, RValue * BRIGHTNESS, GValue * BRIGHTNESS, BValue * BRIGHTNESS);
  }
  // This sends 24 bits for each LED
  // Each LED accepts 24 bits and forwards the rest
  // if changing the LED pin, make sure to change the
  // neopixel_show_P3_4 function as well
  neopixel_show_P3_4(ledData, NUM_BYTES);

  lastColorChange = millis();
}

// This controlls the button
// presses and releases
// for the 6 buttons
void buttonPress(int i)
{
  static byte pins[] = {
      BUTTON1_PIN,
      BUTTON2_PIN,
      BUTTON3_PIN,
      BUTTON4_PIN,
      BUTTON5_PIN,
      BUTTON6_PIN,
  };

  static byte keypad[] = {
      KeyPad1,
      KeyPad2,
      KeyPad3,
      KeyPad4,
      KeyPad5,
      KeyPad6,
  };

  static bool pressPrev[] = {
      false,
      false,
      false,
      false,
      false,
      false,
  };

  static unsigned long lastPressed[] = {
      0,
      0,
      0,
      0,
      0,
      0,
  };

  if (millis() > lastPressed[i] + 50)
  {
    bool press = !digitalRead(pins[i]);
    if (pressPrev[i] != press)
    {
      pressPrev[i] = press;
      if (press)
      {
        Keyboard_press(keypad[i]);
      }
      else
      {
        Keyboard_release(keypad[i]);
      }
      lastPressed[i] = millis();
    }
  }
}

void readEncoder()
{
  static uint8_t state = 0;
  bool CLKstate = digitalRead(EC11A_PIN);
  bool DTstate = digitalRead(EC11B_PIN);
  switch (state)
  {
  case 0: // Idle state, encoder not turning
    if (!CLKstate)
    { // Turn clockwise and CLK goes low first
      state = 1;
    }
    else if (!DTstate)
    { // Turn anticlockwise and DT goes low first
      state = 4;
    }
    break;
  // Clockwise rotation
  case 1:
    if (!DTstate)
    { // Continue clockwise and DT will go low after CLK
      state = 2;
    }
    break;
  case 2:
    if (CLKstate)
    { // Turn further and CLK will go high first
      state = 3;
    }
    break;
  case 3:
    if (CLKstate && DTstate)
    { // Both CLK and DT now high as the encoder completes one step clockwise
      state = 0;
      Consumer_write(RotaryCW);
    }
    break;
  // Anticlockwise rotation
  case 4: // As for clockwise but with CLK and DT reversed
    if (!CLKstate)
    {
      state = 5;
    }
    break;
  case 5:
    if (DTstate)
    {
      state = 6;
    }
    break;
  case 6:
    if (CLKstate && DTstate)
    {
      state = 0;
      Consumer_write(RotaryCCW);
    }
    break;
  }
}

void setup()
{
  USBInit();
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);
  pinMode(BUTTON5_PIN, INPUT_PULLUP);
  pinMode(BUTTON6_PIN, INPUT_PULLUP);
  pinMode(EC11D_PIN, INPUT_PULLUP);
  pinMode(EC11A_PIN, INPUT_PULLUP);
  pinMode(EC11B_PIN, INPUT_PULLUP);
  pinMode(LEDPIN, OUTPUT);
}

void loop()
{
  // BUTTONS 1..6
  for (int i = 0; i < 6; i++)
  {
    buttonPress(i);
  }

  // Rotary Encoder A and B
  readEncoder();

  // Rotary Encoder Button
  static bool presPrevRE = false;
  static unsigned long lastPressedRE = 0;

  // debounce
  if (millis() > lastPressedRE + 50)
  {
    bool press = !digitalRead(EC11D_PIN);
    if (presPrevRE != press)
    {
      presPrevRE = press;
      if (press)
      {
        Consumer_press(RotaryKeypress);
      }
      else
      {
        Consumer_release(RotaryKeypress);
      }
      lastPressedRE = millis();
    }
  }

  // Cycle the hue of the displayed color
  handleColor();
}
