
#include <user_config.h>
#include <SmingCore.h>
#include <SmingCore/Network/TelnetServer.h>
#include <AppSettings.h>
#include <globals.h>
#include <Network.h>
#include <HTTP.h>
#include <gpiod.h>
#include <cparse.hpp>

//FTPServer      g_ftp;
TelnetServer   g_telnet;
static boolean g_firstTime = TRUE;
int            g_isNetworkConnected = FALSE;
Timer          g_appTimer;

//----------------------------------------------------------------------------
// periodic reporting memory usage
//----------------------------------------------------------------------------
void appReportHeapUsage()
{
  tGpiodCmd cmd = { 0 };

  cmd.dwOrig = CGPIOD_ORIG_MQTT;
  cmd.dwObj  = CGPIOD_OBJ_SYSTEM;
  cmd.dwCmd  = CGPIOD_SYS_CMD_MEMORY;
  g_gpiod.DoCmd(&cmd);
  } //

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
// Will be called when system initialization was completed
void startServers()
{
  //HTTP must be first so handlers can be registered
  g_http.begin(); 

  g_appTimer.initializeMs(60000, appReportHeapUsage).start(true);

  // Start FTP server
  if (!fileExist("index.html"))
    fileSetContent("index.html", "<h3>Please connect to FTP and upload files from folder 'web/build' (details in code)</h3>");

//g_ftp.listen(21);
//g_ftp.addUser("admin", "12345678"); // FTP account

  g_telnet.listen(23);

  g_gpiod.begin(); // gpiod
  } //

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
// Will be called when WiFi station was connected to AP
void wifiCb(bool connected)
{
  g_isNetworkConnected = connected;
  if (connected) {
    Debug.println("--> Wifi CONNECTED");
    if (g_firstTime) 
      g_firstTime = FALSE;
    }
  else
    Debug.println("--> Wifi DISCONNECTED");
  } //

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void processInfoCommand(String commandLine, CommandOutput* pOut)
{
//  uint64_t rfBaseAddress = GW.getBaseAddress();

  pOut->printf("\r\n");
    
  pOut->printf("System information  : ESP8266 based Wifi IO node\r\n");
  pOut->printf("Build time          : %s\r\n", build_time);
  pOut->printf("Version             : %s\r\n", build_git_sha);
  pOut->printf("Sming Version       : %s\r\n", SMING_VERSION);
  pOut->printf("ESP SDK version     : %s\r\n", system_get_sdk_version());
  pOut->printf("\r\n");

  pOut->printf("Station SSID        : %s\r\n", AppSettings.ssid.c_str());
  pOut->printf("Station DHCP        : %s\r\n", AppSettings.dhcp ? "TRUE" : "FALSE");
  pOut->printf("Station IP          : %s\r\n", Network.getClientIP().toString().c_str());

  String apModeStr;
  if (AppSettings.apMode == apModeAlwaysOn)
    apModeStr = "always";
  else if (AppSettings.apMode == apModeAlwaysOff)
    apModeStr = "never";
  else
    apModeStr= "whenDisconnected";

  pOut->printf("Access Point Mode   : %s\r\n", apModeStr.c_str());
  pOut->printf("\r\n");

  pOut->printf("wiControl\r\n");
  pOut->printf(" Version            : %s\r\n", szAPP_VERSION);
  pOut->printf(" Topology           : %s\r\n", szAPP_TOPOLOGY);
  pOut->printf(" Emulation          : %s\r\n", (g_gpiod.GetEmul() == CGPIOD_EMUL_OUTPUT)     ? "output"     : "shutter");
  pOut->printf(" Mode               : %s\r\n", (g_gpiod.GetMode() == CGPIOD_MODE_STANDALONE) ? "standalone" :
                                               (g_gpiod.GetMode() == CGPIOD_MODE_MQTT)       ? "MQTT"       : "both");
  pOut->printf(" Event/status format: %s\r\n", (g_gpiod.GetEfmt() == CGPIOD_EFMT_NUMERICAL)  ? "numerical"  : "textual");
  pOut->printf(" Lock state         : %s\r\n", (g_gpiod.GetFlags(CGPIOD_FLG_LOCK))           ? "locked"     : "unlocked");
  pOut->printf(" Disable state      : %s\r\n", (g_gpiod.GetFlags(CGPIOD_FLG_DISABLE))        ? "disabled"   : "enabled");
  pOut->printf(" Loglevel           : 0x%08X\r\n", Debug.logClsLevels(DEBUG_CLS_0));
  pOut->printf("\r\n");

  pOut->printf("System Time         : ");
  pOut->printf(SystemClock.getSystemTimeString().c_str());
  pOut->printf("\r\n");

  pOut->printf("Free Heap           : %d\r\n", system_get_free_heap_size());
  pOut->printf("CPU Frequency       : %d MHz\r\n", system_get_cpu_freq());
  pOut->printf("System Chip ID      : %x\r\n", system_get_chip_id());
  pOut->printf("SPI Flash ID        : %x\r\n", spi_flash_get_id());
  pOut->printf("SPI Flash Size      : %d\r\n", (1 << ((spi_flash_get_id() >> 16) & 0xff)));
  pOut->printf("\r\n");
  } //

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void processRestartCommand(String commandLine, CommandOutput* pOut)
{
  System.restart();
  }

void processRestartCommandWeb(void)
{
  System.restart();
  }

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void processDebugCommand(String commandLine, CommandOutput* pOut)
{
  Vector<String> commandToken;
  int            numToken = splitString(commandLine, ' ' , commandToken);

  if ((numToken != 2) || (commandToken[1] != "on" && commandToken[1] != "off")) {
    pOut->printf("Usage : \r\n\r\n");
    pOut->printf("  debug on  : Enable debug\r\n");
    pOut->printf("  debug off : Disable debug\r\n");
    return;
    }

  if (commandToken[1] == "on")
    Debug.start();
  else
    Debug.stop();
  } // processDebugCommand

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void processCpuCommand(String commandLine, CommandOutput* pOut)
{
  Vector<String> commandToken;
  int numToken = splitString(commandLine, ' ' , commandToken);

  if (numToken != 2 ||
      (commandToken[1] != "80" && commandToken[1] != "160"))
  {
    pOut->printf("Usage : \r\n\r\n");
    pOut->printf("  cpu 80  : Run at 80MHz\r\n");
    pOut->printf("  cpu 160 : Run at 160MHz\r\n");
    return;
    }

  if (commandToken[1] == "80")
  {
    System.setCpuFrequency(eCF_80MHz);
    AppSettings.cpuBoost = false;
    }
  else
  {
    System.setCpuFrequency(eCF_160MHz);
    AppSettings.cpuBoost = true;
    }

  AppSettings.save();
  } //

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void processBaseAddressCommand(String commandLine, CommandOutput* pOut)
{
  Vector<String> commandToken;
  int numToken = splitString(commandLine, ' ' , commandToken);

  if (numToken != 2 ||
      (commandToken[1] != "default" && commandToken[1] != "private"))
  {
    pOut->printf("Usage : \r\n\r\n");
    pOut->printf("  base-address default : Use the default MySensors base address\r\n");
    pOut->printf("  base-address private : Use a base address based on ESP chip ID\r\n");
    return;
    }

  if (commandToken[1] == "default")
  {
    AppSettings.useOwnBaseAddress = false;
    }
  else
  {
    AppSettings.useOwnBaseAddress = true;
    }

  AppSettings.save();
  System.restart();
  } //

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void processShowConfigCommand(String commandLine, CommandOutput* pOut)
{
  pOut->println(fileGetContent(".settings.conf"));
  } //

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void processAPModeCommand(String commandLine, CommandOutput* pOut)
{
  Vector<String> commandToken;
  int numToken = splitString(commandLine, ' ' , commandToken);

    if (numToken != 2 ||
        (commandToken[1] != "always" && commandToken[1] != "never" &&
         commandToken[1] != "whenDisconnected"))
    {
      pOut->printf("Usage : \r\n\r\n");
      pOut->printf("  apMode always           : Always have the AP enabled\r\n");
      pOut->printf("  apMode never            : Never have the AP enabled\r\n");
      pOut->printf("  apMode whenDisconnected : Only enable the AP when disconnected\r\n");
      pOut->printf("                            from the network\r\n");
      return;
    }

    if (commandToken[1] == "always")
    {
      AppSettings.apMode = apModeAlwaysOn;
    }
    else if (commandToken[1] == "never")
    {
      AppSettings.apMode = apModeAlwaysOff;
    }        
    else
    {
      AppSettings.apMode = apModeWhenDisconnected;
    }

  AppSettings.save();
  System.restart();
  } //

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void processLogLevelCommand(String commandLine, CommandOutput* pOut)
{
  Vector<String> commandToken;
  int            numToken = splitString(commandLine, ' ' , commandToken);
  tUint32        dwLevels;

  if (numToken > 1) {
    // commandToken[1] is loglevel
    dwLevels = xstrToUint32(commandToken[1].c_str());
    Debug.logClsLevels(DEBUG_CLS_0, dwLevels);
        
    // toggle msgId usage
    Debug.logPrintMsgId(Debug.logPrintMsgId() ? false : true);
    } // if

  pOut->printf("Loglevel = 0x%08X\r\n\r\n", Debug.logClsLevels(DEBUG_CLS_0));
//AppSettings.save();
//System.restart();
  } // processLogLevelCommand

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void init()
{
  char str[32];

  // Make sure wifi does not start yet! 
  wifi_station_set_auto_connect(0);

  // Make sure all chip enable pins are HIGH 
  g_gpiod.OnInit();

  // Mount the internal storage 
  int slot = rboot_get_current_rom();
  if (slot == 0) {
    Debug.printf("trying to mount spiffs at %x, length %d\n", 0x40300000, SPIFF_SIZE);
    spiffs_mount_manual(0x40300000, SPIFF_SIZE);
    }
  else {
    Debug.printf("trying to mount spiffs at %x, length %d\n", 0x40500000, SPIFF_SIZE);
    spiffs_mount_manual(0x40500000, SPIFF_SIZE);
    }

  Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
  Serial.systemDebugOutput(true); // Enable debug output to serial
  Serial.commandProcessing(true);
  Debug.start();

  // set prompt
  sprintf(str, "%s/%s> ", szAPP_ALIAS, szAPP_TOPOLOGY); 
  commandHandler.setCommandPrompt(str);

  // add new commands
  commandHandler.registerCommand(CommandDelegate("loglevel",
                                                 "Set logging levels",
                                                 "System",
                                                 processLogLevelCommand));
  commandHandler.registerCommand(CommandDelegate("info",
                                                 "Show system information",
                                                 "System",
                                                 processInfoCommand));
  commandHandler.registerCommand(CommandDelegate("debug",
                                                 "Enable or disable debugging",
                                                 "System",
                                                 processDebugCommand));
  commandHandler.registerCommand(CommandDelegate("restart",
                                                 "Restart the system",
                                                 "System",
                                                 processRestartCommand));
  commandHandler.registerCommand(CommandDelegate("cpu",
                                                 "Adjust CPU speed",
                                                 "System",
                                                 processCpuCommand));
  commandHandler.registerCommand(CommandDelegate("apMode",
                                                 "Adjust the AccessPoint Mode",
                                                 "System",
                                                 processAPModeCommand));
  commandHandler.registerCommand(CommandDelegate("showConfig",
                                                 "Show the current configuration",
                                                 "System",
                                                 processShowConfigCommand));
  commandHandler.registerCommand(CommandDelegate("base-address",
                                                 "Set the base address to use",
                                                 "MySensors",
                                                 processBaseAddressCommand));
  AppSettings.load();

  // Start either wired or wireless networking
  Network.begin(wifiCb);

  // CPU boost
  if (AppSettings.cpuBoost)
    System.setCpuFrequency(eCF_160MHz);
  else
    System.setCpuFrequency(eCF_80MHz);

  // Run WEB server on system ready
  System.onReady(startServers);
  } // init
