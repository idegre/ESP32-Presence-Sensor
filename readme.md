# Wiz Radar

This is code for the ESP32-CAM that implemements the use of the ld... radar in order to be used to turn

# Instalation

- Create a secrets.h file
- Add the following to your arduino IDE:
  - https://github.com/fhessel/esp32_https_server to your arduino libraries
  - https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
  - https://github.com/ncmreynolds/ld2410
  - https://github.com/bartoszbielawski/AJSP
- Perform this fix https://forum.arduino.cc/t/https-webserver-with-external-library-esp32-https-server-on-esp32/912930

# Secrets

```c
#define WIFI_SSID "your ssid"
#define WIFIPASS "your password"
#define WIFINAME "Presence Sensor"
#define LIGHTS_ARRAY {"lamp mac address", "lamp 2 mac address", "lamp 3 mac address"}
```

# Programming

This programs normally into board.

here are the pintouts:

```c
#define RADAR_RX_PIN 12
#define RADAR_TX_PIN 13
#define RADAR_OUT_PIN 2
```

# Rest API

The code includes a very basic server with a few endpoints:

- /LED: turns on the front LED on the board
- /img: returns one photo from the camera
- /radar: returns radar info as a json (usefull for debugging)

# More info

You can read more and find the files for the case [here](https://blog.ignaciodegregori.com/posts/building_a_esp32_precense_sensor)
