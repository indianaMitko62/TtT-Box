#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <Wire.h>
#include "bsec.h"
#include <sstream>
#include <string.h>
#include <SoftwareSerial.h>
#include "ThingSpeak.h"


extern String output; //ouput string with the sensor data
extern String K30_output; //ouput string with the K30 data

extern const char home_html[];
extern const char advanced_html[];
extern const char PROGMEM upload_html[];
extern const char index_html[];
extern const char changeAcc_html[];
extern const char login_html[];

WiFiClient  client; 
DNSServer dnsServer;
AsyncWebServer server(80);

extern size_t updateSize;
extern String enteredUsername;
extern String enteredPassword;

extern String output;
extern Bsec iaqSensor;
extern unsigned long myChannelNumber_bme;
extern const char * myWriteAPIKey_bme;

struct bme_680
{
  float pressure, gasResistance, iaq, temperature, humidity, staticIaq, co2Equivalent, breathVocEquivalent, rawTemperature, rawHumidity, iaqAccuracy;
};

void debugFunction(const String& debug_text);
void update_client(void);

struct bme_680 data; 
float times_mid_range_bme = 3; float devider_bme = times_mid_range_bme;

SoftwareSerial opc_serial(13, 27);
String opc_data;


struct opc
{
  float SFR, Temp, RH, PM_1000, PM_2500, PM_10000;
};

String ssid_AP = "Weather Station";
String password_AP = "1234567890";

const char* PARAM_SSID = "ssid";
const char* PARAM_PASS = "pass";
const char* PARAM_AP_SSID = "ap_ssid";
const char* PARAM_AP_PASS = "ap_pass";
const char* PARAM_USERNAME = "username";
const char* PARAM_PASSWORD = "password";
const char* PARAM_LOGIN_USERNAME = "loginUsername";
const char* PARAM_LOGIN_PASSWORD = "loginPassword";

String enteredUsername;
String enteredPassword;


const char* PARAM_DELAY = "delay";

const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";

const char *ssid_manual = "Ndd_wrt";
const char *pass_manual = "tania123";

int debug_serial = 0;
int opc_pin = 4;
bool opc_working = false;

Bsec iaqSensor; // Create an object of the class Bsec
String output; //ouput string with the sensor data
String K30_output; //ouput string with the K30 data

//===================================BME===============================
unsigned long myChannelNumber_bme = 3;
const char * myWriteAPIKey_bme = "LY49LLNS7Q0775I5";

//=================================k30=================================
unsigned long myChannelNumber_k30 = 1;
const char * myWriteAPIKey_k30 = "SYTUUQQUM1V3W4RY";

//=================================opc=================================
unsigned long myChannelNumber_opc = 4;
const char * myWriteAPIKey_opc = "MI73FLI93A4AWKLT";

// //===================================BME  2===============================
// unsigned long myChannelNumber_bme = 3;
// const char * myWriteAPIKey_bme = "3EQLD7UH1OCNF0XV";

// //=================================k30  2=================================
// unsigned long myChannelNumber_k30 = 4;
// const char * myWriteAPIKey_k30 = "O27TPZ7G4SI1J00T";

unsigned long lastTime = 0;
unsigned long timerDelay = 30000;
size_t updateSize = 0;
unsigned int delay_ = 5000;
String debug1;
int co2Addr = 0x68;

float times_mid_range_k30 = 3; float devider_k30 = times_mid_range_k30;
float co2_buff = 0;

int time_trigger = millis();

void checkIaqSensorStatus(); // BME
void errLeds();
void update_output();
void put_in_thingspeak(); // Thingspeak
void debugFunction(const String& debug_text);

/// config page ///
extern WiFiClient  client; 
extern DNSServer dnsServer;
extern AsyncWebServer server;

/// html data structs /// 


void debugFunction(const String& debug_text)
{
  if(debug1.toInt())
    Serial.println(debug_text);
}

// Replaces placeholder with button section in your web page



const char home_html[] = 
R"rawliteral(
<html>
<script> 
  function logoutButton() 
  {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "logout", true);
    xhr.send();
    setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
  }
</script>
<center>  
<h1 class = header >Configuration Page</h1>
  <form action="/update"> <input class = btn type = "submit" value="Firmware Update"></form>
  <form action="/wifi"> <input class = btn type = "submit" value="Wi-Fi Configuration"/></form>
  <form action="/data"> <input class = btn type = "submit" value="Station Data"/></form>
  <form action="/advanced"> <input class = btn type = "submit" value="Advanced Configuration"/></form>
  <form action="/changeAcc"> <input class = btn type = "submit" value="Account Settings"/></form>
  <button onclick="logoutButton()">Logout</button>
  <p class = brand >TtTBox</p>      
  <p class = name >Build by Dimitar and Radoslav</p>
</center>
  <style>
    input{background:#f1f1f1;border:0;padding:0 15px; font-size:23px}
    body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}
    form{background:#fff;max-width:450px; margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:23px;outline-style: outset}
    .btn{background:#E48A01;color:#000000;cursor:pointer; border-radius:5px; font-size:23px; max-width:500px; padding-left:50px; padding-right:50px;}
    .header{background:#fff;color:#000000;max-width:450px;margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:40px; margin-top:100px}
    .brand{background:#fff; font-size:250%;color:#000000; margin-top: 200px; max-width:350px; border-radius:5px; height:45px}
    .name {background:#fff; font-size:200%;color:#000000; margin-top: 1px; max-width:350px; border-radius:5px;}
  </style>
</html>
)rawliteral";

const char advanced_html[] = 
R"rawliteral(
<html><head></head>
<body>
<center>
  <h1 class = header >Advanced Configuration</h1>
  %BUTTONPLACEHOLDER%
  <h1 class = header2>Delay Configuration</h1>
  <form action="/get" target="hidden-form">
    Current delay: %delay% ms <br> Enter new delay(in ms):<br><br> <input autocomplete="off" type="text" name="delay"><br><br>
    <input class = btn type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <h1 class = header2>AP configuration</h1>
  <form action="/get" target="hidden-form">
    current SSID: %ap_ssid% <br> Enter new AP SSID:<br><br> <input autocomplete="off" type="text" name="ap_ssid"><br><br>
    <input class = btn type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <form action="/get" target="hidden-form">
    current password: %ap_pass% <br> Enter new AP password (at least 8 symbols): <br><br> <input autocomplete="off" type="text" name="ap_pass"><br><br>
    <input class = btn type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <iframe style="display:none" name="hidden-form"></iframe>
  <p class = brand >TtTBox</p>      
  <p class = name >Build by Dimitar and Radoslav</p>
</center>
  <script>
    function toggleCheckbox(element) 
    {
      var xhr = new XMLHttpRequest();
      if(element.checked){ xhr.open("GET", "/get?output="+element.id+"&state=1", true); }
      else { xhr.open("GET", "/get?output="+element.id+"&state=0", true); }
      xhr.send();
    }
    function submitMessage() 
    {
      alert("Saved value to Weather Station");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script>
  <style>
    input{background:#f1f1f1;border:0;padding:0 15px; font-size:23px}
    body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}
    form{background:#fff;max-width:450px; margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:23px}
    .btn{background:#E48A01;color:#000000;cursor:pointer; border-radius:5px; font-size:23px; max-width:500px; padding-left:50px; padding-right:50px;}
    .header{background:#fff;color:#000000;max-width:450px;margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:40px; margin-top:100px}
    .header2{background:#fff;color:#000000;max-width:450px; margin-top:35px; margin-bottom:30px; padding:10px;border-radius:5px;text-align:center; font-size:23px; margin-top:100px}
    .switch {position: relative; display: inline-block; width: 132px; height: 80px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #3498db; border-radius: 6px; border: 5px solid black; border-color:#000}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #E48A01}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .brand{background:#fff; font-size:250%;color:#000000; margin-top: 200px; max-width:350px; border-radius:5px; height:45px}
    .name {background:#fff; font-size:200%;color:#000000; margin-top: 1px; max-width:350px; border-radius:5px;}
  </style>
</body>
</html>
)rawliteral";

String data_html =
R"rawliteral(
  <!DOCTYPE html><html>
    <head>
      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
      <link rel=\"icon\" href=\"data:,\">
      <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;} text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;} </style>
    </head>
    <body>
      <h1>BME680 Data</h1>
      <p>no data</p>
    </body>
  </html>
)rawliteral";

//for OTA
const char PROGMEM upload_html[] = 
R"rawliteral(
<html>
<h1 class = header>Firmware Updater</h1>
<form method='POST' enctype='multipart/form-data'>
<input id='file' type='file' name='update' onchange='sub(this)' style=display:none>
<label id='file-input' for='file'>Click to choose .bin file</label>
<input id='btn' type='submit' class=btn value='Upload'>
</form>
<p class = brand >TtTBox</p>      
<p class = name >Build by Dimitar and Radoslav</p>
<script>
  function sub(obj) 
  {
    var fileName = obj.value.split('\\\\');
    console.log(fileName);
    document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];
  };
</script>
<style>
  #file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}
  input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}
  #file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:center;display:block;cursor:pointer}
  form{background:#fff;max-width:450px;margin:50px auto;padding:10px;border-radius:5px;text-align:center}
  .btn{background:#E48A01;color:#000000;cursor:pointer; border-radius:5px; font-size:23px; max-width:500px; padding-left:50px; padding-right:50px;}
  .header{background:#fff;color:#000000;max-width:450px;margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:40px}
  .brand{background:#fff; font-size:250%;color:#000000; margin-top: 200px; max-width:350px; border-radius:5px; height:45px}
  .name {background:#fff; font-size:200%;color:#000000; margin-top: 1px; max-width:350px; border-radius:5px;}
</style>
</html>
)rawliteral";

//for SPIFFS
const char index_html[] = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to Weather Station");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <h1 class = header> Wi-Fi configuration</h1>
  <form action="/get" target="hidden-form">
    SSID: (current value: "%ssid%"):<br><br> <input autocomplete="off" type="text" name="ssid"><br><br>
    <input class = btn type="submit" value="Submit" onclick="submitMessage()">
  </form>
    <form action="/get" target="hidden-form">
    pass: (current value: "%pass%"): <br><br> <input autocomplete="off" type="text" name="pass"><br><br>
    <input class = btn type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <p class = brand >TtTBox</p>      
  <p class = name >Build by Dimitar and Radoslav</p>
  <iframe style="display:none" name="hidden-form"></iframe>
  <style> 
    input{background:#D4D4D4;border:0;padding:0 15px; font-size:23px}
    body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}
    form{background:#fff;max-width:450px; margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:23px}
    .btn{background:#E48A01;color:#000000;cursor:pointer; border-radius:5px; font-size:23px; max-width:500px; padding-left:50px; padding-right:50px;}
    .header{background:#fff;color:#000000;max-width:450px;margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:40px}
    .brand{background:#fff; font-size:250%;color:#000000; margin-top: 200px; max-width:350px; border-radius:5px; height:45px}
    .name {background:#fff; font-size:200%;color:#000000; margin-top: 1px; max-width:350px; border-radius:5px;}
  </style>
</body></html>)rawliteral";

const char changeAcc_html[] = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to Weather Station");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <h1 class = header> Wi-Fi configuration</h1>
  <form action="/get" target="hidden-form">
    new username: (current value: "%username%"):<br><br> <input autocomplete="off" type="text" name="username"><br><br>
    <input class = btn type="submit" value="Submit" onclick="submitMessage()">
  </form>
    <form action="/get" target="hidden-form">
    new password: (current value: "%password%"): <br><br> <input autocomplete="off" type="text" name="password"><br><br>
    <input class = btn type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <p class = brand >TtTBox</p>      
  <p class = name >Build by Dimitar and Radoslav</p>
  <iframe style="display:none" name="hidden-form"></iframe>
  <style> 
    input{background:#D4D4D4;border:0;padding:0 15px; font-size:23px}
    body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}
    form{background:#fff;max-width:450px; margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:23px}
    .btn{background:#E48A01;color:#000000;cursor:pointer; border-radius:5px; font-size:23px; max-width:500px; padding-left:50px; padding-right:50px;}
    .header{background:#fff;color:#000000;max-width:450px;margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:40px}
    .brand{background:#fff; font-size:250%;color:#000000; margin-top: 200px; max-width:350px; border-radius:5px; height:45px}
    .name {background:#fff; font-size:200%;color:#000000; margin-top: 1px; max-width:350px; border-radius:5px;}
  </style>
</body></html>)rawliteral";

const char login_html[] = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to Weather Station");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <h1 class = header>Login Configuration</h1>
  <form action="/get" target="hidden-form">
    username: <br><br> <input autocomplete="off" type="text" name="loginUsername"><br><br>
    <input class = btn type="submit" value="Submit" onclick="submitMessage()">
  </form>
    <form action="/get" target="hidden-form">
    password: <br><br> <input autocomplete="off" type="text" name="loginPassword"><br><br>
    <input class = btn type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <p class = brand >TtTBox</p>      
  <p class = name >Build by Dimitar and Radoslav</p>
  <iframe style="display:none" name="hidden-form"></iframe>
  <style> 
    input{background:#D4D4D4;border:0;padding:0 15px; font-size:23px}
    body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}
    form{background:#fff;max-width:450px; margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:23px}
    .btn{background:#E48A01;color:#000000;cursor:pointer; border-radius:5px; font-size:23px; max-width:500px; padding-left:50px; padding-right:50px;}
    .header{background:#fff;color:#000000;max-width:450px;margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:40px}
    .brand{background:#fff; font-size:250%;color:#000000; margin-top: 200px; max-width:350px; border-radius:5px; height:45px}
    .name {background:#fff; font-size:200%;color:#000000; margin-top: 1px; max-width:350px; border-radius:5px;}
  </style>
</body></html>)rawliteral"; 

void update_client(void)
{
  data_html = 
  R"rawriteral(
  <!DOCTYPE html><html>
  <head>
    <meta http-equiv="refresh" content="10" name=\"viewport\" content=\"width=device-width, initial-scale=1\">
    <link rel=\"icon\" href=\"data:,\">
    <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;} text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;} </style>
  </head>
  <body>
  <h1 class = header>Sensor Data</h1>
  <div class="row">
    <div class="column" style="background-color:#fff;">
      <h2 class = header2>K30 Data</h2>
      <p class = data>)rawriteral" + K30_output + R"rawriteral(</p>
    </div>
    <div class="column" style="background-color:#fff;">
      <h2 class = header2>BME680 Data</h2>
      <p class = data>)rawriteral" + output + R"rawriteral(</p>
    </div>
    <div class="column" style="background-color:#fff;">
      <h2 class = header2>Dust Sensor Data</h2>
      <p class = data>ppm</p>
    </div>
  </div>
  <p class = brand >TtTBox</p>      
  <p class = name >Build by Dimitar and Radoslav</p>
  <style>
  *{box-sizing: border-box;}
  .column {float: left;width: 330px;padding: 10px;margin-left: 230px;margin-top: 10px; text-align: center;border-radius:5px;}
  .row:after{content: "";display: table;clear: both;}
  .header{background:#fff;color:#000000;max-width:600px;margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:40px}
  .header2{background:#fff;color:#000000;max-width:600px;margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:30px}
  .data{background:#fff;max-width:450px; margin:50px auto;padding:10px;border-radius:5px;text-align:center; font-size:20px}
  body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}
  .brand{background:#fff; font-size:250%;color:#000000; margin-top: 200px; max-width:350px; border-radius:5px; height:45px}
  .name {background:#fff; font-size:200%;color:#000000; margin-top: 1px; max-width:350px; border-radius:5px;}
  </style>
  </body>
  </html>
  )rawriteral";
}


String readFile(fs::FS &fs, const char * path) //SPIFFS
{
  File file = fs.open(path, "r");
  if(!file || file.isDirectory())
    return String();
  String fileContent;
  while(file.available())
    fileContent+=String((char)file.read());
  file.close();
  debugFunction(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message)
{
  debugFunction("Writing file:" + String(path) + "\r\n");
  File file = fs.open(path, "w");
  if(!file)
  {
    debugFunction("- failed to open file for writing");
    return;
  }
  if(file.print(message))
    debugFunction("- file written");
  else 
    debugFunction("- write failed");
  file.close();
}


String outputState(int output)
{
  return digitalRead(output) ? "checked" : "";
}

String outputState1()
{
  return readFile(SPIFFS, "/debugButton.txt").toInt() ? "checked" : "";
}


String processor_advanced(const String& var)
{
  debugFunction(var);
  if(var == "BUTTONPLACEHOLDER")
    return "<h4 class=header2 >DEBUG mode</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState1() + "><span class=\"slider\"></span></label>";
  else if(var == "delay")
    return readFile(SPIFFS, "/delay.txt");
  else if(var == "ap_ssid")
    return readFile(SPIFFS, "/ap_ssid.txt");
  else if(var == "ap_pass")
    return readFile(SPIFFS, "/ap_pass.txt");
  return String();
}

//OTA
void updateRequestHandler(AsyncWebServerRequest *request)
{
  //Serial.println(F("updateRequestHandler"));
  debugFunction("updateRequestHandler");
  request->send(200);
  ESP.restart();
}

void updateHandler(uint8_t *data, size_t len, size_t index, size_t total, bool final)
{
  if (index == 0) 
  {
    debugFunction("Update started!");
    debugFunction("Total size:" + String(total));
    updateSize = total;
    if (!Update.begin(total))
    {
      debugFunction("Upload begin error: ");
      Update.printError(Serial);
      return;
    }
  }
  if (Update.write(data, len) != len) 
  {
    debugFunction("Upload write error: ");
    Update.printError(Serial);
    return;
  }
  if (final) 
  {
    if (!Update.end(true)) 
    {
      debugFunction("Upload end error: ");
      Update.printError(Serial);
      return;
    }
    debugFunction("Update Success:" + String(index+len) + "\nRebooting...\n");
    return;
  }
}

void updateBodyHandler(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
    updateHandler(data, len, index, total, index + len == total);
}

void updateFileHandler(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final)
{
    updateHandler(data, len, index, request->contentLength(), final);
}

void notFound(AsyncWebServerRequest *request) 
{
  request->send(404, "text/plain", "Not found");
}

bool loggedIn()
{
    String username = readFile(SPIFFS, "/username.txt");
    String password = readFile(SPIFFS, "/password.txt");
    if(username != enteredUsername || password != enteredPassword)
        return false;
    return true;
}   


String processor(const String& var) // Replaces placeholder with stored values
{
  if(var == "ssid")
    return readFile(SPIFFS, "/ssid.txt");
  if (var == "pass")
    return readFile(SPIFFS, "/pass.txt");
  return String();
}

String processor_newAccInfo(const String& var)
{
    if(var == "username")
        return readFile(SPIFFS, "/username.txt");
    if (var == "password")
        return readFile(SPIFFS, "/password.txt");
    return String();
}



void update_output()
{
  output += "\nRaw temp: " + String(iaqSensor.rawTemperature) + " C<br>";
  output += "\nPressure: " + String(iaqSensor.pressure) + " kPa<br>";
  output += "\nRaw humidity:" + String(iaqSensor.rawHumidity) + " %<br>";
  output += "\nGas resistance: " + String(iaqSensor.gasResistance) + " Ohm  <br>";
  output += "\nIAQ: " + String(iaqSensor.iaq) + "<br>";
  output += "\nIAQAccuracy: " + String(iaqSensor.iaqAccuracy) + "<br>";
  output += "\nTemperature: " + String(iaqSensor.temperature) + " C <br>";
  output += "\nHimidity: " + String(iaqSensor.humidity) + " %<br>";
  output += "\nStaticIaq: " + String(iaqSensor.staticIaq) + "<br>";
  output += "\nCO2Equivalent " + String(iaqSensor.co2Equivalent) + " ppm<br>";
  output += "\nBreathVocEquivalent " + String(iaqSensor.breathVocEquivalent) + " ppm<br><br><br>\n\n\n";
}

void clear_bme_data()
{
  data.pressure = 0;
  data.gasResistance = 0;
  data.iaq = 0;
  data.temperature = 0;
  data.humidity = 0;
  data.staticIaq = 0;
  data.co2Equivalent = 0;
  data.breathVocEquivalent = 0;
  data.rawTemperature = 0;
  data.rawHumidity = 0;
  data.iaqAccuracy = 0;
}

void bme_upload()
{
  if(!iaqSensor.run()) return;
  if(times_mid_range_bme >= 1)
  { 
    //mid_data->rawTemperature = (iaqSensor.rawTemperature);
    data.pressure += iaqSensor.pressure;
    //mid_data->rawHumidity = (iaqSensor.rawHumidity);
    data.gasResistance += iaqSensor.gasResistance;
    data.iaq += iaqSensor.iaq;
    //mid_data->iaqAccuracy = (iaqSensor.iaqAccuracy);
    data.temperature += iaqSensor.temperature;
    data.humidity += iaqSensor.humidity;
    data.staticIaq += iaqSensor.staticIaq;
    data.co2Equivalent += iaqSensor.co2Equivalent;
    data.breathVocEquivalent += iaqSensor.breathVocEquivalent;
    //Serial.println(data.temperature);
    //update_output();
    times_mid_range_bme--;
    // Serial.println(times_mid_range_bme);
    // Serial.println("\n");
    return;
  }
  output = "";

  update_client();
  ThingSpeak.setField(1, data.pressure/devider_bme);
  ThingSpeak.setField(2, data.gasResistance/devider_bme);
  ThingSpeak.setField(3, data.iaq/devider_bme);
  ThingSpeak.setField(4, data.temperature/devider_bme);
  ThingSpeak.setField(5, data.humidity/devider_bme);  
  ThingSpeak.setField(6, data.staticIaq/devider_bme);
  ThingSpeak.setField(7, data.co2Equivalent/devider_bme);
  ThingSpeak.setField(8, data.breathVocEquivalent/devider_bme);
  update_output();
  update_client();
  times_mid_range_bme = devider_bme;
  int bme = ThingSpeak.writeFields(myChannelNumber_bme, myWriteAPIKey_bme);
  if(bme == 200) //Serial.println("Channel update successful.");
    debugFunction("BME Channel update successful.");
  else
    debugFunction("Problem updating BME channel. HTTP error code " + String(bme));
  clear_bme_data();
}

void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK)
  {
    if (iaqSensor.status < BSEC_OK)
    {
      output = "BSEC error code : " + String(iaqSensor.status);
      debugFunction(output);
      errLeds(); /* Halt in case of failure */
    }
    else
    {
      output = "BSEC warning code : " + String(iaqSensor.status);
      debugFunction(output);
    }
  }
  if (iaqSensor.bme680Status != BME680_OK)
  {
    if (iaqSensor.bme680Status < BME680_OK)
    {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      debugFunction(output);
      errLeds();
    }
    else
    {
      output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      debugFunction(output);
    }
  }
}

void errLeds(void)
{
  for(int i = 0; i < 30; i++)
  {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }
  ESP.restart();
}

int readCO2()
{
  int co2_value = 0;  // We will store the CO2 value inside this variable.
  Wire.beginTransmission(co2Addr);
  Wire.write(0x22);
  Wire.write(0x00);
  Wire.write(0x08);
  Wire.write(0x2A);
  Wire.endTransmission();
  delay(10);
  Wire.requestFrom(co2Addr, 4);
  byte i = 0;
  byte buffer[4] = {0, 0, 0, 0};

  while (Wire.available())
  {
    buffer[i] = Wire.read();
    i++;
  }
  co2_value = 0;
  co2_value |= buffer[1] & 0xFF;
  co2_value = co2_value << 8;
  co2_value |= buffer[2] & 0xFF;
  byte sum = 0; //Checksum Byte
  sum = buffer[0] + buffer[1] + buffer[2]; //Byte addition utilizes overflow
  if (sum == buffer[3])
    return co2_value; // Success!
  else
    return 0;
}

void C02_upload()
{
  float co2Value = readCO2();
  co2_buff += co2Value;
  if (times_mid_range_k30 == 1)
  {
    times_mid_range_k30 = devider_k30;
    K30_output = "";
    update_client();
    debugFunction("CO2 Value: " + String(co2_buff));
    int x = ThingSpeak.writeField(myChannelNumber_k30, 1, co2_buff/devider_k30, myWriteAPIKey_k30);
    K30_output = "CO2 Value: " + String(co2_buff/devider_k30) + " ppm";
    co2_buff = 0;
    if(x == 200) 
      debugFunction("K30 Channel update successful.");
    else
      debugFunction("Problem updating K30 channel. HTTP error code " + String(x)); 
  }
  if(co2Value <= 0)
    debugFunction("Checksum failed / Communication failure");
  else
  {
    times_mid_range_k30--;
    debugFunction(String(times_mid_range_k30) + "\n");
  }
}

void setup()
{
  Serial.begin(115200);
  opc_serial.begin(9600);
  opc_serial.setTimeout(10000);

  if(!SPIFFS.begin(true))   // Initialize SPIFFS
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  ssid_AP = readFile(SPIFFS, "/ap_ssid.txt");
  password_AP = readFile(SPIFFS, "/ap_pass.txt");
  //password_AP = "1234567890";
  if(ssid_AP == "" || password_AP == "")
  {
    ssid_AP = "Weather Station";
    password_AP = "1234567890";
  }
  Wire.begin();
  WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);  
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(
    IPAddress(8, 8, 8, 8),
    IPAddress(8, 8, 8, 8),
    IPAddress(255, 255, 255, 0)
  );
  Serial.print(ssid_AP);
  Serial.print(password_AP);
  if (!WiFi.softAP(ssid_AP.c_str(), password_AP.c_str())) 
  {
    Serial.println(F("Error starting AP"));
    ESP.restart();
    return;
  }
  pinMode(opc_pin, OUTPUT);
  debugFunction("AP started");
  debugFunction("AP IP address: ");
  debugFunction(String(WiFi.softAPIP()));
  String username = readFile(SPIFFS, "/username.txt");
  String password = readFile(SPIFFS, "/password.txt");
  if(username == "" || password == "")
  {
    writeFile(SPIFFS, "/username.txt", "admin");
    writeFile(SPIFFS, "/password.txt", "pass");
    Serial.println("alo da");
  }
  Serial.println("\n" + username + "\n" + password);
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.on(PSTR("/update"), HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String username = readFile(SPIFFS, "/username.txt");
    String password = readFile(SPIFFS, "/password.txt");
    while(!request->authenticate(username.c_str(), password.c_str()))
      request->requestAuthentication();
    request->send_P(200, "text/html", upload_html);
  });

  server.on(PSTR("/update"), HTTP_POST, updateRequestHandler, updateFileHandler, updateBodyHandler);

  server.onNotFound([](AsyncWebServerRequest *request)
  {
    request->redirect(F("/update"));
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String username = readFile(SPIFFS, "/username.txt");
    String password = readFile(SPIFFS, "/password.txt");
    while(!request->authenticate(username.c_str(), password.c_str()))
      request->requestAuthentication();
    request->send_P(200, "text/html", home_html);
  });

  server.on("/debug", HTTP_GET, [] (AsyncWebServerRequest *request)
  {
    String username = readFile(SPIFFS, "/username.txt");
    String password = readFile(SPIFFS, "/password.txt");
    while(!request->authenticate(username.c_str(), password.c_str()))
      request->requestAuthentication();
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) 
    {
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    }
    else 
    {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    debugFunction(" Debug value was: " + String(debug_serial));
    debug_serial = readFile(SPIFFS, "/debugButton.txt").toInt();
    debugFunction(" Debug value set to: " + String(inputMessage2));
    request->send(200, "text/plain", "OK");
  });

  server.on("/wifi", HTTP_GET,[](AsyncWebServerRequest *request)
  {
    String username = readFile(SPIFFS, "/username.txt");
    String password = readFile(SPIFFS, "/password.txt");
    while(!request->authenticate(username.c_str(), password.c_str()))
      request->requestAuthentication();
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/changeAcc", HTTP_GET,[](AsyncWebServerRequest *request)
  {
    String username = readFile(SPIFFS, "/username.txt");
    String password = readFile(SPIFFS, "/password.txt");
    while(!request->authenticate(username.c_str(), password.c_str()))
      request->requestAuthentication();
    request->send_P(200, "text/html", changeAcc_html, processor_newAccInfo);
  });

  server.on("/login", HTTP_GET,[](AsyncWebServerRequest *request)
  {
    request->send_P(200, "text/html", login_html);
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String username = readFile(SPIFFS, "/username.txt");
    String password = readFile(SPIFFS, "/password.txt");
    while(!request->authenticate(username.c_str(), password.c_str()))
      request->requestAuthentication();
    request->send_P(200, "text/html", data_html.c_str());
  });
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(401);
  });
  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->redirect("/");
  });
  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) 
  {
    request->send_P(200, "text/html", index_html, processor);
    String inputMessage;
    String pass;
    String delay_string;
    String ssid_ap;
    String pass_ap;
    String username;
    String password;
    String inputMessage2;
    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if (request->hasParam(PARAM_SSID)) 
    {
      inputMessage = request->getParam(PARAM_SSID)->value();
      writeFile(SPIFFS, "/ssid.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_PASS)) 
    {
      pass = request->getParam(PARAM_PASS)->value();
      writeFile(SPIFFS, "/pass.txt", pass.c_str());
    }
    else if (request->hasParam(PARAM_DELAY))
    {
      delay_string = request->getParam(PARAM_DELAY)->value();
      writeFile(SPIFFS, "/delay.txt", delay_string.c_str());
    }
    else if(request->hasParam(PARAM_AP_SSID))
    {
      ssid_ap = request->getParam(PARAM_AP_SSID)->value();
      writeFile(SPIFFS, "/ap_ssid.txt", ssid_ap.c_str());
    }
    else if(request->hasParam(PARAM_AP_PASS))
    {
      pass_ap = request->getParam(PARAM_AP_PASS)->value();
      writeFile(SPIFFS, "/ap_pass.txt", pass_ap.c_str());
    }
    else if(request->hasParam(PARAM_USERNAME))
    {
      username = request->getParam(PARAM_USERNAME)->value();
      writeFile(SPIFFS, "/username.txt", username.c_str());
    }
    else if(request->hasParam(PARAM_PASSWORD))
    {
      password = request->getParam(PARAM_PASSWORD)->value();
      writeFile(SPIFFS, "/password.txt", password.c_str());
    }
    else if(request->hasParam(PARAM_LOGIN_USERNAME))
    {
      enteredUsername = request->getParam(PARAM_LOGIN_USERNAME)->value();
    }
    else if(request->hasParam(PARAM_LOGIN_PASSWORD))
    {
      enteredPassword = request->getParam(PARAM_LOGIN_PASSWORD)->value();
    }
    else if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) 
    {
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      writeFile(SPIFFS, "/debugButton.txt", inputMessage2.c_str());
    }
    else
    {
      inputMessage = "No message sent";
    }
    debugFunction(inputMessage);
    request->send(200, "text/text", inputMessage);
    debugFunction(pass);
    request->send(200, "text/text", pass);
    debugFunction(delay_string);
    request->send(200, "text/text", delay_string);
  });

  server.on("/advanced", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String username = readFile(SPIFFS, "/username.txt");
    String password = readFile(SPIFFS, "/password.txt");
    while(!request->authenticate(username.c_str(), password.c_str()))
      request->requestAuthentication();
    //String output = digitalRead(2) ? "1" : "0";
    //writeFile(SPIFFS, "/debugButton.txt", output.c_str());
    request->send_P(200, "text/html", advanced_html, processor_advanced);
  });

  server.onNotFound(notFound);
  server.begin();
  debugFunction("Configuration server started");
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  Wire.begin();
  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
  output = "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  debugFunction(output);
  checkIaqSensorStatus();
  bsec_virtual_sensor_t sensorList[10] = 
  {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };
  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();
  clear_bme_data();
  String ssid_WIFI = readFile(SPIFFS, "/ssid.txt");
  String pass_WIFI = readFile(SPIFFS, "/pass.txt");
  debugFunction(ssid_WIFI);
  debugFunction(pass_WIFI);
}

void odd2(std::string data)
{
  std::size_t data_start = data.find("D");
  int cnt_param = 0, PM_1000_cnt = 0, PM_2500_cnt = 0, PM_10000_cnt = 0;
  std::string useful_data = data.substr(data_start);
  //Serial.println(useful_data.c_str());
  std::string temp = " ";
  struct opc result = {0,0,0,0,0,0};
  for (std::string::size_type i = 1; i < useful_data.size(); i++)
  {
    //Serial.println(useful_data.substr(i).c_str());
    if(useful_data.compare(i, 1, ",") == 0 && cnt_param <= 5)
    {
      switch(cnt_param)
      {
        case 0:
          result.SFR += atof(temp.c_str());
          break;
        case 1:
          result.Temp += atof(temp.c_str());
          Serial.println("Temp Sum: " + String(result.Temp));
          Serial.println(cnt_param);
          break;
        case 2:
          result.RH += atof(temp.c_str());
          break;
        case 3:
          if (atof(temp.c_str()) == 0) break;
          result.PM_1000 += atof(temp.c_str());
          PM_1000_cnt++;
          break;
        case 4:
          if (atof(temp.c_str()) == 0) break;
          result.PM_2500 += atof(temp.c_str());
          PM_2500_cnt++;
          break;
        case 5:
          if (atof(temp.c_str()) == 0) break;
          result.PM_10000 += atof(temp.c_str());
          PM_10000_cnt++;
          break;
      }
      cnt_param++;
      temp = "";
      continue;
    }
    if(useful_data.compare(i , 1, "\n") == 0)
    {
      useful_data = useful_data.substr(useful_data.find("\n"));
      //Serial.println("alo");
      //Serial.println(useful_data.c_str());
      std::size_t helper = useful_data.find("D");
      if (helper == std::string::npos) break;
      useful_data = useful_data.substr(helper);
      //Serial.println("alo1");
      //Serial.println(useful_data.c_str());
      i = 0;
      cnt_param = 0;
      temp = "";
      continue;
    }
    temp += useful_data[i];
    //Serial.println("temp:");
    //Serial.println(temp.c_str());
  }  
  result.Temp /= 9;
  result.RH /= 9;
  result.SFR /= 9;
  ThingSpeak.setField(1, result.Temp);
  ThingSpeak.setField(2, result.RH);
  ThingSpeak.setField(6, result.SFR);
  if (PM_1000_cnt > 0)
  {
    result.PM_1000 /= PM_1000_cnt;
    ThingSpeak.setField(3, result.PM_1000);
  }
  if (PM_2500_cnt > 0)
  {
    result.PM_2500 /= PM_2500_cnt;
    ThingSpeak.setField(4, result.PM_2500);
  }
  if (PM_10000_cnt > 0)
  {
    result.PM_10000 /= PM_10000_cnt;
    ThingSpeak.setField(5, result.PM_10000);
  }
  Serial.println("Temp: " + String(result.Temp));
  Serial.println("RH: " + String(result.RH));
  Serial.println("SFR: " + String(result.SFR));
  Serial.println("PM_1000: " + String(result.PM_1000));
  Serial.println("PM_2500: " + String(result.PM_2500));
  Serial.println("PM_10000: " + String(result.PM_10000));
  int opc = ThingSpeak.writeFields(myChannelNumber_opc, myWriteAPIKey_opc);
  if(opc == 200) //Serial.println("Channel update successful.");
    debugFunction("OPC Channel update successful.");
  else
    debugFunction("Problem updating OPC channel. HTTP error code " + String(opc));
}

void loop()                    // Function that is looped forever
{
  dnsServer.processNextRequest();
  // String ssid_WIFI = readFile(SPIFFS, "/ssid.txt");
  // String pass_WIFI = readFile(SPIFFS, "/pass.txt");
  String ssid_WIFI = "Indiana";
  String pass_WIFI = "mitko123";
  debugFunction(ssid_WIFI);
  debugFunction(pass_WIFI);
  delay_ = readFile(SPIFFS, "/delay.txt").toInt();
  debug1 = readFile(SPIFFS, "/debugButton.txt");
  Serial.println(debug1);
  digitalWrite(opc_pin, LOW);
  debugFunction("LOW");
  opc_data = "";
  if(WiFi.status() == WL_CONNECTED)
  {  
    C02_upload();
    delay(100);
    bme_upload();
    update_client();
    if(millis() - time_trigger > 120000)
    {
      time_trigger = millis();
      digitalWrite(opc_pin, HIGH);
      debugFunction("HIGH");
      delay(8020);
      opc_data = opc_serial.readString();
      if (opc_data != "")
      {
        Serial.println(opc_data);
        std::string aloda = std::string(opc_data.c_str());
        odd2(aloda);
      }
      else
        Serial.println("No OPC data");
      std::string aloda = std::string(opc_data.c_str());
      odd2(aloda);
    }
    debugFunction(output);
  }
  else
  { 
    Serial.println(ssid_WIFI);
    Serial.println(pass_WIFI);
    WiFi.begin(ssid_WIFI.c_str(), pass_WIFI.c_str());
    delay(10000);
    while(WiFi.status() != WL_CONNECTED)
    {
      Serial.println("while");
      time_trigger = millis();
      while(millis() - time_trigger < 60000)
        dnsServer.processNextRequest();
      // String ssid_WIFI = readFile(SPIFFS, "/ssid.txt");
      // String pass_WIFI = readFile(SPIFFS, "/pass.txt");
      String ssid_WIFI = "Indiana";
      String pass_WIFI = "mitko123";
      Serial.println(ssid_WIFI);
      Serial.println(pass_WIFI);
      ESP.restart();
    }
    debugFunction("Connected to Wi-Fi");
    debugFunction(String(WiFi.localIP()));// Print ESP Local IP Address
    debugFunction(String(WiFi.softAPIP()));
  }
  String username = readFile(SPIFFS, "/username.txt");
  String password = readFile(SPIFFS, "/password.txt");
  if(username == "" || password == "")
  {
    writeFile(SPIFFS, "/username.txt", "admin");
    writeFile(SPIFFS, "/password.txt", "pass");
    debugFunction("admineeeee");
  }
  debugFunction("\n" + username + "\n" + password);
  delay(delay_);
}


// 12,1,1,0,0,0,0,0,0,0,
// 0,0,0,0,0,0,0,0,0,0,
// 0,0,0,0,9.33,0.00,0.00,0.00,0.640,
// 4.720,27.2,25.4,0.228,0.263,0.263,1,1,34,1,618,42553


// 0,0,0,0,0,0,0,0,0,0,
// 0,0,0,0,0,0,0,0,0,0,
// 0,0,0,0,0.00,0.00,0.00,0.00,0.640, //29

// 30      31  32    33    34    35    36 37 38 39 40  41
// 3.700,26.7,28.3,0.000,0.000,0.000,  0,246,25,0,660,53431
