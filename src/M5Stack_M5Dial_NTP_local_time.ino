/*
* M5Dial_NTP_quality_check.ino
* Version 1.0
*
* Example by Microsoft CoPilot
* after my question to CoPilot: c++ how to know if time from NTP is correct.
* Added some functionality to run on a M5Stack MDial
*/

#include <M5Dial.h>
#include <WiFi.h>
#include <TimeLib.h>
#include <stdlib.h>  // for putenv
#include <time.h>
#include <DateTime.h>  // See: /Arduino/libraries/ESPDateTime/src
#include <M5GFX.h>

#include <driver/adc.h>
#include "secret.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <string>
#include <iomanip>      // For setFill and setW
#include <cstring>      // For strcpy
#include <NTPClient.h>  // Include your NTP client library
#include <WiFiUdp.h>

#define WIFI_SSID SECRET_SSID                       // "YOUR WIFI SSID NAME"
#define WIFI_PASSWORD SECRET_PASS                   //"YOUR WIFI PASSWORD"
#define NTP_TIMEZONE SECRET_NTP_TIMEZONE            // for example: "Europe/Lisbon"
#define NTP_TIMEZONE_CODE SECRET_NTP_TIMEZONE_CODE  // for example: "WET0WEST,M3.5.0/1,M10.5.0"
#define NTP_SERVER1 SECRET_NTP_SERVER_1             // for example: "0.pool.ntp.org"
#define NTP_SERVER2 "1.pool.ntp.org"
#define NTP_SERVER3 "2.pool.ntp.org"

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, NTP_SERVER1, 3600, 60000);  // Adjust the time offset as needed

int dw;
int dh;
// M5Dial screen 1.28 Inch 240x240px. Display device: GC9A01
static constexpr const int hori[] = { 0, 60, 120, 180, 220 };
static constexpr const int vert[] = { 0, 60, 120, 180, 220 };  // was: {30, 60, 90}
static constexpr const char* const wd[7] = {"Sun", "Mon", "Tue", "Wed",
                                            "Thu", "Fri", "Sat"};
volatile bool buttonPressed = false;
struct tm unixTime_tm;
std::tm* tm_local;
bool rtc_enabled = false;

int calc_x_offset(const char* t, int ch_width_in_px) {
  int le = strlen(t);
  int char_space = 1;
  int ret = (dw - ((le * ch_width_in_px) + ((le - 1) * char_space))) / 2;
  /*
  std::cout 
    << "calc_x_offset(\"" 
    << (t) 
    << "\", "
    << (pw)
    << "): ret = (" 
    << (dw) 
    << " - ((" 
    << (le) 
    << " * " 
    << (pw) 
    << ") + (("
    << (le-1) 
    << " * "
    << (char_space) 
    << ") )) / 2 = "
    << (ret) 
    << std::endl;
    */
  return (ret < 0) ? 0 : ret;
}

void setRTC(struct tm timeinfo) {
  std::cout << "\nsetRTC(): value received param timeinfo: " << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") << std::endl;
  M5Dial.Rtc.setDateTime({ { timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday }, { timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec } });
  std::cout << "System Time (built-in RTC) synchronized with NTP time\n" << std::endl;
}

void start_scrn(void) {
  static const char* txt[] = { "NTP qual ck", "by Paulus", "Github", "@PaulskPt" };
  static const int char_width_in_pixels[] = { 12, 12, 12, 14 };
  int vert2[] = { 0, 60, 90, 120, 150 };
  int x = 0;

  M5Dial.Display.clear();
  M5Dial.Display.setTextColor(RED, BLACK);
  //M5Dial.Display.setFont(&fonts::FreeSans18pt7b);

  for (int i = 0; i < 4; ++i) {
    x = calc_x_offset(txt[i], char_width_in_pixels[i]);
    // std::cout << "start_srcn(): x = " << std::to_string(x) << std::endl;
    M5Dial.Display.setCursor(x, vert2[i + 1]);
    M5Dial.Display.println(txt[i]);
  }

  //delay(5000);
  M5Dial.Display.setTextColor(YELLOW, BLACK);
  //M5Dial.Display.setFont(&fonts::FreeSans12pt7b); // was: efontCN_14);
}

void getID(void) {
  uint64_t chipid_EfM = ESP.getEfuseMac();  // The chip ID is essentially the MAC address
  char chipid[13] = { 0 };
  sprintf(chipid, "%04X%08X", (uint16_t)(chipid_EfM >> 32), (uint32_t)chipid_EfM);
  std::cout << "\nESP32 Chip ID = " << chipid << std::endl;
  std::cout << "chipid mirrored (same as M5Burner MAC): " << std::flush;
  // Mirror MAC address:
  for (uint8_t i = 10; i >= 0; i -= 2)  // 10, 8. 6. 4. 2, 0
  {
    // bytes 10, 8, 6, 4, 2, 0
    // bytes 11, 9, 7. 5, 3, 1
    std::cout << chipid[i] << chipid[i + 1] << std::flush;
    if (i > 0)
      std::cout << ":" << std::flush;
    if (i == 0)  // Note: this needs to be here. Yes, it is strange but without it the loop keeps on running.
      break;     // idem.
  }
}

bool ck_Btn() {
  M5Dial.update();
  //if (M5Dial.BtnA.isPressed())
  if (M5Dial.BtnA.wasPressed())  // 100 mSecs
  {
    buttonPressed = true;
    return true;
  }
  return false;
}

time_t ntpToTimeT(uint32_t ntpTime) {
  // NTP epoch starts on January 1, 1900
  // Unix epoch starts on January 1, 1970
  const uint32_t seventyYears = 0UL;  // 2208988800UL;
  return ntpTime - seventyYears;
}

struct tm ntpToTm(uint32_t ntpTime) {
  // NTP epoch starts on January 1, 1900
  // Unix epoch starts on January 1, 1970
  const uint32_t seventyYears = 2208988800UL;
  //time_t unixTime = static_cast<time_t>(ntpTime - seventyYears);
  time_t unixTime = static_cast<time_t>(ntpTime);  // - seventyYears);

  // Convert time_t to struct tm
  struct tm timeInfo;
  gmtime_r(&unixTime, &timeInfo);  // Use gmtime_r(&unixTime, &timeInfo) on POSIX systems

  return timeInfo;
}

void setup() {
  // put your setup code here, to run once:
  M5.begin();

  /*
  * A workaround to prevent some problems regarding 
  * M5Dial.BtnA.IsPressed(), M5Dial.BtnA.IsPressedPressed(), 
  * M5Dial.BtnA,wasPressed() or M5Dial.BtnA.pressedFor(ms).
  * See: https://community.m5stack.com/topic/3955/atom-button-at-gpio39-without-pullup/5
  */
  adc_power_acquire();  // ADC Power ON

  M5Dial.Display.init();
  dw = M5Dial.Display.width();
  dh = M5Dial.Display.height();
  M5Dial.Display.setRotation(0);
  M5Dial.Display.setTextColor(YELLOW, BLACK);

  M5Dial.Display.setColorDepth(1);                 // mono color
  M5Dial.Display.setFont(&fonts::FreeSans12pt7b);  // was: efontCN_14);
  M5Dial.Display.setTextWrap(false);
  M5Dial.Display.setTextSize(1);

  Serial.begin(115200);

  std::cout << "\n\nM5Stack M5Dial NTP quality check test V1.0." << std::endl;

  getID();

  std::cout << "M5Stack M5Dial display width = " << std::to_string(dw) << ", height = " << std::to_string(dh) << std::endl;


  if (!M5Dial.Rtc.isEnabled())
  {
    std::cout << "setup(): RTC not found." << std::endl;
  }
  else
  {
    rtc_enabled = true;
  }

  start_scrn();

  // Connect to Wi-Fi
  std::cout << "Connecting to: " << (WIFI_SSID) << std::endl;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    std::cout << "." << std::flush;
  }
  std::cout 
    << std::endl 
    << "WiFi connected.\n" 
    << "IP address: " 
    << WiFi.localIP() 
    << std::endl;

  // Initialize NTPClient
  ntpClient.begin();
}

void calc_diff() {
  // put your main code here, to run repeatedly:
  ntpClient.update();
  unsigned long ntpEpoch = ntpClient.getEpochTime();  // Get time from NTP server
  time_t unixTime_t = ntpToTimeT(ntpEpoch);
  unixTime_tm = ntpToTm(ntpEpoch);

  // Get system time
  auto systemTime = std::chrono::system_clock::now();
  std::time_t systemTimeT = std::chrono::system_clock::to_time_t(systemTime);

  // Calculate offset
  auto offset = std::difftime(unixTime_t, systemTimeT);

  std::cout << "\ncalc_diff(): NTP Time:    " << std::ctime(&unixTime_t);
  std::cout << "             System Time: " << std::ctime(&systemTimeT);
  std::cout << "             Offset:      " << offset << " second(s)" << std::endl;

  if (std::abs(offset) < 2)  // Check if offset is within 2 seconds
  {
    std::cout << "Time is synchronized.\n" << std::endl;
  } else {
    std::cout << "Time is not synchronized.\n" << std::endl;
    // Synchronize the system clock
    if (rtc_enabled) {
      setRTC(unixTime_tm);
    } else {
      std::cout << "Built-in RTC is not enabled. Unable to set it." << std::endl;
    }
  }
}

void poll_RTC(void)
{
  std::shared_ptr<std::string> TAG = std::make_shared<std::string>("poll_RTC(): ");
  time_t t = time(NULL);
  delay(500);

  /// ESP32 internal timer
  //auto dt = M5Dial.Rtc.getDateTime();

  t = std::time(nullptr);
  tm_local = localtime(&t);
  std::cout << std::dec << *TAG << "ESP32 local time: " 
    << std::setw(4)                      << (tm_local->tm_year+1900) << "-"
    << std::setfill('0') << std::setw(2) << (tm_local->tm_mon+1)     << "-"
    << std::setfill('0') << std::setw(2) << (tm_local->tm_mday)      << " ("
    << wd[tm_local->tm_wday] << ") "
    << std::setfill('0') << std::setw(2) << (tm_local->tm_hour)      << ":"
    << std::setfill('0') << std::setw(2) << (tm_local->tm_min)       << ":"
    << std::setfill('0') << std::setw(2) << (tm_local->tm_sec) << std::endl;

}

void disp_time(void)
{
  struct tm my_timeinfo;
  if(!getLocalTime(&my_timeinfo))
  {
    std::cout << "Failed to obtain time" << std::endl;
    return;
  }
  int disp_data_delay = 1000;
  // std::cout << "disp_time(): parameter struct tm my_timeinfo = " << std::put_time(&my_timeinfo, "%Y-%m-%d %H:%M:%S") << std::endl;
  // =========== 1st view =================
  M5Dial.Display.clear();
  M5Dial.Display.setCursor(hori[1], vert[1]+5);
  M5Dial.Display.print(&my_timeinfo, "%A");  // Day of the week
  M5Dial.Display.setCursor(hori[1], vert[2]-2);
  M5Dial.Display.print(&my_timeinfo, "%B %d");
  M5Dial.Display.setCursor(hori[1], vert[3]-10);
  M5Dial.Display.print(&my_timeinfo, "%Y");
  delay(disp_data_delay);
  // =========== 2nd view =================
  M5Dial.Display.clear();
  M5Dial.Display.setTextSize(2);
  M5Dial.Display.setCursor(hori[0]+30, vert[2]-16);
  M5Dial.Display.print(&my_timeinfo, "%H:%M:%S");
  M5Dial.Display.setTextSize(1);
}

unsigned long start_t = millis();
unsigned long msg_start_t = start_t;
unsigned long curr_t = 0L;
unsigned long msg_curr_t = 0L;
unsigned long elapsed_t = 0L;
unsigned long msg_elapsed_t = 0L;
unsigned long interval_t = 5 * 60 * 1000L;  // 5 minutes
unsigned long msg_interval_t = 60 * 1000L;  // 1 minute
unsigned long remaining_t = 0L;
bool lStart = true;
bool lMsgStart = true;

void loop() {
  ck_Btn();
  curr_t = millis();
  msg_curr_t = curr_t;
  elapsed_t = curr_t - start_t;
  msg_elapsed_t = msg_curr_t - msg_start_t;
  remaining_t = interval_t - elapsed_t;

  if (lStart || elapsed_t >= interval_t) 
  {
    if (lStart) lStart = false;
    start_t = curr_t;
    calc_diff();
  }

  if (lMsgStart || msg_elapsed_t >= msg_interval_t)
  {
    msg_start_t = msg_curr_t;
    if (lMsgStart) lMsgStart = false;
    if (lStart) lStart = false;

    disp_time();
    std::cout << "\nloop(): elapsed_t = " << std::to_string(elapsed_t) << " Second(s), interval_t = " << std::to_string(interval_t) << " Second(s)" << std::endl;
    std::cout << "Next NTP check in: " << std::to_string(remaining_t / (1000 * 60)) << " Minute(s)\n" << std::endl;
  }

  if (buttonPressed)
  {
    buttonPressed = false;
    std::cout << "\nButton pressed" << std::endl;
    std::cout << "Going to do a software reset..." << std::endl;
    M5Dial.Display.clearDisplay();
    M5Dial.Display.setCursor(hori[1] - 20, vert[2] - 5);
    M5Dial.Display.print("Going to reset...");
    delay(3000);
    esp_restart();
  }

  //disp_time(unixTime_tm);
  poll_RTC(); // print to serial Monitor
  disp_time(); // show on M5Dial display
  delay(1000);
}
