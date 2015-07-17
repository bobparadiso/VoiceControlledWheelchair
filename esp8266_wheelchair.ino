#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#define MDNS_NAME "esp8266_a"
#define AP_SSID "SSID"
#define AP_PASSWORD "PASSWORD"

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

// multicast DNS responder
MDNSResponder mdns;

#define MODE_NONE 0
#define MODE_DRIVE 1
#define MODE_ARM 2
#define MODE_POWER 3

uint8_t controlMode = MODE_NONE;

#define LED_PIN 0

//
void setControlMode(uint8_t val)
{
  if (controlMode != val)
  {
    Serial.print((char)('0' + val));
    controlMode = val;
  }
}

//
void setup()
{
  Serial.begin(115200);

  // prepare GPIO2
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);

  // Connect to WiFi network
  WiFi.begin(AP_SSID, AP_PASSWORD);
  //Serial.println("");  
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }
  /*
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(AP_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  */
  
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  //   Note: for AP mode we would use WiFi.softAPIP()!
  if (!mdns.begin(MDNS_NAME, WiFi.localIP())) {
    //Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  //Serial.print("mDNS responder started as: ");
  //Serial.println(MDNS_NAME);
  
  // Start TCP (HTTP) server
  server.begin();
  //Serial.println("TCP server started");
}

//
uint32_t lastTx = 0;
uint8_t controlData = '.';
uint32_t brakeTime = 0;
#define TX_INTERVAL 250
void refreshControlData()
{
  if (controlMode != MODE_DRIVE)
    return;
    
  if (brakeTime && millis() > brakeTime)
  {
    controlData = '.';
    brakeTime = 0;
  }
  
  if (millis() - lastTx > TX_INTERVAL)
  {
    Serial.print((char)controlData);
    lastTx = millis();
  }
}

//
void loop()
{
  refreshControlData();
  
  // Check for any mDNS queries and send responses
  mdns.update();

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  //Serial.println("new client");
  while(!client.available()){
    refreshControlData();
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  //Serial.println("req begin:");
  //Serial.println(req);
  //Serial.println("req end");
  client.flush();
  
  // Match the request
  int val;
  int commandDurationPos = req.indexOf(" HTTP/") - 1;
  int commandDuration = 0;
  if (commandDurationPos != -1)
    commandDuration = req.charAt(commandDurationPos) - '0';
  //Serial.println(commandDuration);

  if (req.indexOf("/wc/rt") != -1)
  {
    val = 0;
    controlData = 'r';
    brakeTime = max(millis() + commandDuration * 1000, 1);
    setControlMode(MODE_DRIVE);
  }
  else if (req.indexOf("/wc/lt") != -1)
  {
    val = 0;
    controlData = 'l';
    brakeTime = max(millis() + commandDuration * 1000, 1);
    setControlMode(MODE_DRIVE);
  }
  else if (req.indexOf("/wc/fwd") != -1)
  {
    val = 0;
    controlData = 'u';
    brakeTime = max(millis() + commandDuration * 1000, 1);
    setControlMode(MODE_DRIVE);
  }
  else if (req.indexOf("/wc/rev") != -1)
  {
    val = 0;
    controlData = 'd';
    brakeTime = max(millis() + commandDuration * 1000, 1);
    setControlMode(MODE_DRIVE);
  }
  else if (req.indexOf("/wc/brake") != -1)
  {
    val = 1;
    controlData = '.';
    setControlMode(MODE_DRIVE);
  }
  else if (req.indexOf("/fan/on") != -1)
  {
    controlData = '.';
    setControlMode(MODE_POWER);
    Serial.print('x');
  }
  else if (req.indexOf("/fan/off") != -1)
  {
    controlData = '.';
    setControlMode(MODE_POWER);
    Serial.print('x');
  }
  else
  {
    //Serial.println("invalid request");
    client.stop();
    return;
  }

  // Set GPIO2 according to the request
  digitalWrite(LED_PIN, val);
  
  client.flush();

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nREQ was:";
  s += req;
  s += "</html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  //Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}


