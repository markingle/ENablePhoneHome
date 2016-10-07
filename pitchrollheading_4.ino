// pitch-roll-yaw sensor
// complete program from Adafruit sent sensor values to USB port
// added one line to code (provided also by Adafruit) to run on ESP8255
// now I am adding code to send data to the e-NABLE cloud
// by Les Hall
// e-NABLE cloud functionality started Thu Sep 29 2016, 5:21 AM USA Central Time
//



// incude library files
// 
// first the Adafruit libraries from the example
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_L3GD20_U.h>
#include <Adafruit_9DOF.h>
// 
// second the ESP8266 WiFi libraries
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMesh.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h> 
#include <WiFiServer.h>
#include <string.h>



/* Assign a unique ID to the sensors */
Adafruit_9DOF                dof   = Adafruit_9DOF();
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(30301);
Adafruit_LSM303_Mag_Unified   mag   = Adafruit_LSM303_Mag_Unified(30302);

// my added global variables - Les
// 
// declare heading report variables
float threshold = 10.0;  // change in heading threshold
float averageHeading = 0;  // the long term running average of the heading
float tau = 0.99;  // time constant of averaging, 0.0<tau<1.0, larger => slower change
// 
// declare hard-wired server and WiFi values
const char* host = "webapp.e-nable.me";
String userSSID = "Guest";
String userPassword = "Health101";



/* Update this with the correct SLP for accurate altitude measurements */
float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;

/**************************************************************************/
/*!
    @brief  Initialises all the sensors used by this example
*/
/**************************************************************************/
void initSensors()
{
  if(!accel.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println(F("Ooops, no LSM303 detected ... Check your wiring!"));
    while(1);
  }
  if(!mag.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Adafruit 9 DOF Pitch/Roll/Heading Example")); Serial.println("");

  // specify the I2C pins
  Wire.begin(4,5); //4 is SDA, 5 is SCL

  /* Initialise the sensors */
  initSensors();
}

/**************************************************************************/
/*!
    @brief  Constantly check the roll/pitch/heading/altitude/temperature
*/
/**************************************************************************/
void loop(void)
{
  sensors_event_t accel_event;
  sensors_event_t mag_event;
  sensors_vec_t   orientation;

  /* Calculate pitch and roll from the raw accelerometer data */
  accel.getEvent(&accel_event);
  if (dof.accelGetOrientation(&accel_event, &orientation))
  {
    /* 'orientation' should have valid .roll and .pitch fields */
    Serial.print(F("Roll: "));
    Serial.print(orientation.roll);
    Serial.print(F("; "));
    Serial.print(F("Pitch: "));
    Serial.print(orientation.pitch);
    Serial.print(F("; "));
  }
  
  /* Calculate the heading using the magnetometer */
  mag.getEvent(&mag_event);
  if (dof.magGetOrientation(SENSOR_AXIS_Z, &mag_event, &orientation))
  {
    /* 'orientation' should have valid .heading data now */
    Serial.print(F("Heading: "));
    Serial.print(orientation.heading);
    Serial.println(F("; "));
  }

  // decide if heading change is large enough to report
  if ( abs(float(orientation.heading) - averageHeading) > threshold )
  {
    // snap change the average heading to the latest reported heading
    averageHeading = float(orientation.heading);

    // report the new heading value to the e-NABLE cloud
    phone_home(averageHeading);
  }
  else 
  {
    // update average heading according to exponential change with tau time constant
    averageHeading = tau*averageHeading + (1.0-tau)*orientation.heading;
  }
  Serial.println("averageHeading " + String(averageHeading) );
  
  Serial.println(F(""));
  delay(1000);
}



// make the call
void phone_home(float heading) {

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
  
  // We now create a URI for the request
  String chipID = String(ESP.getChipId());
  String headingStr = String(heading);
  String url = "http://e-nable.me/dev/service?type=cilog&topic=device/" + chipID + "&mssg=heading_" + headingStr;
  //http://webapp.e-nable.me/dev/service?type=cilog&topic=device/A904012&mssg=my LONG Mssg
  Serial.print("Requesting URL #" + String(headingStr) + ": ");
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
}



