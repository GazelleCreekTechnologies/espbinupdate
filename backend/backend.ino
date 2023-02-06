#ifdef ESP32
  #include <ESPAsyncWebServer.h>
  #include <WiFi.h>
  #include <HTTPClient.h>
#else
  #include <Arduino.h>
  #include <ESP8266WiFi.h>
  #include <Hash.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <ESP8266HTTPClient.h>
#endif
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiClient.h>


// Data wire is connected to GPIO 4
#define ONE_WIRE_BUS 4
#define Relay 16

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// Variables to store temperature values
String temperatureF = "";
String temperatureC = "";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

// Replace with your network credentials
const char* ssid = "GazelleCreek";
const char* password = "pdjau58584";
const char* host = "https://express-vercel-lindany.vercel.app";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void httpGet() {
  Serial.println("Requesting URL: ");
  HTTPClient http;   
 // bool begin(WiFiClient &client, const String& host, uint16_t port, const String& uri = "/", bool https = false);
  WiFiClient client;
                                               //Declare an object of class HTTPClient
  http.begin("https://express-vercel-lindany.vercel.app/buckets/2");  //Specify request destination
 
 
  int httpCode = http.GET();                                           //Send the request

  if (httpCode > 0) {                   //Check the returning code
    String payload = http.getString();  //Get the request response payload
    Serial.println(payload);            //Print the response payload
  }
  delay(1000);  //Send a request every 1 seconds
}

void httpPOST(float temp) {
  HTTPClient http;
  WiFiClient client;

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(String(temp));
    const String serverName = "https://express-vercel-lindany.vercel.app/setcurrenttemp/" + String(temp) + "/2";
    
    http.begin(serverName.c_str());
    http.addHeader("Content-Type", "text/plain");

    int httpResponseCode = http.PUT("PUT sent from ESP32");

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending PUT Request: ");
      Serial.println(httpResponseCode);
    }
  } else {
    Serial.println("Error in WiFi connection");
  }
  delay(1000);  //Send a request every 1 seconds
}

String readDSTemperatureC() {
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == -127.00) {
    Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  } else {
    Serial.print("Temperature Celsius: ");
    Serial.println(tempC);
    if (tempC >= 60) {
      digitalWrite(Relay, HIGH);
    } else {
      digitalWrite(Relay, LOW);
    }
  }
  return String(tempC);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ds-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>MINI GYSER TEST Server</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature Celsius</span> 
    <span id="temperaturec">%TEMPERATUREC%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature Fahrenheit</span>
    <span id="temperaturef">%TEMPERATUREF%</span>
    <sup class="units">&deg;F</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturec").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturec", true);
  xhttp.send();
}, 10000) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturef").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturef", true);
  xhttp.send();
}, 10000) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DS18B20 values
String processor(const String& var) {
  //Serial.println(var);
  if (var == "TEMPERATUREC") {
    return temperatureC;
  } else if (var == "TEMPERATUREF") {
    return temperatureF;
  }
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println();

  pinMode(Relay, OUTPUT);

  // Start up the DS18B20 library
  sensors.begin();

  temperatureC = readDSTemperatureC();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", temperatureC.c_str());
  });
  server.on("/temperaturef", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", temperatureF.c_str());
  });
  // Start server
  server.begin();
}


void loop() {

  if ((millis() - lastTime) > timerDelay) {
    temperatureC = readDSTemperatureC();
    float currentTemp = random(1, 100);
    httpGet();
    httpPOST(currentTemp);

    // httpPOST(float(temperatureC));
    lastTime = millis();
  }
}