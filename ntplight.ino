#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <MD5Builder.h>
//------------------------------------------
MD5Builder _md5;
String htime;
String md5(String str) {
  _md5.begin();
  _md5.add(String(str));
  _md5.calculate();
  return _md5.toString();
}
//------------------------------------------
WiFiServer server(80);

String header;

String gpio2State = "off";
String gpio16State = "off";

const int gpio2 = 2;
const int gpio16 = 16;

unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;
//------------------------------------------
int h,m,s;
ESP8266WiFiMulti wifiMulti;      // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
String apm;
WiFiUDP UDP;                     // Create an instance of the WiFiUDP class to send and receive

IPAddress timeServerIP;          // time.nist.gov NTP server address
const char* NTPServerName = "in.pool.ntp.org";

const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message

byte NTPBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

void setup() {
  Serial.begin(115200);          // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("\r\n");
  
  pinMode(gpio2, OUTPUT);
  pinMode(gpio16, OUTPUT);
  digitalWrite(gpio2, 1);
  digitalWrite(gpio16, 1);

  startWiFi();                   // Try to connect to some given access points. Then wait for a connection

  startUDP();

  if(!WiFi.hostByName(NTPServerName, timeServerIP)) { // Get the IP address of the NTP server
    Serial.println("DNS lookup failed. Rebooting.");
    Serial.flush();
    ESP.reset();
  }
  Serial.print("Time server IP:\t");
  Serial.println(timeServerIP);
  
  Serial.println("\r\nSending NTP request ...");
  sendNTPpacket(timeServerIP);  
  server.begin();
}
//-----------setup

unsigned long intervalNTP = 86400000; // Request NTP time every minute
unsigned long prevNTP = 0;
unsigned long lastNTPResponse = millis();
uint32_t timeUNIX = 0;

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - prevNTP > intervalNTP) { // If a minute has passed since last NTP request
    prevNTP = currentMillis;
    Serial.println("\r\nSending NTP request ...");
    sendNTPpacket(timeServerIP);               // Send an NTP request
  }

  uint32_t time = getTime();                   // Check if an NTP response has arrived and get the (UNIX) time
  if (time) {                                  // If a new timestamp has been received
    timeUNIX = time;
    Serial.print("NTP response:\t");
    Serial.println(timeUNIX);
    lastNTPResponse = currentMillis;
  } else if ((currentMillis - lastNTPResponse) > 3600000) {
    Serial.println("More than 1 hour since last NTP response. Rebooting.");
    Serial.flush();
    ESP.reset();
  }
  
  uint32_t actualTime = timeUNIX + (currentMillis - lastNTPResponse)/1000;
  h = (getHours(actualTime)+5)%24;
  m = (getMinutes(actualTime)+30);
  if(m>=60){
    h+=m/60;
    h=h%24;
    m=m%60;
    }
  //s = getSeconds(actualTime);
  //if(h>12){
  //  apm="PM";
  //  h=h%12;
  //  }
  //else apm = "AM";
  //htime = String(h)+":"+String(m)+":"+String(s)+apm;
  htime = String(h)+":"+String(m);  
  //Serial.println(htime);
  
  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
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
            if (header.indexOf("GET /"+md5("2"+htime+"on")) >= 0) {
              Serial.println("GPIO 2 on");
              gpio2State = "on";
              digitalWrite(gpio2, 0);
            } else if (header.indexOf("GET /"+md5("2"+htime+"off")) >= 0) {
              Serial.println("GPIO 2 off");
              gpio2State = "off";
              digitalWrite(gpio2, 1);
            } else if (header.indexOf("GET /"+md5("16"+htime+"on")) >= 0) {
              Serial.println("GPIO 16 on");
              gpio16State = "on";
              digitalWrite(gpio16, 0);
            } else if (header.indexOf("GET /"+md5("16"+htime+"off")) >= 0) {
              Serial.println("GPIO 16 off");
              gpio16State = "off";
              digitalWrite(gpio16, 1);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP8266 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 5  
            client.println("<p>GPIO 2 - State " + gpio2State + "</p>");
            // If the gpio2State is off, it displays the ON button
            if (gpio2State=="off") {
              client.println("<p><a href=\"/"+md5("2"+htime+"on")+"\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/"+md5("2"+htime+"off")+"\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 4  
            client.println("<p>GPIO 16 - State " + gpio16State + "</p>");
            // If the gpio16State is off, it displays the ON button       
            if (gpio16State=="off") {
              client.println("<p><a href=\"/"+md5("16"+htime+"on")+"\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/"+md5("16"+htime+"off")+"\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
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
  /*uint32_t actualTime = timeUNIX + (currentMillis - lastNTPResponse)/1000;
  h = (getHours(actualTime)+5)%24;
  m = (getMinutes(actualTime)+30);
  if(m>=60){
	  h+=m/60;
	  h=h%24;
	  m=m%60;
	  }
  s = getSeconds(actualTime);
  if(h>12){
	  apm="PM";
	  h=h%12;
	  }
  else apm = "AM";
  htime = String(h)+":"+String(m)+":"+String(s)+apm;
  Serial.println(htime);
  Serial.println(md5(htime)); 
  */
}
//----------loop


void startWiFi() { // Try to connect to some given access points. Then wait for a connection
  wifiMulti.addAP("ECE_Block", "");   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("M30S", "encyclopaedia");
  wifiMulti.addAP("ACT_2_4G", "encyclopaedia");

  Serial.println("Connecting");
  while (wifiMulti.run() != WL_CONNECTED) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println("\r\n");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());             // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  Serial.println("\r\n");
}



void startUDP() {
  Serial.println("Starting UDP");
  UDP.begin(123);                          // Start listening for UDP messages on port 123
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());
  Serial.println();
}



uint32_t getTime() {
  if (UDP.parsePacket() == 0) { // If there's no response (yet)
    return 0;
  }
  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears;
  return UNIXTime;
}



void sendNTPpacket(IPAddress& address) {
  memset(NTPBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011;   // LI, Version, Mode
  // send a packet requesting a timestamp:
  UDP.beginPacket(address, 123); // NTP requests are to port 123
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}



inline int getSeconds(uint32_t UNIXTime) {
  return UNIXTime % 60;
}



inline int getMinutes(uint32_t UNIXTime) {
  return UNIXTime / 60 % 60;
}



inline int getHours(uint32_t UNIXTime) {
  return UNIXTime / 3600 % 24;
}
