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
    case '&':
      escaped += F("&amp;");
      break;
    case '<':
      escaped += F("&lt;");
      break;
    case '>':
      escaped += F("&gt;");
      break;
    case '"':
      escaped += F("&quot;");
      break;
    case '\'':
      escaped += F("&#39;");
      break;
    default:
      escaped += *value;
    }
    ++value;
  }
  return escaped;
}

String configurationPage()
{
  String page = F("<!doctype html><html><head><meta name=viewport content='width=device-width,initial-scale=1'><title>Temperature Monitor</title><style>body{font-family:system-ui,sans-serif;max-width:760px;margin:2rem auto;padding:0 1rem;color:#17202a}table{width:100%;border-collapse:collapse}th,td{padding:.5rem;border-bottom:1px solid #d5d8dc;text-align:left}input{box-sizing:border-box;padding:.45rem;width:100%;font:inherit}input[type=checkbox]{width:auto}button{margin-top:1rem;background:#147a75;color:white;border:0;padding:.65rem 1rem;font:inherit;border-radius:4px}</style></head><body><h1>Temperature Monitor</h1><p>Configure 10k B3950 NTC sensors. On the LOLIN S3, use ADC1 GPIO pins 1 through 10.</p><form method=post action=/save><table><tr><th>Enabled</th><th>Name</th><th>GPIO</th></tr>");
  for (size_t index = 0; index < MAX_SENSORS; ++index)
  {
    page += "<tr><td><input type=checkbox name=e" + String(index);
    if (sensors[index].enabled)
      page += " checked";
    page += "></td><td><input maxlength=24 name=n" + String(index) + " value='" + escapeHtml(sensors[index].name) + "'></td><td><input type=number min=1 max=10 name=p" + String(index) + " value='" + String(sensors[index].pin) + "'></td></tr>";
  }
  page += F("</table><button type=submit>Save sensors</button></form></body></html>");
  return page;
}

void handleSave()
{
  for (size_t index = 0; index < MAX_SENSORS; ++index)
  {
    const String nameKey = String("n") + index;
    const String pinKey = String("p") + index;
    const String enabledKey = String("e") + index;
    const String name = server.arg(nameKey);
    const int pin = server.arg(pinKey).toInt();
    name.substring(0, sizeof(sensors[index].name) - 1)
        .toCharArray(sensors[index].name, sizeof(sensors[index].name));
    sensors[index].pin = static_cast<uint8_t>(pin);
    sensors[index].enabled = server.hasArg(enabledKey) && name.length() > 0 &&
                             isAdcPin(sensors[index].pin);
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
  server.on("/", HTTP_GET, []()
            { server.send(200, "text/html", configurationPage()); });
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

void handleWebUi()
{
  server.handleClient();
}
