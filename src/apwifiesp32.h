#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

String ssidYpass;
bool isConnected = false;

WebServer server(80);

void writeStringEE(String cadena, int direccion){
  int longitudCadena = cadena.length();
  for (int i = 0; i < longitudCadena; i++){
    EEPROM.write(direccion + i, cadena[i]);
  }
  EEPROM.write(direccion + longitudCadena, '\0'); // Null-terminated string
  EEPROM.commit(); // Guardamos los cambios en la memoria EEPROM
}

String readSTringEE(int direccion) {
  String cadena = "";
  char caracter = EEPROM.read(direccion);
  int i = 0;
  while (caracter != '\0' && i < 100) {
    cadena += caracter;
    i++;
    caracter = EEPROM.read(direccion + i);
  }
  return cadena;
}

void handleRoot() {
  String html = "<html><body>";
  html += "<form method='POST' action='/wifi'>";
  html += "Red Wi-Fi: <input type='text' name='ssid'><br>";
  html += "Contraseña: <input type='password' name='password'><br>";
  html += "<input type='submit' value='Conectar'>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleWifi() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  Serial.print("Conectando a la red Wi-Fi ");
  Serial.println(ssid);
  Serial.print("Clave Wi-Fi ");
  Serial.println(password);
  Serial.print("...");

  WiFi.disconnect(); // Desconectar la red Wi-Fi anterior, si se estaba conectado
  WiFi.begin(ssid.c_str(), password.c_str(),6);
  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED and cnt < 8) {
    delay(1000);
    Serial.print(".");
    Serial.print(cnt);
    cnt++;
  }
  
if (WiFi.status() == WL_CONNECTED){
  Serial.println("Conexión establecida");
  server.send(200, "text/plain", "Conexión establecida");
  //ssidYpass = ssid + ";" + password + ";";
  isConnected = true;
  EEPROM.begin(512);
  writeStringEE(ssid,100);
  writeStringEE(password,200);
  Serial.println("Escrito en 100 " + readSTringEE(100));
  Serial.println("Escrito en 200 " + readSTringEE(200));

  }
else{
  Serial.println("Conexión no establecida");
  server.send(200, "text/plain", "Conexión no establecida");
  }
}

void initAP(const char* apSsid,const char* apPassword) { // Nombre de la red Wi-Fi y  Contraseña creada por el ESP32
  //Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid, apPassword);

  server.on("/", handleRoot);
  server.on("/wifi", handleWifi);

  server.begin();
  Serial.print("Ip de esp32...");
  Serial.println(WiFi.softAPIP());
  Serial.println("Servidor web iniciado");  

}

void loopAP() {

  server.handleClient();

}
