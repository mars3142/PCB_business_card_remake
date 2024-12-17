# PCB business card - StuckAtPrototype

A custom PCB based "business" card that lets you play a version of Flappy Bird. 

## Youtube Video
A bit of a back-story of how this project came to be.

[![My Kickstarter failed, so I open sourced it](https://img.youtube.com/vi/y-lWarLUhfg/0.jpg)](https://youtu.be/y-lWarLUhfg)

Sub if you like what you see.

## Project Structure
The project consists of
1. Firmware
2. Hardware
3. Mechanical
4. Scripts

These are structured into their own files. I could have used submodules, but decided against it.

### 1. Firmware
Code for the business card. This lives on the ESP32

#### Requirements
- ESP32 IDF version 5.4
- Target set to ESP32-H2

### 2. Hardware

#### Schematic
PDF schematic included for your viewing pleasure.
#### PCBs
All the gerber files you'd need to send to a fab house.
#### Kicad
All the files you'd need to expand and work on this further. If you'd like.

### 3. Mechanical

#### Enclosure
All the step files you need to make one of these. Extrusion printer works well for this part.

### 4. Scripts

The script requires the `config.py` file to be modified. You will need to create your own API key at openweather.org

To run the script, simply run `python CardController.py`

#### Requirements
A bunch of pip packages

## What the project could use
1. Cleanup, but thats true for almost anything out there
2. A proper graphics library -- Adafruit, or even LVGL 
3. Scripts that would show stock data. anyone? 

## If you take it further
Let me know if you ever make one of these, I'd love to see it. Seriously, that'd be exciting and inspiring to keep making my projects open source!

## Known issues
1. Switch requires to be shorted due to power consumption at run time
2. GPIO 1 as is does not allow the device to wake up from sleep
3. The LEDs seem to be pulling a lot power even when ESP32 is in deep sleep mode

---
## License
### Apache 2.0 -- i.e. use as you'd like
http://www.apache.org/licenses/LICENSE-2.0

--- 
## Special Thanks
Thanks to Michael Angerer for his open sourced `esp32_ble_ota` project. I used it to get BLE running in this project. His blog post and github repo are a great resource. Check it out. https://github.com/michael-angerer/esp32_ble_ota 
