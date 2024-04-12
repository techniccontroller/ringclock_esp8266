# RingClock

RingClick with Neopixel LEDs and ESP8266

More details on my website: https://techniccontroller.com/ringclock-with-neopixel-led-and-esp8266/


## Features
- receives time updates via a NTP server
- automatic switches between summer and winter time
- provides easy WIFI setup with WifiManager
- has a HTML webserver interface for configuration and control
- color of display is configurable
- night mode is configurable (start and end time)
- brightness of LEDs is adjustable
- an automatic current limiting of LEDs protects the power supply

## Pictures of clock

| ![](https://techniccontroller.com/wp-content/uploads/ringclock_6-1024x1024.webp) | ![](https://techniccontroller.com/wp-content/uploads/ringclock_5-1024x1024.webp) | ![](https://techniccontroller.com/wp-content/uploads/ringclock_1-1024x1024.webp) |
|-|-|-|

## Screenshots of webserver UI
![screenshots_UI](https://techniccontroller.com/wp-content/uploads/weboberflache_RINGCLOCK_en.png)

## Quickstart

1. Clone the project into the sketch folder of the Arduino IDE, 
2. Install the additional libraries and upload the program to the ESP8266 as usual (See section [*Upload program to ESP8266*](https://github.com/techniccontroller/ringclock_esp8266/blob/main/README.md#upload-program-to-esp8266-with-arduino-ide) below). 
3. The implemented WiFiManager helps you to set up a WiFi connection with your home WiFi -> on the first startup it will create a WiFi access point named "RingclockAP". Connect your phone to this access point and follow the steps which will be shown to you. 
4. After a successful WiFi setup, open the browser and enter the IP address of your ESP8266 to access the interface of the webserver. 
5. Here you can upload all files located in the folder "data". Please make sure all icons stay in the folder "icons" also on the webserver.
    - Open **http://\<ip-address\>/fs.html** in a browser
    - Upload **fs.html**
    - Upload **style.css**
    - Upload **index.html**
    - Create a new folder **icons**
    - Upload all icons into this new folder **icons**


<img src="https://techniccontroller.com/wp-content/uploads/filemanager1-1-1.png" height="300px" /> <img src="https://techniccontroller.com/wp-content/uploads/filemanager2-1-1.png" height="300px" /> <img src="https://techniccontroller.com/wp-content/uploads/filemanager3-1-1.png" height="300px" />

## Install needed Libraries

Please download all these libraries as ZIP from GitHub, and extract them in the *libraries* folder of your Sketchbook location (see **File -> Preferences**):

- https://github.com/adafruit/Adafruit_NeoPixel
- https://github.com/tzapu/WiFiManager
- https://github.com/adafruit/Adafruit_BusIO

folder structure should look like this:

```
MySketchbookLocation 
│
└───libraries
│   └───Adafruit_NeoPixel
│   └───WiFiManager
│   └───Adafruit_BusIO
│   
└───ringclock_esp8266
    │   ringclock_esp8266.ino
    │   (...)
    |
    └───data
        │   index.html
        |   (...)
        |
        └───icons 
```


## Upload program to ESP8266 with Arduino IDE

#### STEP1: Installation of Arduino IDE
First, the latest version of the Arduino IDE needs to be downloaded and installed from [here](https://www.arduino.cc/en/software).

#### STEP2: Installation of ESP8266 Arduino Core
To program the ESP8266 with the Arduino IDE, you need to install the board information first in Arduino IDE. To do that follow the following instructions:

- Start Arduino and open the File -> Preferences window.

- Enter http://arduino.esp8266.com/stable/package_esp8266com_index.json into the Additional Board Manager URLs field. You can add multiple URLs, separating them with commas.
![image](https://user-images.githubusercontent.com/36072504/169649790-1b85660e-8c7d-4dfe-a63a-5dfd9862a5de.png)

- Open Boards Manager from Tools > Board menu and search for "esp8266".

- Click the install button.

- Don’t forget to select your ESP8266 board from Tools > Board menu after installation (e.g NodeMCU 1.0, or Wemos D1 mini)
![image](https://user-images.githubusercontent.com/36072504/169649801-898c4819-9145-45c5-b65b-52f2689ab646.png)

#### STEP3: Upload a program to ESP8266

- Open ringclock_esp8266.ino in Arduino IDE
- Connect ESP8266 board with Computer
- Select right serial Port in Tools -> Port
- Click on the upload button in the Arduino IDE to upload the program to the ESP8266 Module.     
![image](https://user-images.githubusercontent.com/36072504/169649810-1fda75c2-5f4d-4d71-98fe-30985d82f7f5.png)


## Resetting the WiFi configuration

You can clear the stored WiFi credentials and restart the WiFi setup described above with these steps:
1. Open the settings panel in the web UI.
2. Enable 'Reset WiFi' slider.
3. Save settings.
4. Disconnect and reconnect the power. WiFi credentials were removed. The setup should be restarted.
Resetting the wifi credentials does not delete uploaded files.

## Remark about Logging

The RingClock sends continuous log messages to the serial port and via multicast UDP. If you want to see these messages, you have to 

- open the serial monitor in the Arduino IDE (Tools -> Serial Monitor). The serial monitor must be set to 115200 baud.

OR

- run the following steps for the multicast UDP logging:

1. Starting situation: RingClock is connected to WLAN, a computer with installed Python (https://www.python.org/downloads/) is in the same local area network (WLAN or LAN doesn't matter).
3. Open the file **multicastUDP_receiver.py** in a text editor and in line 81 enter the IP address of the computer (not the RingClock!).
```python	
# ip address of network interface
MCAST_IF_IP = '192.168.0.7'
```
4. Execute the script with following command: 

```bash
python multicastUDP_receiver_analyzer.py
```

5. Now you should see the log messages of the RingClock (every 5 seconds a heartbeat message and the currently displayed time). 
If this is not the case, there could be a problem with the network settings of the computer, then recording is unfortunately not possible.

6. If special events (failed NTP update, reboot) occur, a section of the log is saved in a file called *log.txt*. 
In principle, the events are not critical and will occur from time to time, but should not be too frequent.
