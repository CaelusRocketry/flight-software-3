# flight-software-3

Code for ESP32 on Sensor PCB 

## Setup

### Setting up ESP32
Go to **File > Preferences**
and under **Addtional Boards Manager URLs** copy this link:
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

Next, go to **Sketch > Include Library > Manage Libraries**
Once loaded, type in the search bar **esp32** and install espressif's esp32 library

You should now be able to upload to an ESP32. If you're having issues, make sure you the CP2102 USB driver installed on your device.

### Installing libraries

All Arduino libraries are listed in the /lib of the repo. Go through them and install them either through


