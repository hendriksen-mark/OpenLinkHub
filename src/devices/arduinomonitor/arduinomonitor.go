package arduinomonitor

import (
	"OpenLinkHub/src/common"
	"OpenLinkHub/src/logger"
	"OpenLinkHub/src/serial"
	"bufio"
	"crypto/sha1"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"go.bug.st/serial/enumerator"
	"math"
	"strings"
	"sync"
)

const (
	defaultBaud    = 115200
	maximumCurrent = 1000.0
	maximumTemp    = 150.0
	lolinS3Vid     = "303a"
	lolinS3Pid     = "1001"
)

type report struct {
	Temperatures []reading `json:"temperatures"`
	Currents     []reading `json:"currents"`
}

type reading struct {
	Name  string  `json:"name"`
	Value float64 `json:"value"`
}

type TemperatureProbe struct {
	ChannelId int
	Name      string
	Label     string
	Serial    string
	Product   string
}

type CurrentSensor struct {
	ChannelId int     `json:"channelId"`
	Name      string  `json:"name"`
	Label     string  `json:"label"`
	Serial    string  `json:"serial"`
	Product   string  `json:"product"`
	Value     float64 `json:"value"`
}

type Device struct {
	dev               *serial.Device
	Product           string `json:"product"`
	Serial            string `json:"serial"`
	Path              string `json:"path"`
	TemperatureProbes *[]TemperatureProbe
	CurrentSensors    *[]CurrentSensor
	instance          *common.Device
	stop              chan struct{}
	once              sync.Once
	mutex             sync.RWMutex
}

// Init opens the configured combined Arduino monitor serial port.
func Init(port string, baud int) *common.Device {
	if port == "" {
		return nil
	}
	if baud == 0 {
		baud = defaultBaud
	}

	dev, err := serial.Open(&serial.Config{Name: port, Baud: baud})
	if err != nil {
		logger.Log(logger.Fields{"error": err, "path": port}).Error("Unable to open Arduino monitor")
		return nil
	}

	d := &Device{
		dev:     dev,
		Product: "Arduino Sensor Monitor",
		Serial:  serialForPath(port),
		Path:    port,
		stop:    make(chan struct{}),
	}
	d.createDevice()
	go d.readReports()
	logger.Log(logger.Fields{"serial": d.Serial, "path": port}).Info("Arduino sensor monitor initialized")
	return d.instance
}

func serialForPath(path string) string {
	sum := sha1.Sum([]byte(path))
	return "arduino-monitor-" + hex.EncodeToString(sum[:6])
}

func (d *Device) createDevice() {
	d.instance = &common.Device{
		ProductType: common.ProductTypeArduinoMonitor,
		Product:     d.Product,
		Serial:      d.Serial,
		Image:       "icon-temperature.svg",
		Instance:    d,
		GetDevice:   d,
	}
}

func (d *Device) Stop() {
	d.once.Do(func() {
		close(d.stop)
		if err := d.dev.Close(); err != nil {
			logger.Log(logger.Fields{"error": err, "serial": d.Serial}).Error("Unable to close Arduino monitor")
		}
	})
}

func (d *Device) StopDirty() uint8 {
	d.Stop()
	return 1
}

func (d *Device) GetDeviceTemplate() string {
	return "404-no-device.html"
}

func (d *Device) GetTemperatureProbes() *[]TemperatureProbe {
	d.mutex.RLock()
	defer d.mutex.RUnlock()
	return d.TemperatureProbes
}

func (d *Device) GetCurrentSensors() *[]CurrentSensor {
	d.mutex.RLock()
	defer d.mutex.RUnlock()
	return d.CurrentSensors
}

func (d *Device) readReports() {
	scanner := bufio.NewScanner(d.dev)
	scanner.Buffer(make([]byte, 256), 4096)
	for scanner.Scan() {
		select {
		case <-d.stop:
			return
		default:
		}

		var value report
		if err := json.Unmarshal(scanner.Bytes(), &value); err != nil {
			continue
		}
		d.applyReport(value)
	}
	select {
	case <-d.stop:
	default:
		logger.Log(logger.Fields{"error": scanner.Err(), "serial": d.Serial}).Warn("Arduino monitor stopped reading")
	}
}

func (d *Device) applyReport(value report) {
	d.mutex.Lock()
	defer d.mutex.Unlock()

	probes := make([]TemperatureProbe, 0, len(value.Temperatures))
	for index, item := range value.Temperatures {
		if math.IsNaN(item.Value) || math.IsInf(item.Value, 0) || item.Value < -100 || item.Value > maximumTemp {
			continue
		}
		name := strings.TrimSpace(item.Name)
		if name == "" {
			name = fmt.Sprintf("Temperature %d", index+1)
		}
		probes = append(probes, TemperatureProbe{ChannelId: index, Name: name, Label: name, Serial: d.Serial, Product: d.Product})
	}

	sensors := make([]CurrentSensor, 0, len(value.Currents))
	for index, item := range value.Currents {
		if math.IsNaN(item.Value) || math.IsInf(item.Value, 0) || item.Value < 0 || item.Value > maximumCurrent {
			continue
		}
		name := strings.TrimSpace(item.Name)
		if name == "" {
			name = fmt.Sprintf("Current %d", index+1)
		}
		sensors = append(sensors, CurrentSensor{ChannelId: index, Name: name, Label: name, Serial: d.Serial, Product: d.Product, Value: item.Value})
	}

	d.TemperatureProbes = &probes
	d.CurrentSensors = &sensors
}
