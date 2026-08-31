package arduinotemp

import (
	"OpenLinkHub/src/common"
	"OpenLinkHub/src/logger"
	"OpenLinkHub/src/serial"
	"bufio"
	"crypto/sha1"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"math"
	"strings"
	"sync"
)

const (
	defaultBaud        = 115200
	maximumTemperature = 150.0
)

type temperatureReport struct {
	Temperatures []temperatureValue `json:"temperatures"`
}

type temperatureValue struct {
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

type Devices struct {
	ChannelId          int     `json:"channelId"`
	DeviceId           string  `json:"deviceId"`
	Name               string  `json:"name"`
	Temperature        float64 `json:"temperature"`
	TemperatureString  string  `json:"temperatureString"`
	Description        string  `json:"description"`
	Label              string  `json:"label"`
	HasTemps           bool
	IsTemperatureProbe bool
}

type Device struct {
	dev               *serial.Device
	Product           string                    `json:"product"`
	Serial            string                    `json:"serial"`
	Path              string                    `json:"path"`
	Devices           map[int]*Devices          `json:"devices"`
	TemperatureProbes *[]TemperatureProbe
	instance          *common.Device
	stop              chan struct{}
	once              sync.Once
	mutex             sync.RWMutex
}

// Init opens the configured Arduino serial port. An empty port disables the monitor.
func Init(port string, baud int) *common.Device {
	if port == "" {
		return nil
	}
	if baud == 0 {
		baud = defaultBaud
	}

	dev, err := serial.Open(&serial.Config{Name: port, Baud: baud})
	if err != nil {
		logger.Log(logger.Fields{"error": err, "path": port}).Error("Unable to open Arduino temperature monitor")
		return nil
	}

	d := &Device{
		dev:     dev,
		Product: "Arduino Temperature Monitor",
		Serial:  serialForPath(port),
		Path:    port,
		Devices: make(map[int]*Devices),
		stop:    make(chan struct{}),
	}
	d.createDevice()
	go d.readReports()
	logger.Log(logger.Fields{"serial": d.Serial, "path": port}).Info("Arduino temperature monitor initialized")
	return d.instance
}

func serialForPath(path string) string {
	sum := sha1.Sum([]byte(path))
	return "arduino-" + hex.EncodeToString(sum[:6])
}

func (d *Device) createDevice() {
	d.instance = &common.Device{
		ProductType: common.ProductTypeArduinoTemperature,
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
			logger.Log(logger.Fields{"error": err, "serial": d.Serial}).Error("Unable to close Arduino temperature monitor")
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

func (d *Device) readReports() {
	scanner := bufio.NewScanner(d.dev)
	scanner.Buffer(make([]byte, 256), 4096)
	for scanner.Scan() {
		select {
		case <-d.stop:
			return
		default:
		}

		var report temperatureReport
		if err := json.Unmarshal(scanner.Bytes(), &report); err != nil || report.Temperatures == nil {
			continue
		}
		d.applyReport(report)
	}
	select {
	case <-d.stop:
	default:
		logger.Log(logger.Fields{"error": scanner.Err(), "serial": d.Serial}).Warn("Arduino temperature monitor stopped reading")
	}
}

func (d *Device) applyReport(report temperatureReport) {
	d.mutex.Lock()
	defer d.mutex.Unlock()

	d.Devices = make(map[int]*Devices, len(report.Temperatures))
	probes := make([]TemperatureProbe, 0, len(report.Temperatures))
	for index, value := range report.Temperatures {
		if math.IsNaN(value.Value) || math.IsInf(value.Value, 0) || value.Value < -100 || value.Value > maximumTemperature {
			continue
		}
		name := strings.TrimSpace(value.Name)
		if name == "" {
			name = fmt.Sprintf("Temperature %d", index+1)
		}
		d.Devices[index] = &Devices{
			ChannelId:          index,
			DeviceId:           fmt.Sprintf("probe-%d", index),
			Name:               name,
			Temperature:        value.Value,
			TemperatureString:  fmt.Sprintf("%.1f C", value.Value),
			Description:        "Arduino temperature probe",
			Label:              name,
			HasTemps:           true,
			IsTemperatureProbe: true,
		}
		probes = append(probes, TemperatureProbe{ChannelId: index, Name: name, Label: name, Serial: d.Serial, Product: d.Product})
	}
	d.TemperatureProbes = &probes
}