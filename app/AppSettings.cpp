/*
 * AppSettings.h
 *
 *  Created on: 13 ��� 2015 �.
 *      Author: Anakod
 */

#include <SmingCore/SmingCore.h>
#include <AppSettings.h>

void ApplicationSettingsStorage::load()
{
  char              str[64];
  DynamicJsonBuffer jsonBuffer;

  if (exist()) {
    int size = fileGetSize(APP_SETTINGS_FILE);
    char* jsonString = new char[size + 1];
    fileGetContent(APP_SETTINGS_FILE, jsonString, size + 1);
    JsonObject& root = jsonBuffer.parseObject(jsonString);

    JsonObject& network = root["network"];

    wired = false;

    ssid = (const char *)network["ssid"];
    password = (const char *)network["password"];
    apPassword = (const char *)network["apPassword"];
    portalUrl = (const char *)network["portalUrl"];
    portalData = (const char *)network["portalData"];

    if (!network.containsKey("apMode"))
      apMode = apModeAlwaysOn;
    else {
      String newMode = (const char *)network["apMode"];
      if (newMode.equals("always"))
        apMode = apModeAlwaysOn;
      else if (newMode.equals("never"))
        apMode = apModeAlwaysOff;
      else if (newMode.equals("whenDisconnected"))
        apMode = apModeWhenDisconnected;
      else
        apMode = apModeAlwaysOff;
      } // else

    dhcp    = network["dhcp"];
    ip      = (const char *)network["ip"];
    netmask = (const char *)network["netmask"];
    gateway = (const char *)network["gateway"];

    JsonObject& mqtt = root["mqtt"];
    mqttUser     = (const char *)mqtt["user"];
    mqttPass     = (const char *)mqtt["password"];
    mqttServer   = (const char *)mqtt["server"];
    mqttPort     = mqtt["port"];
    mqttClientId = (const char *)mqtt["clientId"];

    if (mqttClientId.equals(String(""))) {
      sprintf(str, "ESP_%08X", system_get_chip_id());
      mqttClientId = (const char *) str; 
      }

    JsonObject& gpiod = root["gpiod"];
    gpiodEmul = gpiod["emul"];
    gpiodMode = gpiod["mode"];
    gpiodEfmt = gpiod["efmt"];

    cpuBoost = root["cpuBoost"];
    useOwnBaseAddress = root["useOwnBaseAddress"];

    webOtaBaseUrl = (const char *)root["webOtaBaseUrl"];

    delete[] jsonString;
    }
  } //

void ApplicationSettingsStorage::save()
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  JsonObject& network = jsonBuffer.createObject();
  root["network"] = network;
  network["wired"] = wired;
  network["ssid"] = ssid.c_str();
  network["password"] = password.c_str();
  network["apPassword"] = apPassword.c_str();
  network["portalUrl"] = portalUrl.c_str();
  network["portalData"] = portalData.c_str();

  if (apMode == apModeAlwaysOn)
    network["apMode"] = "always";
  else if (apMode == apModeAlwaysOff)
    network["apMode"] = "never";
  else
    network["apMode"] = "whenDisconnected";

  network["dhcp"] = dhcp;

  // Make copy by value for temporary string objects
  network.set("ip", ip.toString());
  network.set("netmask", netmask.toString());
  network.set("gateway", gateway.toString());

  JsonObject& mqtt = jsonBuffer.createObject();
  root["mqtt"] = mqtt;
  mqtt.set("user", mqttUser);
  mqtt.set("password", mqttPass);
  mqtt.set("server", mqttServer);
  mqtt["port"] = mqttPort;
  mqtt.set("clientId", mqttClientId);

  JsonObject& gpiod = jsonBuffer.createObject();
  root["gpiod"] = gpiod;
  gpiod["emul"] = gpiodEmul;
  gpiod["mode"] = gpiodMode;
  gpiod["efmt"] = gpiodEfmt;

  root["cpuBoost"] = cpuBoost;
  root["useOwnBaseAddress"] = useOwnBaseAddress;

  root.set("webOtaBaseUrl", webOtaBaseUrl.c_str());

  //TODO: add direct file stream writing
  String out;
  root.printTo(out);
  fileSetContent(APP_SETTINGS_FILE, out);
  } //

ApplicationSettingsStorage AppSettings;
