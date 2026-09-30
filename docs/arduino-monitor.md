# ESP32 Arduino Monitor

This optional ESP32 device combines temperature and current sensing in one monitor. The supplied firmware targets the LOLIN S3 and supports up to 18 configured ADC channels, using ADC pins GPIO 1 through 18.

Each channel can be configured as an NTC temperature sensor, an ACS712-style analog current sensor, or disabled. Current sensors use configurable zero-current offset voltage and volts-per-amp sensitivity. Do not connect any sensor output above 3.3 V to an ESP32 GPIO. Many ACS712 boards are powered from 5 V and require suitable level shifting or a voltage divider.

Build and upload the PlatformIO project in [arduino/monitor](../arduino/monitor). After flashing, join the `Arduino Monitor` Wi-Fi network with password `configureme`, then open `http://192.168.4.1` to configure the channels. Settings persist on the ESP32.

## OpenLinkHub configuration

The monitor is automatically discovered by its LOLIN S3 USB identity (`VID 0x303A`, `PID 0x1001`). To override discovery, set a serial port and optional baud rate in `config.json`, then restart OpenLinkHub:

```json
{
  "arduinoMonitorPort": "/dev/ttyACM0",
  "arduinoMonitorBaud": 115200
}
```

Leave `arduinoMonitorPort` empty to use automatic USB discovery. Set it to an explicit path to disable discovery and force a specific port. If no matching device is connected, the monitor is simply not initialized.

## Serial protocol

The monitor sends one newline-delimited JSON report every two seconds:

```json
{
  "temperatures": [{"name":"Coolant","value":31.25}],
  "currents": [{"name":"12V Rail","value":2.347}]
}
```

Temperatures are degrees Celsius and currents are amperes. OpenLinkHub continues to expose temperature probes through its existing probe handling and current sensors through `GET /api/devices/current/{deviceId}`.
