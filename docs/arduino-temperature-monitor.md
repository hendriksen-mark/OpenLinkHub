# ESP32 Temperature Monitor

This optional ESP32 device adds NTC temperatures to the normal OpenLinkHub temperature-probe list. It does not control fans, lighting, or any other hardware.

## Hardware

The supplied firmware targets the LOLIN S3 and supports up to eight NTC thermistors. Each sensor needs its own voltage divider:

```text
3.3V --- 10k fixed resistor --- ESP32 ADC GPIO --- NTC --- GND
```

The defaults are for 10k NTC thermistors with a B value of 3950 and 10k fixed resistors. Build and upload the PlatformIO project in [arduino/temperature_monitor](../arduino/temperature_monitor). The firmware uses GPIO 1 through 10 as ADC inputs. Avoid applying more than 3.3V to any ESP32 GPIO.

After flashing, join the `Temperature Monitor` Wi-Fi network with password `configureme`, then open `http://192.168.4.1`. Enable, name, and assign each sensor's GPIO in the web UI. Settings persist on the ESP32. OpenLinkHub does not electrically discover NTC sensors; it creates one probe for every enabled sensor included in the ESP32's serial report.

## Configuration

Set the serial port and optional baud rate in OpenLinkHub's `config.json`, then restart the service:

```json
{
  "arduinoTemperaturePort": "/dev/ttyACM0",
  "arduinoTemperatureBaud": 115200
}
```

Leave `arduinoTemperaturePort` empty to disable the monitor. The OpenLinkHub service account needs read/write access to the port; on most Linux distributions, add it to the `dialout` group.

The device accepts one newline-delimited JSON report at a time in this form, so custom Arduino sensor code can use the same integration:

```json
{"temperatures":[{"name":"Loop","value":31.25},{"name":"Ambient","value":22.5}]}
```
