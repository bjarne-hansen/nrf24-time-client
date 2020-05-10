//
// Functions for debugging low-power NRF24L01 solutions.
//
#ifndef _debug_util_h
#define _debug_util_h

#ifdef DEBUG_PRINT
  #ifdef __RF24_H__
    #define debug_init() Serial.begin(9600); printf_begin();
  #else
    #define debug_init() Serial.begin(9600);
  #endif
  #define debug(s) debug_print(s);
  #define debugln(s) debug_println(s);
#else
  #define debug_init()
  #define debug(s) 
  #define debugln(s)
#endif

void debug_begin();
void debug_print(byte n);
void debug_print(char n);
void debug_print(short n);
void debug_print(int n);
void debug_print(unsigned int n);
void debug_print(long n);
void debug_print(float n);
void debug_print(double n);
void debug_print(const char* s);

void debug_println();
void debug_println(byte n);
void debug_println(char n);
void debug_println(short n);
void debug_println(int n);
void debug_println(unsigned int n);
void debug_println(long n);
void debug_println(float n);
void debug_println(double n);
void debug_println(const char* s);

#ifdef __RF24_H__
void debug_println(RF24 r);
#endif

#ifdef _Time_h
void debug_print(time_t t);
void debug_println(time_t t);
#endif

#endif _debug_util_h
