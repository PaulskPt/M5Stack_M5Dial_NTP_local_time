M5Stack M5Dial NTP local time

Important credit:

I was only able to create and successfully finish this project with the great help of Microsoft AI assistant CoPilot.
CoPilot helped me correcting wrong C++ code fragments. It suggested C++ code fragments. CoPilot gave me, in one Q&A session, a "workaround" 
for certain limitation that the Arduino IDE has with the standard C++ specification. And CoPilot gave it's answers at great speed of response.
There were no delays. The answers were instantaneous! Thank you CoPilot. Thank you Microsoft for this exciting new tool!

Hardware used:

1. M5Stack M5Dial


Reset:

Pressing the button (of the display) will cause a software reset.

On reset the Arduino Sketch will try to connect to the WiFi Access Point of your choice (set in secret.h). If successful the sketch will next connect to a NTP server of your choice, then download the current datetime stamp. If successful, the built-in RTC will be synchronized with the NTP datetime stamp.
Next every 5 minutes the built-in RTC will be updated from a fetched NTP datetime, if the time of the built-in RTC and the time from the NTP server differ less than 2 seconds (tolerance).

File secret.h:

Update the file secret.h as far as needed:
```
 a) your WiFi SSID in SECRET_SSID;
 b) your WiFi PASSWORD in SECRET_PASS;
 c) the name of the NTP server of your choice in SECRET_NTP_SERVER_1, for example: 2.pt.pool.ntp.org.
```
At startup or at reset the sketch will load the WiFi Credentials and the address of the first NTP server.

Docs:

See the Monitor_output.txt

Images:

Images taken during the sketch was running are in the folder ```images```.

Links to product pages of the hardware used:

- M5Stack M5Dial [info](https://docs.m5stack.com/en/core/M5Dial);


Known problem:
None so far.




