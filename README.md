# Home-Automation-Project
## Arduino
### Arduino nodes uses below libraries for functioning

WiFiNINA library for Wi-Fi connectivity

ArduinoBLE for BLE communication

ArduinoMqttClient for comminaction with Raspberry pi

Arduino_JSON library for JSON data creation

## Raspberry Pi
Wide range of libraries and packages are installed in Raspberry Pi for MQTT communication and Face Recognition
### MQTT
Mosquitto MQTT Broker
```
sudo apt install mosquitto
sudo apt install mosquitto-clients
```
Paho C++ MQTT clients
```
sudo apt install libssl-dev git
    
git clone http://github.com/eclipse/paho.mqtt.c
    
cd paho.mqtt.c/
    
make
    
sudo make install
```
WiringPi Library
```
sudo apt-get install wiringpi
```
JSON C Library
```
sudo apt install libjson-c-dev
```
The Raspberry Pi C++ programs are compiled using below commands
```
g++ -o status publish_arduino.cpp -lpaho-mqtt3c -lwiringPi

g++ -o temp temp_subscribe.cpp -lpaho-mqtt3c -lwiringPi -ljson-c

g++ -o set_temp publish_temp.cpp -lpaho-mqtt3c -lwiringPi
```
### DNS program for Remote access
For remote access the current public IP address of raspberry pi is updated continuosly to DNS, which is done through below commands
```
mkdir duckdns
cd duckdns
vi duck.sh
```
Below line of code is added to duck.sh file and then saved and closed
```
echo url="https://www.duckdns.org/update?domains=foghome&token=d154c6f9-2dcd-479b-8c4e-f4fde570b247&ip=" | curl -k -o ~/duckdns/duck.log -K -
```
The above file is made executable and added to cron service to run at every 5 minutes
```
chmod 700 duck.sh
crontab -e
*/5 * * * * ~/duckdns/duck.sh >/dev/null 2>&1
./duck.sh
sudo service cron start
```

### Face Recognition
Different set of libraries are installed for compiling OpenCV all of these are provided in below single command, the face recognition system is tested on Raspberry Pi with Debian Buster OS
```
sudo apt install cmake build-essential pkg-config git libjpeg-dev libtiff-dev libjasper-dev libpng-dev libwebp-dev libopenexr-dev libavcodec-dev libavformat-dev libswscale-dev libv4l-dev libxvidcore-dev libx264-dev libdc1394-22-dev libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev libgtk-3-dev libqtgui5 libqtwebkit5 libqt5-test python3-pyqt5 libatlas-base-dev liblapacke-dev gfortran libhdf5-dev libhdf5-103 python3-dev python3-pip python3-numpy
```

After library installation OpenCV is installed from source using below set of commands
```
git clone https://github.com/opencv/opencv.git
mkdir ~/opencv/build
cd ~/opencv/build
cmake
make -j$(nproc)
sudo make install
sudo ldconfig
```
Other pakages for facial recognation are installed as below
```
pip install face-recognition
pip install imputils
```
For Google Drive upload below packages are used
```
pip install --upgrade google-api-python-client google-auth-httplib2 google-auth-oauthlib
```
The facial recognition part was inspired from the work at https://github.com/carolinedunn/facial_recognition and some notable changes are made to adapt it to my model.


