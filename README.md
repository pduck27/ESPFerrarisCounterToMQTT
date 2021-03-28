[![License: MPL 2.0](https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0)
![](https://github.com/pduck27/ESPFerrarisCounterToMQTT/workflows/BuildAndRelease/badge.svg)
![](https://img.shields.io/github/v/release/pduck27/ESPFerrarisCounterToMQTT)

# ESPFerrarisCounterToMQTT
This project monitors a power electricity meter using the TCRT5000 infrared sensor to determine electricity consumption. Results are sent via MQTT including current power, daily and total consumption e.g.. Optionally a light emitting diode is switched by the ESP on detection.

The TCRT5000 continuously checks the Ferraris disk for distance. If instead of the silver outline the red mark comes into the detection field, the distance becomes noticeably higher, because the reflection is lower. This change is registered by the program code to send a MQTT message.

# Setup
I use an ESP32DevKitv4, but any other ESP module should work just as well. A ready-made TRCT5000 module (https://www.az-delivery.de/products/linienfolger-modul-mit-tcrt5000-und-analog-ausgang) is used as sensor. This has an analog and digital output and two LEDs. Because of the narrow measuring ranges I could not fine tune the digital output and evaluate the analog output in the program code. But the digital output can be used quite easily (see Loop()). 
Wiring itself is very easy, just follow the several guides like this https://www.instructables.com/How-to-Use-TCRT5000-IR-Sensor-Module-With-Arduino-/.

To align the sensor, I recommend using a holder from the 3D printer (https://www.thingiverse.com/thing:2969084) and a mobile phone camera to make the infrared light visible on the disk. I had to experiment a bit with the cover cap on the module (pull it back-/forward). 

# Run it
For the first start you should adapt the file *Credentials_sample.h* from the include directory with your data and rename it to *Credentials.h*.

![credentials image](/ressource/shot2.png)

In the second step adjust the pin assignment and run the code. The values for your individual treshold and power meter parameters can be changed later after some runs.

![credentials image](/ressource/shot1.png)

At the serial monitor the values for digital output, analog output, distance, threshold check and current power are displayed. You can also have a look at them with the serial plotter. 
The threshold value for the detection (sensorTreshold) must be set in such a way that it is exceeded only when the marker is detected by the distance. 
In the release version one exceeding of the distance is sufficient. From first tests there is still a commented out version, which checks the distance as an average value of the last five readings. 

![credentials image](/ressource/shot3.png)

This can make sense if the values are very close together or the disks have too strong unevenness. But if the disc rotates very fast, it may lead to dropouts.

# Some more
Further settings that can be done in the code at the commented places are:
- Conversion of a disc round to your own consumption figures depending on the power meter.
- Separate delays for day / night after one turn
- Individual timestamps with and without UTC delay


# Licence
All code is licensed under the [MPLv2 License](https://github.com/pduck27/ESPFerrarisCounterToMQTT/blob/master/LICENSE).
Please recognize additional comments in the code.