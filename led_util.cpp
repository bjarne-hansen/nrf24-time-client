//
// Utilities for flashing LED.
//
#include <Arduino.h>

void flash_led(int pin, int count, int ms)
{
  for (int i = 0; i < count; i++)
  {
    digitalWrite(pin, HIGH);
    delay(ms);
    digitalWrite(pin, LOW);
    delay(ms);      
  }  
}

void flash_led(int pin, int count, int onms, int offms)
{
  for (int i = 0; i < count; i++)
  {
    digitalWrite(pin, HIGH);
    delay(onms);
    digitalWrite(pin, LOW);
    delay(offms);      
  }    
}
