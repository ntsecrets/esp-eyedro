const int VERSION_MAJOR = 1;
const int VERSION_MINOR = 01;

// this is a copy of a clock program for the esp8266 that I just modded to run 
// an energy monitor - it polls it via json and displays it.  It has to handle negative and positive values.
// it could be augmented to show the other values too like power factor, amps, volts etc but for now this will do.

const char* www_username = "admin";
const char* updatePath = "/fwupload";

String ID = String(ESP.getChipId(), HEX);

char *www_password = new char[ID.length() + 1];


#include <Timezone.h>


#include <EEPROM.h>
//#include <ESP8266WiFiMulti.h>  //not sure what this was for!!

#include <ESP8266WiFi.h>
#include <WiFiServer.h>

#include <ESP8266WebServer.h>

//#include <ESP8266mDNS.h>
#include "NtpTime.h"

//#include <DNSServer.h>  // for setting up


#include <ESP8266HTTPUpdateServer.h>   //OTA update stuff!!
ESP8266HTTPUpdateServer httpUpdater;



// dns server stuff - setup mode
//DNSServer dnsServer;
IPAddress apIP(192, 168, 4, 1);
//const byte DNS_PORT = 53;

//ESP8266HTTPUpdateServer httpUpdater2; //temporary backdoor


#include "settings.h"
#include "mainPage.h"

#define SETUP_PIN 3

//New feature, Set a password even though its hard coded - WPA2 will establish enough security to prevent spying
#define WPA_PSK "12345678"

#define MODE_SETUP 0
#define MODE_CLOCK 1
int clockMode;
//bool ipshown = false;
//int wificonnects = 0;
uint8_t setupdisp = 0;
//bool synced = false;

// this is for the kwh mode

unsigned long lastpoll = 0;


time_t firstSync = 0;

ESP8266WebServer server (80);

String httpUpdateResponse;

//time_t prevDisplay = 0;

TimeChangeRule DT = settings.LoadDT();
TimeChangeRule ST = settings.LoadST();

TimeChangeRule *tcr;
Timezone myTZ( DT, ST);

void handleNotFound() {
  //server.sendHeader("Location", String("/"), true);
  server.send ( 404, "text/plain", "404 not found!");

}

String GenerateMonthList(uint8_t SelectedItem, String Setting) {
  String ret;
  char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";

  // what we want to return basically is
  // <option value="1">January</option>  and so on
  char monthchar[3];


  ret = "<select name=" + Setting + ">";
  uint8_t i;
  for (i = 0; i < 12; i++) {
    memcpy(monthchar, &months[i * 3], 3);
    monthchar[3] = '\0';
    ret = ret + "<option value=\"" + String(i + 1) + "\"";
    if (i == SelectedItem - 1) {
      ret = ret + " selected ";

    }
    ret = ret + ">" + monthchar + "</option>";

  }

  ret = ret + "</select>";

  return ret;


}

String GenerateHourList(uint8_t SelectedItem, String Setting) {
  String ret;
  ret = "<select name=" + Setting + ">";
  uint8_t i;
  for (i = 0; i < 24; i++) {
    ret = ret + "<option value=\"" + String(i) + "\"";
    if (i == SelectedItem) {
      ret = ret + " selected ";
    }

    ret = ret + ">" + String(i) + "</option>";
  }

  ret = ret + "</select>";

  return ret;

}

String GenerateDimmodeList(uint8_t SelectedItem, String Setting) {
  String ret;
  ret = "<select name=" + Setting + ">";
  uint8_t i;
  for (i = 0; i < 17; i++) {
    ret = ret + "<option value=\"" + String(i) + "\"";
    if (i == SelectedItem) {
      ret = ret + " selected ";
    }

    if (i == 0) {
      ret = ret + ">Auto</option>";
    }
    else {
      ret = ret + ">" + String(i) + "</option>";
    }
  }

  ret = ret + "</select>";

  return ret;

}

String GenerateDayList(uint8_t SelectedItem, String Setting) {
  String ret;
  char *days = "SunMonTueWedThrFriSat";
  char daychar[3];
  ret = "<select name=" + Setting + ">";
  uint8_t i;
  for (i = 0; i < 7; i++) {
    memcpy(daychar, &days[i * 3], 3);
    daychar[3] = '\0';
    ret = ret + "<option value=\"" + String(i + 1) + "\"";
    if (i == SelectedItem - 1) {
      ret = ret + " selected ";
    }

    ret = ret + ">" + daychar + "</option>";
  }

  ret = ret + "</select>";

  return ret;

}

String GenerateWeekList(uint8_t SelectedItem, String Setting) {
  String ret;
  char *weeks = "Lst1st2nd3rd4th";
  char weekchar[3];
  ret = "<select name=" + Setting + ">";
  uint8_t i;
  for (i = 0; i < 5; i++) {
    memcpy(weekchar, &weeks[i * 3], 3);
    weekchar[3] = '\0';
    ret = ret + "<option value=\"" + String(i) + "\"";
    if (i == SelectedItem) {
      ret = ret + " selected ";
    }

    ret = ret + ">" + weekchar + "</option>";
  }

  ret = ret + "</select>";

  return ret;

}

void handleReset() {

  if (!server.authenticate(www_username, String(ESP.getChipId(), HEX).c_str()) && clockMode == MODE_CLOCK) {
    return server.requestAuthentication();
  }

  settings.SaveDefaults();
  eraseSSID();
  ESP.restart();

}

void handleRoot() {

  if (!server.authenticate(www_username, String(ESP.getChipId(), HEX).c_str()) && clockMode == MODE_CLOCK) {
    return server.requestAuthentication();
  }

  String s = MAIN_page;
  s.replace("@@SSID@@", WiFi.SSID());
  //s.replace("@@PSK@@", settings.psk);
  //  s.replace("@@TZ@@", String(settings.timezone));
  s.replace("@@TIME@@", String(tcr -> abbrev) + " " + String(ntp.getTimeDate(myTZ.toLocal(ntp.timestamp))));
  //  s.replace("@@MIN@@", String(minute()));
  s.replace("@@NTPSRV@@", settings.timeserver);
  //s.replace("@@NTPSRV1@@", settings.timeserver1);
  s.replace("@@NTPINT@@", String(settings.interval));
  s.replace("@@SYNCSTATUS@@", ntp.timeIsSynced ? "OK" : "Overdue");
  s.replace("@@CLOCKNAME@@", settings.name);
  s.replace("@@UPDATERESPONSE@@", httpUpdateResponse);
  /* s.replace("@@DAY@@", String(day()));
    s.replace("@@MONTH@@", String(month()));
    s.replace("@@YEAR@@", String(year()));  */
  s.replace("@@VERSION@@", String(VERSION_MAJOR) + "." + String(VERSION_MINOR));
  s.replace("@@BUILDDATE@@", __DATE__);
  s.replace("@@BUILDTIME@@", __TIME__);

  s.replace("@@DST@@", settings.DST);
  s.replace("@@STD@@", settings.STD);


   s.replace("@@REFRESH1@@", String(settings.refresh1));

  s.replace("@@EYEDRO1@@", settings.eyedro1);
   
   
 


  if (settings.twelvehr == 1) {
    s.replace("@@12HRMODE@@", "checked");
  } else {
    s.replace("@@12HRMODE@@", "");
  }

  if (settings.syncind == 1) {
    s.replace("@@SYNCIND@@", "checked");
  } else {
    s.replace("@@SYNCIND@@", "");
  }

  if (settings.centerdot == 1) {
    s.replace("@@CENTERDOT@@", "checked");
  } else {
    s.replace("@@CENTERDOT@@", "");
  }

  if (settings.spacemode == 1) {
    s.replace("@@SPACEMODE@@", "checked");
  } else {
    s.replace("@@SPACEMODE@@", "");
  }

  //dstDayofweek

  s.replace("@@DSTDAYOFWEEK@@", GenerateDayList(settings.dstDayofweek, "dstDayofweek"));

  String dstWeekTxt = "@@DSTWEEKTXT@@";

  s.replace("@@DSTWEEK@@", GenerateWeekList(settings.dstWeek, "dstWeek"));

  s.replace("@@DSTMONTH@@",  GenerateMonthList(settings.dstMonth, "dstMonth"));

  s.replace("@@DSTHOUR@@", GenerateHourList(settings.dstHour, "dstHour"));
  s.replace("@@DSTOFFSET@@", String(settings.dstOffset));

  // STD

  s.replace("@@STDDAYOFWEEK@@", GenerateDayList(settings.stdDayofweek, "stdDayofweek"));

  //stdWeek

  s.replace("@@STDWEEK@@", GenerateWeekList(settings.stdWeek, "stdWeek"));

  s.replace("@@STDMONTH@@", GenerateMonthList(settings.stdMonth, "stdMonth"));

  s.replace("@@STDHOUR@@", GenerateHourList(settings.stdHour, "stdHour"));
  s.replace("@@STDOFFSET@@", String(settings.stdOffset));

  // dim, bright hours
  s.replace("@@DIM@@", GenerateHourList(settings.dim, "dim"));
  s.replace("@@BRIGHT@@", GenerateHourList(settings.bright, "bright"));
  s.replace("@@DIMMODE@@", GenerateDimmodeList(settings.dimmode, "dimmode"));


  s.replace("@@ID@@", String(ESP.getChipId(), HEX));
  s.replace("@@SIGNAL@@", String(WiFi.RSSI()));

  s.replace("@@FUDGE@@", String(settings.fudge));
  // s.replace("@@WIFICONNECTS@@", String(wificonnects));

  s.replace("@@DEBUG@@",  String(settings.DST) + " " + String(settings.dstWeek) + " " + String(settings.dstDayofweek) + " " + String(settings.dstMonth) + " " + String(settings.dstHour) + " " + String(settings.dstOffset) +
            "<br>" + String(settings.STD) + " " + String(settings.stdWeek) + " " + String(settings.stdDayofweek) +  " " + String(settings.stdMonth) + " " + String(settings.stdHour) + " " + String(settings.stdOffset) +
            "<br>Last Sync: " + String(ntp.getTimeDate(myTZ.toLocal(ntp.lastSync))) + "<br>First Sync: " + String(ntp.getTimeDate(myTZ.toLocal(firstSync))) + "<br>LI: " + String(ntp.LI) + "<br>Heap Free: " + String(ESP.getFreeHeap()) +
            "<br>PktDelay: " + String(ntp.pktDelay) + " msec");
  //  s.replace("@@TIMESERVER1@@", "<br>Current NTP Server 1: " + String(NTP.getNTPServer(0)));

  s.replace("@@LASTIP@@", "<br>Last NTP Server IP Used: " + String(ntp.timeServerIP[0]) + "." + String(ntp.timeServerIP[1]) + "." + String(ntp.timeServerIP[2]) + "." + String(ntp.timeServerIP[3]));
  //  if (sizeof(settings.timeserver1 > 0)) {
  //   s.replace("@@TIMESERVER2@@", "<br>Current NTP Server 2: " + String(NTP.getNTPServer(1)));
  //  }


  httpUpdateResponse = "";
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);  //enable chunked encoding

  server.send(200, "text/html", "");  //set the content type
  server.sendContent_P(HEADER_page);

  //send the page
  server.sendContent(s);

}

void handleForm() {


  if (!server.authenticate(www_username, String(ESP.getChipId(), HEX).c_str()) && clockMode == MODE_CLOCK) {
    return server.requestAuthentication();
  }

  //String t_ssid = server.arg("ssid");
  // String t_psk = server.arg("psk");
  String t_timeserver = server.arg("ntpsrv");
  t_timeserver.toCharArray(settings.timeserver, EEPROM_TIMESERVER_LENGTH, 0);
  //t_timeserver = server.arg("ntpsrv1");
  //t_timeserver.toCharArray(settings.timeserver1, EEPROM_TIMESERVER1_LENGTH, 0);

  //dst
  server.arg("dst").toCharArray(settings.DST, EEPROM_DT_LENGTH, 0);
  server.arg("std").toCharArray(settings.STD, EEPROM_ST_LENGTH, 0);

  settings.dstWeek = server.arg("dstWeek").toInt();
  settings.stdWeek = server.arg("stdWeek").toInt();

  settings.dstDayofweek = server.arg("dstDayofweek").toInt();
  settings.stdDayofweek = server.arg("stdDayofweek").toInt();

  settings.dstMonth = server.arg("dstMonth").toInt();
  settings.stdMonth = server.arg("stdMonth").toInt();

  settings.dstHour = server.arg("dstHour").toInt();
  settings.stdHour = server.arg("stdHour").toInt();

  settings.dstOffset = server.arg("dstOffset").toInt();
  settings.stdOffset = server.arg("stdOffset").toInt();
  // settings.dstOffset = -240;
  // settings.stdOffset = -300;
  settings.dim = server.arg("dim").toInt();
  settings.bright = server.arg("bright").toInt();

  settings.fudge = server.arg("fudge").toInt();


  settings.twelvehr = server.arg("twelvehr").toInt();

  settings.syncind = server.arg("syncind").toInt();

  settings.dimmode = server.arg("dimmode").toInt();
  settings.centerdot = server.arg("centerdot").toInt();
  settings.spacemode = server.arg("spacemode").toInt();


server.arg("eyedro1").toCharArray(settings.eyedro1, EEPROM_EYEDRO1_LENGTH, 0);

 settings.refresh1 = server.arg("refresh1").toInt();

  int syncInt = server.arg("ntpint").toInt();;
  if (syncInt < MINIMUM_INTERVAL) {
    syncInt = MINIMUM_INTERVAL;
  }

  settings.interval = syncInt;

  settings.name = server.arg("clockname");
  settings.name.replace("+", " ");

  httpUpdateResponse = "The configuration was updated, power cycle for DST changes to take effect.";

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Moved");

  settings.Save();


  if (server.arg("update_wifi") == "1") {

    setupNewWiFi(server.arg("ssid"), server.arg("psk"), false);
  }

  ntp.ntpServerName = (char*)settings.timeserver;

  if (ntp.syncOffset != settings.interval) {
    ntp.syncOffset = settings.interval;
    ntp.syncTheTime();  //force an immediate re-sync so that the interval is updated
  }


  clockMode = MODE_CLOCK;

}


void setup() {

  //char hostname = char(ESP.getChipId());



  setupDisplay();
  pinMode(SETUP_PIN, INPUT_PULLUP);
  digitalWrite(SETUP_PIN, HIGH);


  server.onNotFound ( handleNotFound );

  server.begin();

  setupWiFi();
  //  setupTime();
  server.on("/", handleRoot);
  server.on("/form", handleForm);
  server.on("/reset", handleReset);

  strcpy(www_password, ID.c_str());  //need this to get the pw to work for some reason with the update svr.

  httpUpdater.setup(&server, updatePath, www_username, www_password);  //ota


  ntp.localPort = (uint16_t)random(0xC000, 0xFFFF);          //generate random port number in range C000-FFFF
  ntp.ntpServerName = (char*)settings.timeserver;
  ntp.syncOffset = settings.interval;
}

void loop() {
  server.handleClient();

  if (!WiFi.SSID()) {
    setupAP();
  }

  if (digitalRead(SETUP_PIN) == 0) {

    displayIP(false);
    delay(1000);
    displayIP(true);
    delay(1000);
    displayID();
    delay(1000);

    digitalWrite(SETUP_PIN, HIGH);
  }



  if (clockMode == MODE_CLOCK) {
    //  if (ntp.timeIsSynced) {

    if (millis() % 10 == 0 ) {
      ntp.addTime();
     // displayClock();
     
     if (millis() - lastpoll > settings.refresh1 * 1000) {
     
     PollKwH();

     lastpoll = millis();

     }


      //connect wifi if not connected
      /*  if (WiFi.status() != WL_CONNECTED) {
         delay(1);
         displayDash();
          setupSTA();
          wificonnects++;
          return;
        }  */
    }
    //   }
    //  }
  } else {

    // dnsServer.processNextRequest();
    yield();

    //mode setup
    if (millis() % 1000 == 0) {
      switch (setupdisp) {
        case 0: displayID();
          setupdisp++;
          break;
        case 1:
          displayIP(false);
          setupdisp++;
          break;
        case 2:
          displayIP(true);
          setupdisp++;
          break;
        case 3:
          display8s();
          setupdisp = 0;
          break;
      }


    }

  }

}

void setupWiFi() {

  bool SmartStatus = false;

  settings.Load();
  // Wait up to 5s for GPIO0 to go low to enter AP/setup mode.
  //displayBusy(0);
  //display8s();  // display test for 1 second (first 1000 ms)
  displayVersion();
  while (millis() < 5000) {
    if (millis() < 2000 && millis() > 1000) {
      display8s();
    } else if (millis() > 3000) {
      displayID();
    }


    // if (digitalRead(SETUP_PIN) == 0 || !settings.ssid.length()) {
    if (digitalRead(SETUP_PIN) == 0 || WiFi.SSID() == "") {
      clockMode = MODE_SETUP;
      // new 5.85 go into SMART setup mode for 2 min then go into regular mode

      SmartStatus = setupSmartMode();
      if (clockMode == MODE_SETUP) {
        return setupAP();

      }

    }
    delay(50);
  }
  // stopDisplayBusy();
  if (!SmartStatus) {   //no need to setup STA if smart config succeeded
    setupSTA();
  }
}


void setupSTA() {

  clockMode = MODE_CLOCK;
  // https://github.com/esp8266/Arduino/issues/2186 - need the stuff below to fully reset an existing conn.
  WiFi.persistent(false);

  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);

  WiFi.setAutoConnect(true);

  // end conn reset
  // set the hostname
  WiFi.hostname(String("espclock-") + String(ESP.getChipId(), HEX).c_str());


  WiFi.begin();

  while (WiFi.status() != WL_CONNECTED) {
    displayDash();
    delay(1000);
    displayID();
    delay(1000);

  }

  displayIP(false);
  delay(1000);
  displayIP(true);
  delay(1000);
  WiFi.persistent(true);
}


void setupNewWiFi(String sSSID, String PSK, bool Smart) {

  clockMode = MODE_CLOCK;
  WiFi.mode(WIFI_STA);
  WiFi.hostname(String("espclock-") + String(ESP.getChipId(), HEX).c_str());

  if (Smart) {
    WiFi.begin(); // should already be set?
  }
  else if (PSK) {
    WiFi.begin(sSSID.c_str(), PSK.c_str());

  } else {
    WiFi.begin(sSSID.c_str());

  }


  while (WiFi.status() != WL_CONNECTED) {
    displayDash();
    delay(1000);
    displayID();
    delay(1000);
  }

  displayIP(false);
  delay(1000);
  displayIP(true);
  delay(1000);

}


void eraseSSID() {
  WiFi.disconnect();

}

void setupAP() {

  clockMode = MODE_SETUP;

  int channel = random(1, 12);

  WiFi.mode(WIFI_AP);
  WiFi.softAP((String(WIFI_AP_NAME) + String(ESP.getChipId(), HEX)).c_str(), WPA_PSK, channel);

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  // dnsServer.start(DNS_PORT, "espclock.com", apIP);

  displayAP();


}

bool setupSmartMode() {
  clockMode = MODE_SETUP;

  // start the smart config mode
  //  WiFi.mode(WIFI_AP_STA);
  //WiFi.mode(WIFI_OFF);
  WiFi.beginSmartConfig();

  while (millis() < 60000) {
    if (!WiFi.smartConfigDone()) {
      delay(1000);
      displayUseApp();
      delay(1000);
      displayDash();
    }  else {
      // returnd true so we shoud exit setup mode and carry on

    // WiFi.stopSmartConfig();
      clockMode = MODE_CLOCK;
      displayDash();
      delay(2000);  // might make this fancier in the future to show success message but for now just dashes
      


      return true;
    }
  }
  WiFi.stopSmartConfig();
  return false;
}
