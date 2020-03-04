// Load Wi-Fi library
#include <WiFi.h>
#include <ESP32Servo.h>

// Replace with your network credentials
const char* ssid     = "TrackPod";
const char* password = "group40";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliary variables to store the current output state
String output18State = "off";
String output19State = "off";

// Assign output variables to GPIO pins
const int output18 = 18;
const int output19 = 19;

unsigned long last_centered_time;

Servo pan, tilt;
int pos_tilt = 35;  
int pos_pan = 90;  
int servoPan = 17;
int servoTilt = 27;

int centered = 0; 

void setup() {
  Serial.begin(115200);
  pan.setPeriodHertz(50);    // standard 50 hz servo
  pan.attach(servoPan, 500 , 2500);  
  tilt.setPeriodHertz(50);    // standard 50 hz servo
  tilt.attach(servoTilt, 500 , 2500); 

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  pan.write(90);
  tilt.write(30);

  server.begin();
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                  // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /box/") >= 0) {
              int data_end = header.indexOf(" ", header.indexOf("/box/"));
              int data_start = header.lastIndexOf("/", data_end) + 1;
              String coordinates = header.substring(data_start, data_end);
              int y = coordinates.substring(coordinates.indexOf("&")+1).toInt();
              int x = coordinates.substring(0, coordinates.indexOf("&")).toInt();

              bool x_centered = false;
              bool y_centered = false;

          
              //Serial.println(x);
              //Serial.println(y);

              // X adjustment
              int delta = (abs(x)/500)*12 + 3;
              if (x < -100 ) {
                pos_pan = (((pos_pan+delta) > 180)) ? 180 : (pos_pan+delta);
                pan.write(pos_pan);
                centered = 0;
              } else if (x > 100) {
                pos_pan = (((pos_pan-delta) < 0)) ? 0 : (pos_pan-delta);
                pan.write(pos_pan);
                centered = 0;
              } else {
                x_centered = true;
              }

              // Y adjustment
              delta = (abs(y)/600)*8 + 2;
              if (y < -200) {
                pos_tilt = (((pos_tilt+delta) > 60)) ? 60 : (pos_tilt+delta);
                tilt.write(pos_tilt);
                centered = 0;
              } else if (y > 200) {
                pos_tilt = (((pos_tilt-delta) < 30)) ? 30 : (pos_tilt-delta);
                tilt.write(pos_tilt);
                centered = 0;
              } else {
                y_centered = true;
              }

              if (x_centered && y_centered)
                centered++;
                
            }
            if (header.indexOf("GET /tilt/") >= 0) {
              int fin = header.indexOf(" ", header.indexOf("/tilt/"));
              int start = header.lastIndexOf("/", fin);
              Serial.println("Tilt");
              Serial.println(header.substring(start+1, fin));
              tilt.write(header.substring(start+1, fin).toInt());
            } 

            if (header.indexOf("GET /pan/") >= 0) {
              int fin = header.indexOf(" ", header.indexOf("/pan/"));
              int start = header.lastIndexOf("/", fin);
              Serial.println("Pan");
              Serial.println(header.substring(start+1, fin));
              pan.write(header.substring(start+1, fin).toInt());
            } 

            client.println((centered >= 10) ? "CENTERED" : "IN_PROGRESS");
            client.println();

            if ((centered >= 10)&& ((millis()-last_centered_time)> 5000)) {
              centered = 0;
            }

            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
