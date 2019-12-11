#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN            6
#define NUMPIXELS      144

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
uint8_t * pxs = pixels.getPixels();

RTC_DS1307 rtc;

void setup() {
  // a couple buttons to allow you to set the time.
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  while(!Serial); // for Leonardo/Micro/Zero
  Serial.begin(57600);
  rtc.begin();
  
  for(int retries=0; retries<100 && !rtc.isrunning(); retries++) {
    Serial.println("waiting for rtc... (It may be browned out - is your USB port providing enough power?");
    delay(100); 
  }
  
  if (!rtc.isrunning()) {
    Serial.println("RTC IS NOT RUNNING!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  } else {
    Serial.println("RTC is running.");
  }
  pixels.begin(); // This initializes the NeoPixel library.
}

void set_color(int pixel, int color, int intensity) {
  int memory_offset = (pixel%NUMPIXELS)*3+color;
  pxs[memory_offset] = intensity;
}

void blend_color(int pixel, int color, int intensity) {
  int memory_offset = (pixel%NUMPIXELS)*3+color;
  pxs[memory_offset] = uint8_t(min(255, pxs[memory_offset]+intensity));
}

#define modmath(a,b) (((a)+(b)+NUMPIXELS)%NUMPIXELS)
float red_float_offset = 18.0;
float green_float_offset = 13.0;
float blue_float_offset = 0.0;
float red_delta = .31;
float green_delta = .23;
float blue_delta = -.17;

int dim(int intensity) {
  return (2 + intensity) / 32;
}

void rainbow() {
  memset(pxs, 0, NUMPIXELS*3);
  red_float_offset += red_delta;
  if(red_float_offset > NUMPIXELS)
    red_float_offset -= NUMPIXELS;
  if(red_float_offset < 0)
    red_float_offset += NUMPIXELS;
  green_float_offset += green_delta;
  if(green_float_offset > NUMPIXELS)
    green_float_offset -= NUMPIXELS;
  if(green_float_offset < 0)
    green_float_offset += NUMPIXELS;
  blue_float_offset += blue_delta;
  if(blue_float_offset > NUMPIXELS)
    blue_float_offset -= NUMPIXELS;
  if(blue_float_offset < 0)
    blue_float_offset += NUMPIXELS;
  for(int i=0; i<NUMPIXELS/2; i++) {
    blend_color(int(red_float_offset) + i, 0, dim(i));
    blend_color(int(red_float_offset) + i + NUMPIXELS/2, 0, dim(NUMPIXELS/2 - i));
    blend_color(int(green_float_offset) + i, 1, dim(i));
    blend_color(int(green_float_offset) + i + NUMPIXELS/2, 1, dim(NUMPIXELS/2 - i));
    blend_color(int(blue_float_offset) + i, 2, dim(i));
    blend_color(int(blue_float_offset) + i + NUMPIXELS/2, 2, dim(NUMPIXELS/2 - i));
  }
}

float second_float = -1;
int loop_delay_base = 22; // 22 w/ no rainbow, 32 with.
float second_float_delta = 0.04;
int loop_delay = loop_delay_base;
long unsigned int when_hour_set_down=0, when_minute_set_down=0;
void loop() {
  float hour_float;
  float minute_float;
  float fractional;
  int now_hour, now_minute, now_second;

  delay(loop_delay);
  DateTime now = rtc.now();  
 
  // Check for hour-adjust button press and update RTC if user is changing it:
  if (digitalRead(3) == 0) {
    if (when_hour_set_down == 0) {
      when_hour_set_down = millis();
    } else {
      if (millis() - when_hour_set_down > 250) {
        when_hour_set_down = millis();
        rtc.adjust(DateTime(2016, 1, 1, (now.hour()+1)%12, now.minute(), 0));
        second_float = 0.0;
        now = rtc.now();
      }
    }
  } else {
    when_hour_set_down = 0;
  }

  // Check for minute-adjust button press and update RTC if user is changing it:
  if (digitalRead(2) == 0) {
    if (when_minute_set_down == 0) {
      when_minute_set_down = millis();
    } else {
      if (millis() - when_minute_set_down > 150) {
        when_minute_set_down = millis();
        rtc.adjust(DateTime(2016, 1, 1, now.hour(), (now.minute()+1)%60, 0));
        second_float = 0.0;
        now = rtc.now();
      }
    }
  } else {
    when_minute_set_down = 0;
  }

  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());
  
  
  if (second_float < 0.0) {
    second_float = now.second();
  }
  second_float += second_float_delta;
  if (second_float >= 60.0) {
    second_float -= 60.0;
  }
  if (now.second() > 0 and now.second() < 59) {
    if ((int)second_float < now.second()) 
      loop_delay = max(loop_delay-1, loop_delay_base-20);
    else if ((int)second_float > now.second()) {
      loop_delay = min(loop_delay+1, loop_delay_base+20);
    } else {
      loop_delay = loop_delay_base;
    }
  }

  now_hour = now.hour();
  now_minute = now.minute();
  now_second = now.second();

  //I have to clear the LEDs that follow the clock "hands" before the rainbow, so 
  //the clears don't interfere with what the rainbow sets.
  hour_float = now_hour%12 + now_minute/60.0;
  float hour_led = hour_float * 12.0;
  set_color(int(hour_led)+NUMPIXELS-2,   0, 0);
  
  minute_float = now_minute + now_second/60.0;
  float minute_led = minute_float * 144.0/60.0;
  set_color(int(minute_led)+NUMPIXELS-2, 2, 0);
  
  float second_led = min(143.99, (second_float) * 144.0/60.0);
  set_color(int(second_led)+NUMPIXELS-2, 1, 0);

  rainbow(); 

  fractional = hour_led - int(hour_led);
  set_color(int(hour_led),               0, 255.0);
  set_color(int(hour_led)+1,             0, 255.0*fractional);
  set_color(int(hour_led)+NUMPIXELS-1,   0, 255.0-255.0*fractional);
  
  fractional = minute_led - int(minute_led);
  set_color(int(minute_led),             2, 255.0);
  set_color(int(minute_led)+1,           2, 255.0*fractional);
  set_color(int(minute_led)+NUMPIXELS-1, 2, 255.0-255.0*fractional);
    
  fractional = second_led - int(second_led); 
  set_color(int(second_led),             1, 255.0);
  set_color(int(second_led)+1,           1, 255.0 * fractional);
  set_color(int(second_led)+NUMPIXELS-1, 1, 255.0-255.0 * fractional);

  pixels.show(); 
}

