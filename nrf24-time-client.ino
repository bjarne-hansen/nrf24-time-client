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
#include <nRF24L01.h>
#include <RF24_config.h>
#include <RF24.h>

#include <TimeLib.h>

#include <DS3232RTC.h>

RF24 radio(9, 10);          // CSN, CE pins
byte txaddr[6] = "DTP01";   // Address of date/time server.
byte rxaddr[6] = "DTCLN";   // Client address.
byte payload[10];           // Payload being received.

short a;
unsigned short b;
long long c = 123456789;

void setup() {
  Serial.begin(9600);
  printf_begin();
  
  Serial.println("\n\nExample date/time client.");
  Serial.println(sizeof(c));
  // Setup RF24 module for receiving date/time.
  Serial.println("Configure RF24 module ...");
  setupDateTimeClient();

  // Receive date/time being broadcastet
  Serial.println("Receiving date/time from server ...");
  time_t dt = receiveDateTime();

  // Update the RTC with current date/time
  setupRTC();
  updateRTCDateTime(dt);
}

void loop() 
{

  // Get the current date/time from RTC and
  time_t dt;
  dt = RTC.get();
  Serial.print("The time is now: ");
  printDateTime(dt);
  Serial.println();
 
  delay(5000);  
}

void setupRTC()
{
  // Setup RTC with basic defaults.
  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);
}

void updateRTCDateTime(time_t dt)
{
  // Set RTC clock to the specified date/time.
  RTC.set(dt);  
}

void setupDateTimeClient() 
{
  radio.begin();  
  radio.setPALevel(RF24_PA_MAX);        // Maximum transmission power.
  radio.setAutoAck(true);               // Automatic acknowledgement.
  
  radio.setDataRate(RF24_250KBPS);      // Default: RF24_1MBPS.   Value: RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  radio.setChannel(1);                  // Default: 76            Minimum 1. Maximum 125.  
  radio.setPayloadSize(10);             // Default: 32.           Maximum 32 bytes. 0 means dynamic payload size.
  radio.setCRCLength(RF24_CRC_16);      // Default: RF24_CRC_16.  Other values: RF24_CRC_DISABLED = 0, RF24_CRC_8
  radio.setRetries(5, 15);              // Default: 5, 15         5 usec delay, 15 retries.
  radio.openWritingPipe(txaddr);
  radio.openReadingPipe(1, rxaddr);
  radio.stopListening();
  
  Serial.println("RF24 Details:");
  radio.printDetails();  
}


time_t receiveDateTime() 
{
  tmElements_t tm;
  time_t result;
  boolean timed_out = false;
  unsigned long started;
  int timeouts = 0;

  // Start RF24 radio listining ...
  radio.startListening();
  
  while (true) 
  {

    // Prepare checking if data has been received.
    timed_out = false;
    started = millis();

    // Check if data has been received. Timeout after 200 ms.
    while (!radio.available()) 
    {
      if (millis() - started > 200) 
      {
          timed_out = true;
          break;
      }
    }

    if (!timed_out) 
    {
      // Data has been received, so copy them into payload buffer.
      radio.read(&payload, sizeof(payload));

      // Check the signature "tim" (0x74, 0x69, 0x6d)
      if (payload[0] == 0x74 && payload[1] == 0x69 && payload[2] == 0x6d) 
      {

        // Create date/time structure.
        tm.Year = (payload[3] << 8 | payload[4]) - 1970;
        tm.Month = payload[5];
        tm.Day = payload[6];
        tm.Hour = payload[7];
        tm.Minute = payload[8];
        tm.Second = payload[9];

        // Make time_t
        result = makeTime(tm);
        
        Serial.print("\nReceived: "); printDateTime(result);
        Serial.println();

        // Return result.
        return result;    
      } 
      else
      {
        Serial.println("\nInvalid payload received.");
      }
    }
    else
    {
      Serial.print(".");
      timeouts++;
      if (timeouts >= 25)
      {
        Serial.println();
        timeouts = 0;
      }
    }
  } 
} 

void printDateTime(time_t dt)
{
  Serial.print(year(dt)); Serial.print("-");
  Serial.print((month(dt) < 10) ? "0" : "");  Serial.print(month(dt));  Serial.print("-");
  Serial.print((day(dt) < 10) ? "0" : "");    Serial.print(day(dt));    Serial.print(" ");
  Serial.print((hour(dt) < 10) ? "0" : "");   Serial.print(hour(dt));   Serial.print(':');
  Serial.print((minute(dt) < 10) ? "0" : ""); Serial.print(minute(dt)); Serial.print(':');
  Serial.print((second(dt) < 10) ? "0" : ""); Serial.print(second(dt));  
}
