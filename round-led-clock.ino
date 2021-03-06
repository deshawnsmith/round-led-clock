/*
    This is a WiFi connected round LED Clock based on work by Leon van den Beukel.

    This software gets NTP time from the Internet and translates the time to hours,
    minutes, and seconds that will be displayed on a 60 RGB WS2812B LED strip.

*/

/*
    Add the following lines into a settings.h file and place it in the same directory as this file:
    const char ssid[] = "YourWiFiSSID";
    const char passwd[] = "YourWiFiPassword";
    .gitignore will ignore this file, so it won't exist when you first clone the repo
*/
#include "settings.h"     
#include <FastLED.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const int NTP_PACKET_SIZE = 48;     // NTP time is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming & outgoing NTP packets
WiFiUDP Udp;
// Change the following to an NTP server near you.
const char NTPServerName[] = "0.us.pool.ntp.org"; // Find your server name at https://www.pool.nep.org/en/
unsigned int localPort = 8888;      // Local UDP port for connecting to NTP server

time_t prevDisplay = 0;             // Helps only refresh the display every second

// See JChristensen's Timezone library to set up your own time change rules
// Daylight Saving Time in the Mountain time zone starts on the second Sunday of March at 2am and offsets
// the time by -360 minutes (-6 hours) from UTC
TimeChangeRule mdtRule = {"MDT", Second, Sun, Mar, 2, -360}; 
// Standard time in the Mountain time zone starts on the first Sunday of November at 2am and offsets
// the time by -420 minutes (-7 hours) from UTC
TimeChangeRule mstRule = {"MST", First, Sun, Nov, 2, -420};
// Set up the US Mountain time zone with the daylight saving and standard time change rules
Timezone usMountain(mdtRule, mstRule);

// To simulate the slow movement of the hour hand between hours, set this to true
#define MOVE_HOUR_HAND_BETWEEN_HOURS true

#define SIGNAL_PIN D6                // Output signal for LED array

// Variables for the LED array
#define LED_COUNT 60
CRGB LEDArray[LED_COUNT];

// These are the colors that will be used for the hour hand, minute hand, and second hand
// as well as the values that will be used when these hands "overlap" in the LED clock.
const CRGB HOUR_HAND_COLOR = CRGB::Red;
const CRGB MINUTE_HAND_COLOR = CRGB::Green;
const CRGB SECOND_HAND_COLOR = CRGB::Blue;
const CRGB HOUR_AND_MINUTE_COLOR = CRGB::Yellow;
const CRGB HOUR_AND_SECOND_COLOR = CRGB::Magenta;
const CRGB MINUTE_AND_SECOND_COLOR = CRGB::Cyan;
const CRGB ALL_HANDS_COLOR = CRGB::White;

// If your clock is too bright or you want to turn it off at night, you can use the following Variables
// to dim the clock
const unsigned short DIM_START_HOUR = 22;   // Start dimming the clock at 10pm
const unsigned short DIM_END_HOUR = 7;  // Stop dimming the clock at 7am
const unsigned short DIM_PERCENTAGE = 20;   // The brightness of the clock is between 0% (all off) and 100% (full on)

void setup() {
    // Set up the LED array
    FastLED.delay(3000);
    FastLED.addLeds<WS2812B, SIGNAL_PIN, GRB>(LEDArray, LED_COUNT);

    // Start up WiFi
    WiFi.begin(ssid, passwd);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    // Start up UDP
    Udp.begin(localPort);

    // Set up the time synchronization provider
    setSyncProvider(getNtpTime);
    setSyncInterval(14400);         // Synchronize with NTP every 4 hours.
}

void loop() {
    if (timeStatus() != timeNotSet) {
        if(now() != prevDisplay) {  // update the display only if time has changed
            displayTimeOnLEDs();
        }
    }
}

void displayTimeOnLEDs() {
    int secondIndex = getIndexOfMinuteOrSecond(second());
    int minuteIndex = getIndexOfMinuteOrSecond(minute());
    int hourIndex = getIndexOfHour(hour(), minute());

    for(int i = 0; i < LED_COUNT; i++) {
        LEDArray[i] = CRGB::Black;
    }

    // Set the colors on the LED array to show the time.
    LEDArray[secondIndex] = SECOND_HAND_COLOR;
    LEDArray[minuteIndex] = MINUTE_HAND_COLOR;
    LEDArray[hourIndex] = HOUR_HAND_COLOR;

    if(secondIndex == minuteIndex) {
        LEDArray[minuteIndex] = MINUTE_AND_SECOND_COLOR;
    }
    if(minuteIndex == hourIndex) {
        LEDArray[hourIndex] = HOUR_AND_MINUTE_COLOR;
    }
    if(secondIndex == hourIndex) {
        LEDArray[hourIndex] = HOUR_AND_SECOND_COLOR;
    }
    if(secondIndex == minuteIndex && secondIndex == hourIndex) {
        LEDArray[hourIndex] = ALL_HANDS_COLOR;
    }

    // Set the appropriate brightness for the time of day
    if(hour() >= DIM_START_HOUR || hour() < DIM_END_HOUR)
    {
        FastLED.setBrightness(255 * DIM_PERCENTAGE / 100);
    }
    else 
    {
        FastLED.setBrightness(255);
    }

    FastLED.show();
}

byte getIndexOfMinuteOrSecond(byte minuteOrSecond) {
    if (minuteOrSecond < 30) {
        return minuteOrSecond + 30;
    } else {
        return minuteOrSecond - 30;
    }
}

byte getIndexOfHour(byte hour, byte minute) {
    if (hour > 12) {
        hour -= 12;
    }

    byte hourLED;
    if( hour < 6) {
        hourLED = (hour * 5) + 30;
    } else {
        hourLED = (hour * 5) - 30;
    }

    if (MOVE_HOUR_HAND_BETWEEN_HOURS) {
        hourLED += minute / 12;
    }
    return hourLED;
}

time_t getNtpTime() {
    IPAddress ntpServerIp;  // NTP server's ip address

    while (Udp.parsePacket() > 0) ; // discard any previously received packets
    WiFi.hostByName(NTPServerName, ntpServerIp);
    sendNTPpacket(ntpServerIp);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500) {
        int size = Udp.parsePacket();
        if (size >= NTP_PACKET_SIZE) {
            Udp.read(packetBuffer, NTP_PACKET_SIZE);    // read packet into the packetBuffer
            unsigned long secsSince1900;
            // convert four bytes starting at location 40 to a long integer
            secsSince1900 = (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];
            return usMountain.toLocal(secsSince1900 - 2208988800UL);
        }
    }
    return 0;
}

void sendNTPpacket(IPAddress &address) {
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;            // Stratum, or type of clock
    packetBuffer[2] = 6;            // Polling interval
    packetBuffer[3] = 0xEC;         // Peer Clock Precision
    // 8 bytes of zero for Root Delay and Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp
    Udp.beginPacket(address, 123);  // NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}
