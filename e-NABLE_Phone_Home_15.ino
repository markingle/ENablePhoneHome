// e-NABLE Phone Home program for ESP8266 WiFi IoT module
// this program is pieced together from a collection of
// example programs.  The legal statement from each example
// is preserved here at the top of the program.  Enjoy!  
// 
// Les Hall - Fri Oct 16 2015
// 


/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* Create a WiFi access point and provide a web server on it. */
/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO2 low,
 *    http://server_ip/gpio/1 will set the GPIO2 high
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 */

 
/*
 *  Simple HTTP get webclient test
 */


// modified for e-NABLE Phone Home
// by Les Hall Tue Oct 13 2015
// 



// identifies the module
String ID = "1";



// specify the time in seconds between phone home attempts
#define INTERVAL_SECONDS 60
// specify the pin definitions
#define LED_PIN 0



#include <ESP8266WiFi.h>
#include <ESP8266WiFiMesh.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h> 
#include <WiFiServer.h>
#include <string.h>


// Set the SSID and passwords of the access point
// also specify the host site url 
// (the place to which we are phoning home)
const char *ssidAccessPoint = "e-NABLE";
const char *passwordAccessPoint = "password";
const char* host = "webapp.e-nable.me";
String userSSID = "";
String userPassword = "";

// multicast DNS responder
MDNSResponder mdns;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);



// declare global variables
int network = 0;
int val = 0;
int n = -1;
int selection = -1;
int passwordCredentials = 0;
int SSIDCredentials = 0;


void setup() {
  
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  //WiFi.softAP(ssidAccessPoint, passwordAccessPoint);
  WiFi.softAP(ssidAccessPoint);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  /*if (!mdns.begin("phonehome", myIP)) {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  */
 
  server.begin();
  Serial.println("HTTP server started");

  // prepare GPIO0
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, val);
}


int value = 0;


void loop() {
  if (network == 0)
    get_credentials();
  else
    phone_home();
}




void get_credentials() {

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    delay(10000);
    return;
  }
  
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  // get the selection
  if ( (req.indexOf("/SSID=") != -1) ) {//&& (SSIDCredentials == 0) ) {
    userSSID = req;
    int pos1 = userSSID.indexOf("/SSID=");
    pos1 += 6;
    int pos2 = userSSID.indexOf(" HTTP");
    userSSID = userSSID.substring(pos1, pos2);
    Serial.println("userSSID=" + userSSID);
    ++SSIDCredentials;
  }

  // recognize password
  int pos1 = req.indexOf("pwd?pwd=");
  if ( (pos1 != -1) && (passwordCredentials == 0) ) {
    userPassword = req;
    pos1 += 8;
    int pos2 = req.indexOf(" HTTP");
    userPassword = userPassword.substring(pos1, pos2);
    Serial.println(String(pos1)+","+String(pos2));
    Serial.println("userPassword=\"" + userPassword + "\"");
    ++passwordCredentials;
  }

  // set up the response string
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";

  // scan the network
  if ( (SSIDCredentials == 0) && (passwordCredentials == 0) )
    n = WiFi.scanNetworks();  
  
  if ( (SSIDCredentials > 0) && (passwordCredentials > 0) ) {
    network = 1;
    s += "<h1>Thank you for entering your network information.<br>";
    s += "Phoning home now.</h1>";
    s += "</html>\r\n";
    client.print(s);
  } else {
    // find the password and the selection
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      if (SSIDCredentials >= 1) {
        s += "<h1><form action=\"pwd\">";
        s += "Please enter your local network password:<br>";
        s += "<input type=\"password\" name=\"pwd\">";
        s += "</form></h1>";
        s += "</html>\r\n";
        client.print(s);
      } else {
        s += "<h1>Please select your network</h1><br><br>";
        for (int i = 0; i < n; ++i)
        {
          int match = 0;
          String newSSID = WiFi.SSID(i);
          for (int j = 0; j < i; ++j) {
            if (newSSID.equals(WiFi.SSID(j))) {
              match = 0;
            }
          }
    
          if (match == 0) {
            s += "<h1><a href=\"/SSID=";
            s += WiFi.SSID(i);
            s += "\">";
            s += WiFi.SSID(i);
            s += "</a> ";
            s += "</h1>";
            s += "</html>\r\n";
          }
        }
        delay(10);
        client.print(s);
      }
    }
  }
  
  client.flush();

  delay(1);
  Serial.println("Client disconnected");
}



// make the call
void phone_home() {
  
  digitalWrite(LED_PIN, LOW);  // turn LED on
  ++value;


  // connect to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(userSSID);
  
  WiFi.begin(userSSID.c_str(), userPassword.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    ++attempts;
    if (attempts > 100) {
      network = 0;
      SSIDCredentials = 0;
      passwordCredentials = 0;
      return;
    }
    delay(2000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // read the battery voltage from the analog input
  int analogInput = analogRead(A0);
  float batteryVoltage = (50.3/3.3) * 1.0 * (analogInput/1024.0);
  //                      divider     Vmax          input
  Serial.print(analogInput);
  Serial.print(" ");
  Serial.println(batteryVoltage);
  
  // We now create a URI for the request
  String chipID = String(ESP.getChipId());
  String voltage = String(batteryVoltage);
  String url = "http://e-nable.me/dev/service?type=cilog&topic=device/" + chipID + "&mssg=" + voltage;
  //http://webapp.e-nable.me/dev/service?type=cilog&topic=device/A904012&mssg=my LONG Mssg
  Serial.print("Requesting URL #" + String(value) + ": ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(100);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");

  WiFi.disconnect();

  // delay between reports
  digitalWrite(LED_PIN, HIGH);  // turn LED off
  for (int time=0; time<INTERVAL_SECONDS; ++time) {
    for (int count=0; count<1000; count += 500) {
      digitalWrite(LED_PIN, HIGH);  // turn LED off
      delay(250);
      digitalWrite(LED_PIN, LOW);  // turn LED on
      delay(250);
    }
  }
}



