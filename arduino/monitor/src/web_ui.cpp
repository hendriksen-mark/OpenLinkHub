#include "web_ui.h"

#include <WebServer.h>
#include <WiFi.h>
#include "config.h"
#include "sensors.h"

WebServer server(80);

String escapeHtml(const char *value)
{
  String escaped;
  while (*value != '\0')
  {
    switch (*value)
    {
    case '&': escaped += F("&amp;"); break;
    case '<': escaped += F("&lt;"); break;
    case '>': escaped += F("&gt;"); break;
    case '"': escaped += F("&quot;"); break;
    case '\'': escaped += F("&#39;"); break;
    default: escaped += *value;
    }
    ++value;
  }
  return escaped;
}

String configurationPage()
{
  String page = F("<!doctype html><html><head><meta name=viewport content='width=device-width,initial-scale=1'><title>Arduino Monitor</title><style>body{font-family:system-ui,sans-serif;max-width:1000px;margin:2rem auto;padding:0 1rem;color:#17202a}table{width:100%;border-collapse:collapse}th,td{padding:.5rem;border-bottom:1px solid #d5d8dc;text-align:left}input,select{box-sizing:border-box;padding:.45rem;width:100%;font:inherit}input[type=checkbox]{width:auto}button{margin-top:1rem;background:#147a75;color:white;border:0;padding:.65rem 1rem;font:inherit;border-radius:4px}</style></head><body><h1>Arduino Monitor</h1><p>Configure NTC temperature sensors and ACS712-style current sensors. Current offset is volts at zero current; sensitivity is volts per amp.</p><form method=post action=/save><table><tr><th>Enabled</th><th>Type</th><th>Name</th><th>GPIO</th><th>Offset (V)</th><th>Sensitivity (V/A)</th></tr>");
  for (size_t index = 0; index < MAX_SENSORS; ++index)
  {
    page += "<tr><td><input type=checkbox name=e" + String(index);
    if (sensors[index].enabled) page += " checked";
    page += "></td><td><select name=t" + String(index) + "><option value=1" + String(sensors[index].type == SENSOR_TEMPERATURE ? " selected" : "") + ">Temperature</option><option value=2" + String(sensors[index].type == SENSOR_CURRENT ? " selected" : "") + ">Current</option></select></td><td><input maxlength=24 name=n" + String(index) + " value='" + escapeHtml(sensors[index].name) + "'></td><td><input type=number min=1 max=" + String(MAX_SENSORS) + " name=p" + String(index) + " value='" + String(sensors[index].pin) + "'></td><td><input type=number step=0.001 min=0 max=3.3 name=o" + String(index) + " value='" + String(sensors[index].offsetVoltage, 4) + "'></td><td><input type=number step=0.001 min=0.001 name=s" + String(index) + " value='" + String(sensors[index].sensitivity, 4) + "'></td></tr>";
  }
  page += F("</table><button type=submit>Save sensors</button></form></body></html>");
  return page;
}

void handleSave()
{
  for (size_t index = 0; index < MAX_SENSORS; ++index)
  {
    const String name = server.arg(String("n") + index);
    const SensorType type = static_cast<SensorType>(server.arg(String("t") + index).toInt());
    sensors[index].type = type;
    sensors[index].pin = static_cast<uint8_t>(server.arg(String("p") + index).toInt());
    sensors[index].offsetVoltage = server.arg(String("o") + index).toFloat();
    sensors[index].sensitivity = server.arg(String("s") + index).toFloat();
    name.substring(0, sizeof(sensors[index].name) - 1).toCharArray(sensors[index].name, sizeof(sensors[index].name));
    sensors[index].enabled = server.hasArg(String("e") + index) && name.length() > 0 && isAdcPin(sensors[index].pin) &&
                             (type == SENSOR_TEMPERATURE || (type == SENSOR_CURRENT && sensors[index].sensitivity > 0));
  }
  saveSensors();
  server.sendHeader("Location", "/");
  server.send(303);
}

void beginWebUi()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("Sensor setup page: http://");
  Serial.println(WiFi.softAPIP());
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", configurationPage()); });
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

void handleWebUi()
{
  server.handleClient();
}
