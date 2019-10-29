# Arduino NRF24L01 Time Client 
 
Example code showing an Arduino client receiving date/time updates via a NRF24L01 transiever. 
 
It it intended for constrained Arduino style IoT devices that may not be connected to the Internet, and as a consequence is not able
to synchronize time using NTP.
 
A client may get the current date/time every time it boots and use it while operational, or it may use the date/time received to
update an RTC that is part of the configuration. This example shows how to update an RTC on boot.

The date/time being received is in UTC. I assume that most IoT devices would report date/time in UTC and leave it up to some server
code to display information according to the users timezone. If your scenario is different, you must factor in the timezone of the device.

A corresponding server sending date/time updates periodically has been implemented for the Raspberry PI. Since the Raspberry PI
runs a linux operating system it is easily configured to use NTP to keep the clock updated.

The server has been implemented in Python and uses the pigpiod Python client module for accessing the NRF24 module. It requires the
installation of the pigpiod daemon on the Raspberry PI.

Please refer to the following link for further information about the server: 
https://github.com/bjarne-hansen/nrf24-time-server
 
    Wiring:

    I always use the same colour code for the wires connecting the RF24 modules. It seems that the colours below are popular in many
    articles on the RF24 module.

    NRF24L01, PIN side.
    +---+----------------------
    |       *    *
    +---+
    |7|8|   purple |
    +-+-+
    |5|6|   green  |  blue
    +-+-+
    |3|4|   yellow | orange
    +-+-+   
    |1|2|   black  |  red
    +-+-+----------------------
 
    The following layout is for the Arduino NANO.
 
         RF24L01                 NANO
    -------------------------------------
    PIN DESC  COLOR           PIN   GPIO
    1   GND   black   <--->   GND    -
    2   3.3V  red     <--->   3V3    -
    3   CE    yellow  <--->   14     10 
    4   CSN   orange  <--->   13      9 
    5   SCKL  green   <--->   17     13    
    6   MOSI  blue    <--->   15     11 
    7   MISO  purple  <--->   16     12 
    8   IRQ           <--->   N/C    - 
    
      DS3231     NANO
    ----------------------
     GND  <-->   -  GND  
     VCC  <-->   -  3V3
     SDA  <-->  27   A4
     SCL  <-->  28   A5
 
