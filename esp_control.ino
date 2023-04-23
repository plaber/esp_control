#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

const char* ssid = "scp-121";
const char* password = "bioshock";
String sap = "ledControl";
String ver = "0.06";

const int led = 2;
const int b1 = 12;
const int b2 = 14;
const int b3 = 13;
unsigned long t1, t2, t3;
bool k1 = true, k2 = true, k3 = true;


ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiUDP Udp;
char udpBuf[255]; 
String udpAns = "";

enum opt {
  mloop,
  mone,
};
enum opt btmode = mloop;

void udp_poll()
{
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    int len = Udp.read(udpBuf, 255);
    if (len > 0)
    {
      udpBuf[len] = 0;
      udpAns = String(udpBuf);
    }
  }
}

void handleRoot() {
  digitalWrite(led, 1);
  if (httpServer.args() > 0){
    sap = httpServer.arg(0);
    char ebuf[21];
    memset(ebuf, 0, 21);
    sap.toCharArray(ebuf, 21);
    EEPROM.put(0, ebuf);
    EEPROM.commit();
  }
  String ans = "hello from esp8266! v" + ver + "<br>\n";
  ans += "SSID: " + WiFi.SSID() + " [" + WiFi.RSSI() + "]<br>\n";
  ans += "IP: " + WiFi.localIP().toString() + "<br>\n";
  ans += "Ans: " + udpAns + "<br>\n";
  ans += "mode: " + String(btmode == mloop ? "loop" : "one") + "<br>\n";
  ans += "<form method='post'><input required name='ssid' value='" + sap + "' maxlength=15 onkeyup='this.value = this.value.replace(/[^A-Za-z0-9_]/g, '')'><input type='submit'></form><br>";
  ans += "<a href=\"/update\">update</a><br><br>\n";
  ans += "<a href=\"/reboot\">reboot</a>\n";
  if(digitalRead(b1) == LOW) ans += "b1 ";
  if(digitalRead(b2) == LOW) ans += "b2 ";
  if(digitalRead(b3) == LOW) ans += "b3 ";
  httpServer.send(200, "text/html", ans);
  digitalWrite(led, 0);
}

void handleReboot() {
  digitalWrite(led, 1);
  String ans = "Goodbye from esp8266!<br>\n";
  httpServer.send(200, "text/html", ans);
  digitalWrite(led, 0);
  ESP.restart();
}

void setup() {
  // put your setup code here, to run once:
  pinMode(led, OUTPUT);
  pinMode(b1, INPUT_PULLUP);
  pinMode(b2, INPUT_PULLUP);
  pinMode(b3, INPUT_PULLUP);
  
  Serial.begin(115200);
  Serial.println("kek");
  EEPROM.begin(21);
  char ebuf[21];
  EEPROM.get(0, ebuf);
  if (ebuf[0] != 0 && ebuf[0] != 255) sap = String(ebuf);
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(sap, "12345678");
  
  int scn = WiFi.scanNetworks();
  for (int scni = 0; scni < scn; ++scni)
  {
    if(WiFi.SSID(scni).equals(ssid))
    {
      WiFi.begin(ssid, password);
      int tr = 0;
      while (WiFi.status() != WL_CONNECTED && tr < 10) {
        Serial.println(".");
        digitalWrite(led, 1);
        delay(250);
        digitalWrite(led, 0);
        delay(250);
        tr++;
      }
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }

  httpUpdater.setup(&httpServer);
  httpServer.on("/", handleRoot);
  httpServer.on("/reboot", handleReboot);
  httpServer.onNotFound(handleRoot);
  httpServer.begin();
  Udp.begin(60201);
}

void delayu(unsigned long val)
{
  unsigned long start_time = millis();
  while(millis() - start_time < val) udp_poll();
}

void loop() {
  // put your main code here, to run repeatedly:
  httpServer.handleClient();
  udp_poll();
  if(digitalRead(b1) == LOW && k1 == true){
    k1 = false;
    t1 = millis();
    Udp.beginPacket("192.168.4.255", 8888);
    if (btmode == mloop) Udp.write("go=1"); else Udp.write("modp=p");
    Udp.endPacket();
    digitalWrite(led, 1);
    delay(50);
    digitalWrite(led, 0);
  }
  if(digitalRead(b2) == LOW && k2 == true){
    k2 = false;
    t2 = millis();
    Udp.beginPacket("192.168.4.255", 8888);
    if (btmode == mloop) Udp.write("stp=1"); else Udp.write("modp=m");
    Udp.endPacket();
    digitalWrite(led, 1);
    delay(50);
    digitalWrite(led, 0);
  }
  if(digitalRead(b3) == LOW && k3 == true){
    k3 = false;
    t3 = millis();
    Udp.beginPacket("192.168.4.255", 8888);
    if (btmode == mloop) {
        Udp.write("btmode=one");
        btmode = mone;
        Udp.endPacket();
        udp_poll();
        digitalWrite(led, 1); delayu(50); digitalWrite(led, 0); delayu(100); digitalWrite(led, 1); delayu(50); digitalWrite(led, 0);
    } else {
        Udp.write("btmode=loop");
        btmode = mloop;
        Udp.endPacket();
        udp_poll();
        digitalWrite(led, 1); delayu(50); digitalWrite(led, 0);
    }
  }
  if (millis() - t1 > 500) k1 = true;
  if (millis() - t2 > 500) k2 = true;
  if (millis() - t3 > 500) k3 = true;
}
