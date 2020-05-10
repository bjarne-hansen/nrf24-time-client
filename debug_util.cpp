//
// Functions for debugging low-power NRF24L01 solutions.
//
#include <Arduino.h>
#include <RF24.h>
#include <TimeLib.h>

void debug_print(byte n)
{
  Serial.print(n);
}

void debug_print(char n)
{
  Serial.print(n);
}

void debug_print(short n)
{
  Serial.print(n);
}

void debug_print(int n)
{
  Serial.print(n);
}

void debug_print(unsigned int n)
{
  Serial.print(n);
}

void debug_print(long n)
{
  Serial.print(n);
}

void debug_print(float n)
{
  Serial.print(n);
}

void debug_print(double n)
{
  Serial.print(n);
}

void debug_print(const char* s)
{
  Serial.print(s);
}

void debug_println()
{
  Serial.println();
}

void debug_println(byte n)
{
  Serial.println(n);
}

void debug_println(char n)
{
  Serial.println(n);
}

void debug_println(short n)
{
  Serial.println(n);
}

void debug_println(int n)
{
  Serial.println(n);
}

void debug_println(unsigned int n)
{
  Serial.println(n);
}

void debug_println(long n)
{
  Serial.println(n);
}

void debug_println(float n)
{
  Serial.println(n);
}

void debug_println(double n)
{
  Serial.println(n);
}

void debug_println(const char* s)
{
  Serial.println(s);
}

void debug_println(RF24 r)
{
  r.printDetails();
}

void debug_print(time_t t)
{
    // Print year-month-day.
    debug_print(year(t)); debug_print("-");
    debug_print(month(t) < 10 ? "0" : ""); debug_print(month(t)); debug_print("-");
    debug_print(day(t) < 10 ? "0" : ""); debug_print(day(t)); 

    debug_print(" ");
    
    // Print time.
    debug_print(hour(t) < 10 ? "0" : ""); debug_print(hour(t)); debug_print(':');
    debug_print(minute(t) < 10 ? "0" : ""); debug_print(minute(t)); debug_print(':');
    debug_print(second(t) < 10 ? "0" : ""); debug_print(second(t));    
}

void debug_println(time_t t)
{
  debug_print(t);
  debug_println();
}
