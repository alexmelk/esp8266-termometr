//Подключение
//nRF24L01 - ESP8266 12
//1 GRND
//2 VCC
//3 CE   - GPIO4
//4 CSN  - GPIO15
//5 SCK  - GPIO14
//6 MOSI - GPIO13
//7 MISO - GPIO12
//8 IRQ  - NC

//not this dev
//file wifiConf.conf
//wifiSSID
//wifiPass
//file Device.conf
//ID
//TYPE
//SERVER
//in this dev
//devices.conf
//ID
//TYPE
//IP
//VALUE
 
//for temp
#include <OneWire.h>
#include <DallasTemperature.h>
//for wifi
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>


//////////////////////////////////////////////

const char* www_username = "admin";
const char* www_password = "admin";
const char* ssidInit	 = "Smart Home Module (temperture)";
const char *passwordInit = "123123123";
const int tempPin        = 4;
unsigned int udpPort     = 6510;
const int httpPort       = 80;
const int jsonLength	 = 2048;

DynamicJsonDocument json(jsonLength);
OneWire oneWire(tempPin);
DallasTemperature sensors(&oneWire);
ESP8266WebServer server(httpPort);
WiFiUDP udp;

void setup(void) {

	ArduinoOTA.setHostname("Smart Home Module (temperture)");
	ArduinoOTA.begin();

	SPIFFS.begin();
  
	Serial.begin(115200);
 
    sensors.setResolution(11);
    sensors.begin();
  
	pinMode(LED_BUILTIN, OUTPUT);
	//restartInitial();
	//tryToErace();

	wifiInit();

	//if (MDNS.begin("smart-home", WiFi.localIP()))
	//{
	//	Serial.println("mdns Started!");
	//}
	udp.begin(udpPort);

	if (!openFile("devices.conf")) { createOrErase("devices.conf", ""); Serial.println("devices.conf"); };

	Serial.println("starting:");


	//MDNS.addService("http", "tcp", 80);

}


void loop(void) {

	String str = tryToReceive();
	if (str.length() != 0)
	{
		Serial.printf("Recived packet!\n");
		Serial.println(str);

		if (str == "initial")
		{
			Serial.println("tryToSend to Mobile");
			tryToSend(udp.remoteIP(), udpPort, WiFi.localIP()? WiFi.localIP().toString(): WiFi.softAPIP().toString());

			blink(4, 100);
		}
		else
		{
			blink(1, 100);
		}
	}
	server.handleClient();//ждём клиентов
	ArduinoOTA.handle();  //ждём обнов
	//MDNS.update();
}
//работа с устройствами
void wifiInit()
{
	while (WiFi.status() == WL_CONNECTED)
	{
		WiFi.disconnect();
		WiFi.softAPdisconnect();
		Serial.println("try to disconnect");
		delay(500);
	}

	File f = openFile("wifiConf.conf");
	if (!f)
	{
		WiFi.softAP(ssidInit, passwordInit);
		IPAddress myIP = WiFi.softAPIP();
		Serial.println("AP IP address: ");
		Serial.println(myIP);
		server.on("/", htmlAccessPoint);
		server.on("/getWifiList", sendWifiList);
		server.on("/configure", configWiFi);

		blink(2, 100);
	}
	else
	{
		json.clear();

		Serial.println("Client starting...");

		String str = f.readString();
		Serial.println(str);

		deserializeJson(json, str);
		String wifiSSID = json["wifiSSID"];
		String wifiPass = json["wifiPass"];

		WiFi.mode(WIFI_STA);
		WiFi.begin(wifiSSID, wifiPass);

		int i = 25;
		while (WiFi.status() != WL_CONNECTED) {
			delay(500);
			Serial.print(".");

			if (i == 0) {
				SPIFFS.remove("/wifiConf.conf");
				Serial.println("remove /wifiConf.conf");
				ESP.restart(); 
				delay(1000);
			}
			else { i--; }
		}
		Serial.println(WiFi.localIP());
		blink(5, 100);
	}

	server.onNotFound(handleNotFound);          //  Cтраница ошибки
	server.on("/", html);
	server.on("/clearAll", clearAll);
	server.on("/info", api);
	server.on("/getTemperature",getTemperature);
	filesHandling();
	server.begin();
}

//события http-сервера
void htmlAccessPoint()
{
	File f = SPIFFS.open("/htmlAccessPoint.html", "r");
	if (!f) {
		Serial.println("file open failed");
	}
	else {
		Serial.println("send page ok! ");
		Serial.print(server.uri());
		Serial.print(" ");
		Serial.println(f.size());
	}
	server.streamFile(f,"text/html");
	f.close();
}
void handleNotFound(){ //страница ошибок
 
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (int i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
 
}
void api(){         
    if(!server.authenticate(www_username, www_password))
      return server.requestAuthentication();   
String api = "ESP8266 + NRF2L01  \n\n";
api += "WiFi RSSI \n";
api += WiFi.RSSI();
api += "\n";
api += "Ram \n";
api += ESP.getFreeHeap();
api += "\n";
api += "Chip ID\n ";
api += ESP.getChipId();
api += "\n";
api += "Sent requests:\n";
server.send(200, "text/plain", api);
};
void html() {
	//if (!server.authenticate(www_username, www_password))
	//	return server.requestAuthentication();
		File f = SPIFFS.open("/index.html", "r");
		if (!f) {
			Serial.println("file open failed");
		}
		else {

			Serial.println("send page ok! ");
			Serial.print(server.uri());
			Serial.print(" ");
			Serial.println(f.size());
		}
		server.streamFile(f,"text/html");
		f.close();

}
void clearAll()
{
	SPIFFS.remove("/wifiConf.conf");
	SPIFFS.remove("/device.conf");
	SPIFFS.remove("/devices.conf");

	server.send(200, "text/html", "clear Ok");
	Serial.println("clear Ok");

	WiFi.disconnect();
	delay(1000);
	ESP.restart();
}
void sendWifiList()
{
	String http;
	int n = WiFi.scanNetworks();
	if (n == 0)
	{
		http += "<option value='-1'>No Networks!</option>";
	}
	else
	{
		Serial.println("FindWifi:");
		http += "<option selected>Choose...</option>";
		for (int i = 0; i < n; ++i)
		{
			http += "<option value='";
			http += WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
			Serial.println(WiFi.SSID(i));
			delay(1);

		}
	}
	server.send(200, "text/plain", http);
}

//общие функции
String tryToReceive()
{
	int udpPacketSize = udp.parsePacket();
	String str;
	if (udpPacketSize)
	{
		str = udp.readString();
	}
	return str;
}
void tryToSend(IPAddress remoteIp, int udpPort, String text)
{
	udp.beginPacket(remoteIp, udpPort);
	char buf[256];
	text.toCharArray(buf, 256, 0);
	udp.write(buf);
	udp.endPacket();

}
void configWiFi()
{
	String str;
	String wifiSSID = server.arg("SSID");
	String wifiPass = server.arg("pass");
	json["wifiSSID"] = wifiSSID;
	json["wifiPass"] = wifiPass;
	serializeJson(json, str);
	createOrErase("wifiConf.conf", str);
	Serial.println("New wifiConf!");
	Serial.println(str);

	Serial.println("restart");

	WiFi.disconnect();
	WiFi.softAPdisconnect();
	delay(2000);

	ESP.restart();
}
void filesHandling()
{
	Dir dir = SPIFFS.openDir("/");
	Serial.println("Add file to listeaner");
	while (dir.next()) {
		server.on(dir.fileName(), fileDownload);
		Serial.println(dir.fileName());
	}
}
void fileDownload()
{
	File f = SPIFFS.open(server.uri(), "r");
	if (!f) {
		Serial.println("file open failed");
		Serial.println(server.uri());
	}
	else { 
		Serial.println("send page ok! "); 
		Serial.print(server.uri());
		Serial.print(" ");
		Serial.println(f.size());
	}
	server.streamFile(f, getContentType(server.uri()));
	f.close();
	Serial.print("Send file: ");
	Serial.println(server.uri());
}
String getContentType(String filename) {
	if (server.hasArg("download")) return "application/octet-stream";
	else if (filename.endsWith(".htm")) return "text/html";
	else if (filename.endsWith(".html")) return "text/html";
	else if (filename.endsWith(".css")) return "text/css";
	else if (filename.endsWith(".js")) return "application/javascript";
	else if (filename.endsWith(".png")) return "image/png";
	else if (filename.endsWith(".gif")) return "image/gif";
	else if (filename.endsWith(".jpg")) return "image/jpeg";
	else if (filename.endsWith(".ico")) return "image/x-icon";
	else if (filename.endsWith(".xml")) return "text/xml";
	else if (filename.endsWith(".pdf")) return "application/x-pdf";
	else if (filename.endsWith(".zip")) return "application/x-zip";
	else if (filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}
File openFile(String Filename)
{
	File f = SPIFFS.open("/" + Filename, "r");
	if (!f) {
		Serial.println("file open failed");
		Serial.println(Filename);
	}
	return f;
}
void createOrErase(String Filename, String Text)
{
	File f = SPIFFS.open("/" + Filename, "w");
	if (!f) {
		Serial.println("file open failed");
		Serial.println(Filename);
	}
	f.print(Text);
	f.close();
}
void getTemperature() {
	sensors.requestTemperatures();
	String temp = String(sensors.getTempCByIndex(0));
	Serial.println(temp);
	server.send(200, "text/html", temp);
	blink(3, 100);
}
void blink(int num, int delayMs)
{
	for (int i = 0; i < num; i++)
	{
		digitalWrite(LED_BUILTIN, LOW);
		delay(delayMs);
		digitalWrite(LED_BUILTIN, HIGH);
		delay(delayMs);
	}

}
 
