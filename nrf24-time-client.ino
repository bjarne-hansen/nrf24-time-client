/**
 * 
 * Example code showing an Arduino client receiving date/time updates via a NRF24L01 transiever. 
 * 
 * This code has been pubished on:
 *   https://github.com/bjarne-hansen/nrf24-time-client
 * 
 * It it intended for constrained Arduino style IoT devices that may not be connected to the Internet, 
 * and as a consequence is not able to synchronize time using NTP.
 * 
 * A client may get the current date/time every time it boots and use it while operational, or it may 
 * use the date/time received to update an RTC that is part of the configuration. This example shows how to 
 * update an RTC on boot.
 * 
 * The date/time being received is in UTC. I assume that most IoT devices would report date/time in UTC and 
 * leave it up to some server code to display information according to the users timezone. If your scenario 
 * is different, you must factor in the timezone of the device.
 *  
 * A corresponding server sending date/time updates periodically has been implemented for the Raspberry PI. 
 * Since the Raspberry PI runs a linux operating system it is easily configured to use NTP to keep the clock 
 * updated.
 * 
 * The server has been implemented in Python and uses the pigpiod Python client module for accessing the NRF24 
 * module. It requires the installation of the pigpiod daemon on the Raspberry PI.
 * 
 * Please refer to the following link for further information about the server: 
 *   https://github.com/bjarne-hansen/nrf24-time-server
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

#include "debug_util.h"
#include "led_util.h"

#define PIN_LED        8
#define PIN_RF24_CSN   9
#define PIN_RF24_CE   10

RF24 radio(PIN_RF24_CE, PIN_RF24_CSN);  // RF24 radio.
byte rx_addr_1[6] = "DTCLI";            // Address to listen to. 
byte payload[32];                       // Buffer for payload.

time_t last_sync;                       // Time for last synchronisation.

void setup() {
  
  // Initialise ...
  debug_init();  
  debugln("\n\nInitialising nrf24-time-client.");
  
  // Initialise LED.
  debugln("Initialise LED ...");
  pinMode(PIN_LED, OUTPUT);         // LED pin is output.
  digitalWrite(PIN_LED, LOW);       // Turn off LED.
  flash_led(PIN_LED, 3, 800, 200);

  debugln("Initialise RTC module ...");  
  rtc_setup();
  
  // Setup RF24 module.
  debugln("Configure RF24 module ...");
  nrf24_setup();
  debug_println(radio);
  
  // Receive date/time being broadcastet
  debugln("Receiving date/time from server ...");
  nrf24_receive_date_time();
  last_sync = RTC.get();

  // We are done initialising ...
  flash_led(PIN_LED, 3, 200, 800);
}

void loop() 
{

  // Get the current date/time from RTC and
  time_t dt;
  dt = RTC.get();

  // Print the current date/time.
  debug("The time is now: ");
  debug(dt);
  debug(" (");
  debug(dayStr(weekday(dt)));
  debugln(").");

  debug("Minutes since last sync.: "); 
  debugln((int)numberOfMinutes(dt - last_sync));

  // Wait for 5 seconds.
  delay(5000);  
}


//
// NRF24 functions
//

void nrf24_setup()
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
        debug('+');
        break;
      }
      else
      {
        debug('-');  
        flash_led(PIN_LED, 5, 10, 10);
      }
    }
    else
    {
      debug('.');
      flash_led(PIN_LED, 1, 50, 50);
    }
    count++;
    if (count == 30)
    {
      debug_println();
      count = 0;
    }
  }
  debugln();
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
