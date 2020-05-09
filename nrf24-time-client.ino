/**
 * 
 * Example code showing an Arduino client receiving date/time updates via a NRF24L01 transiever. 
 * 
 * It it intended for constrained Arduino style IoT devices that may not be connected to the Internet, and as a consequence is not able
 * to synchronize time using NTP.
 * 
 * A client may get the current date/time every time it boots and use it while operational, or it may use the date/time received to
 * update an RTC that is part of the configuration. This example shows how to update an RTC on boot.
 * 
 * The date/time being received is in UTC. I assume that most IoT devices would report date/time in UTC and leave it up to some server
 * code to display information according to the users timezone. If your scenario is different, you must factor in the timezone of the device.
 *  
 * A corresponding server sending date/time updates periodically has been implemented for the Raspberry PI. Since the Raspberry PI
 * runs a linux operating system it is easily configured to use NTP to keep the clock updated.
 * 
 * The server has been implemented in Python and uses the pigpiod Python client module for accessing the NRF24 module. It requires the
 * installation of the pigpiod daemon on the Raspberry PI.
 * 
 * Please refer to the following link for further information about the server: 
 *   https://...
 * 
 *
 * Wiring:
 *   
 *   I always use the same colour code for the wires connecting the RF24 modules. It seems that the colours below are popular in many
 *   articles on the RF24 module.
 *   
 *   NRF24L01, PIN side.
 *   +---+----------------------
 *   |       *    *
 *   +---+
 *   |7|8|   purple |
 *   +-+-+
 *   |5|6|   green  |  blue
 *   +-+-+
 *   |3|4|   yellow | orange
 *   +-+-+   
 *   |1|2|   black  |  red
 *   +-+-+----------------------
 *   
 *   The following layout is for the Arduino NANO.
 *   
 *           NRF24L01            NANO
 *  -------------------------------------
 *  PIN DESC  COLOR           PIN   GPIO
 *  1   GND   black   <--->   GND    -
 *  2   3.3V  red     <--->   3V3    -
 *  3   CE    yellow  <--->   14     10 
 *  4   CSN   orange  <--->   13      9 
 *  5   SCKL  green   <--->   17     13    
 *  6   MOSI  blue    <--->   15     11 
 *  7   MISO  purple  <--->   16     12 
 *  8   IRQ           <--->   N/C    - 
 * 
 *  
 *   DS3231       NANO
 *  ----------------------
 *    GND  <-->   -  GND  
 *    VCC  <-->   -  3V3
 *    SDA  <-->  27   A4
 *    SCL  <-->  28   A5  
 */

#include <printf.h>
#include <RF24.h>
#include <TimeLib.h>
#include <DS3232RTC.h>

#define DEBUG_PRINT

#define PIN_LED        8
#define PIN_RF24_CSN   9
#define PIN_RF24_CE   10

RF24 radio(PIN_RF24_CE, PIN_RF24_CSN);  // RF24 radio.

byte rx_addr_1[6] = "DTCLI";            // Address to listen to. 
byte payload[32];                       // Buffer for payload.

time_t last_sync;

void setup() {

  debug_begin();  
  debug_println("\n\nInitialising nrf24-time-client.");

  // Initialise LED.
  debug_println("Initialize LED ...");
  pinMode(PIN_LED, OUTPUT);         // LED pin is output.
  digitalWrite(PIN_LED, LOW);       // Turn off LED.
  flash_led(3, 800, 200);

  debug_println("Initialise RTC module ...");  
  rtc_setup();
  
  // Setup RF24 module.
  debug_println("Configure RF24 module ...");
  nrf24_client_setup();

  // Receive date/time being broadcastet
  debug_println("Receiving date/time from server ...");
  nrf24_receive_date_time();
  last_sync = RTC.get();

  // We are done initialising ...
  flash_led(3, 200, 800);
}

void loop() 
{

  // Get the current date/time from RTC and
  time_t dt;
  dt = RTC.get();

  // Print the current date/time.
  debug_print("The time is now: ");
  debug_print(dt);
  debug_print(" (");
  debug_print(dayStr(weekday(dt)));
  debug_println(").");

  debug_print("Minutes since last sync.: "); debug_println((int)numberOfMinutes(dt - last_sync));

  // Wait for 5 seconds.
  delay(5000);  
}


//
// NRF24 functions
//

void nrf24_client_setup()
{
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);        
  radio.setAutoAck(true);               
  radio.setDataRate(RF24_250KBPS);      
  radio.setChannel(100);                  
  radio.enableDynamicPayloads();
  radio.setPayloadSize(sizeof(payload));             
  radio.setCRCLength(RF24_CRC_16);      
  radio.setRetries(5, 15);              
  radio.openReadingPipe(1, rx_addr_1);  
  radio.stopListening();  
}

boolean nrf24_receive(int timeout)
{
  unsigned long started;
  boolean timed_out;
  boolean result;
  
  radio.startListening();
    
  // Check if data is available, or  we pass the timeout.
  timed_out = false;
  started = millis();
  while (!radio.available())
  {
    if (millis() - started > timeout)
    {
      timed_out = true;
      break;
    }
  }

  if (!timed_out)
  {
    // Data available ... read it ...
    radio.read(&payload, sizeof(payload));
    result = true;
  }
  else
    result = false;   

  radio.stopListening();
  return result;
}

void nrf24_receive_date_time()
{
  unsigned int count = 0;
  
  while (true)
  {
    if (nrf24_receive(1000))
    {
      if (rtc_set_time_from_payload())
      {
        debug_print('+');
        break;
      }
      else
      {
        debug_print('-');  
        flash_led(5, 10, 10);
      }
    }
    else
    {
      debug_print('.');
      flash_led(1, 50, 50);
    }
    count++;
    if (count == 30)
    {
      debug_println();
      count = 0;
    }
  }
  debug_println();
}


//
// RTC functions
//
void rtc_setup()
{
  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);
}

boolean rtc_set_time_from_payload()
{

  tmElements_t tm;
  time_t result;

  // Check the signature 0xfe followed by "TIME" (0x54, 0x49, 0x4d, 0x45)
  if (payload[0] == 0xfe && payload[1] == 0x54 && payload[2] == 0x49 && payload[3] == 0x4d && payload[4] == 0x45) 
  {
    
    // Extract date/time data from payload.
    tm.Year = (payload[6] << 8 | payload[5]) - 1970;
    tm.Month = payload[7];
    tm.Day = payload[8];
    tm.Hour = payload[9];
    tm.Minute = payload[10];
    tm.Second = payload[11];
    tm.Wday = payload[12] % 7 + 1;
    
    // Create time_t structure from date/time elements.
    result = makeTime(tm);

    // Update the RTC clock with date/time.
    RTC.set(result);
    
    return true;
  }
  else
  {
    return false;
  }
}


//
// Utilities for flashing LED.
//

void flash_led(int count, int ms)
{
  for (int i = 0; i < count; i++)
  {
    digitalWrite(PIN_LED, HIGH);
    delay(ms);
    digitalWrite(PIN_LED, LOW);
    delay(ms);      
  }  
}

void flash_led(int count, int onms, int offms)
{
  for (int i = 0; i < count; i++)
  {
    digitalWrite(PIN_LED, HIGH);
    delay(onms);
    digitalWrite(PIN_LED, LOW);
    delay(offms);      
  }    
}

//
// Functions for debugging low-power NRF24L01 solutions.
//

void debug_begin()
{
  #ifdef DEBUG_PRINT
  Serial.begin(9600);
  #endif
  #ifdef __PRINTF_H__
  printf_begin();
  #endif
  
}

void debug_print(byte n)
{
  #ifdef DEBUG_PRINT
  Serial.print(n);
  #endif
}

void debug_print(char n)
{
  #ifdef DEBUG_PRINT
  Serial.print(n);
  #endif
}

void debug_print(short n)
{
  #ifdef DEBUG_PRINT
  Serial.print(n);
  #endif
}

void debug_print(int n)
{
  #ifdef DEBUG_PRINT
  Serial.print(n);
  #endif
}

void debug_print(unsigned int n)
{
  #ifdef DEBUG_PRINT
  Serial.print(n);
  #endif
}

void debug_print(long n)
{
  #ifdef DEBUG_PRINT
  Serial.print(n);
  #endif
}


void debug_print(float n)
{
  #ifdef DEBUG_PRINT
  Serial.print(n);
  #endif
}

void debug_print(double n)
{
  #ifdef DEBUG_PRINT
  Serial.print(n);
  #endif
}

void debug_print(const char* s)
{
  #ifdef DEBUG_PRINT
  Serial.print(s);
  #endif    
}

void debug_println()
{
  #ifdef DEBUG_PRINT
  Serial.println();
  #endif  
}

void debug_println(byte n)
{
  #ifdef DEBUG_PRINT
  Serial.println(n);
  #endif    
}

void debug_println(char n)
{
  #ifdef DEBUG_PRINT
  Serial.println(n);
  #endif    
}

void debug_println(short n)
{
  #ifdef DEBUG_PRINT
  Serial.println(n);
  #endif    
}

void debug_println(int n)
{
  #ifdef DEBUG_PRINT
  Serial.println(n);
  #endif    
}

void debug_println(unsigned int n)
{
  #ifdef DEBUG_PRINT
  Serial.println(n);
  #endif    
}

void debug_println(long n)
{
  #ifdef DEBUG_PRINT
  Serial.println(n);
  #endif    
}

void debug_println(float n)
{
  #ifdef DEBUG_PRINT
  Serial.println(n);
  #endif    
}

void debug_println(double n)
{
  #ifdef DEBUG_PRINT
  Serial.println(n);
  #endif    
}

void debug_println(const char* s)
{
  #ifdef DEBUG_PRINT
  Serial.println(s);
  #endif  
}

#ifdef __RF24_H__
void debug_nrf24_details(RF24 r)
{
  #ifdef __PRINTF_H__
  r.printDetails();
  #endif
}
#endif

#ifdef _Time_h
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
#endif
