#include <DHT.h>
#include <SoftwareSerial.h>
#include <Wire.h>            
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <WiFi.h>

#define DHTPIN A1
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial mySerial(4, 3);
LiquidCrystal_I2C lcd(0x27,20,4);
WiFiServer server(80);  // 80포트를 사용하는 웹서버 선언

char ssid[] = "KT_GiGA_2G_TEST";      //  연결하실 와이파이 SSID
char pass[] = "micheal07!";   // 네트워크 보안키
int status = WL_IDLE_STATUS;

void setup()
{
  Serial.begin(9600);
  mySerial.begin(9600);
  lcd.init();
  lcd.backlight();
  if (WiFi.status() == WL_NO_SHIELD) // 현재 아두이노에 연결된 실드를 확인
  { 
    Serial.println("WiFi shield not present"); 
    while (true);  // 와이파이 실드가 아닐 경우 계속 대기
  } 

  // 와이파이에 연결 시도
  while (status != WL_CONNECTED) //연결될 때까지 반복
  { 
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);  // WPA/WPA2 연결
  } 
  server.begin();
  printWifiStatus();  // 연결 성공시 연결된 네트워크 정보를 출력
}

// 연결된 네트워크 정보 출력
void printWifiStatus()  
{ 
  // 네트워크 SSID 출력
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // 네트워크 ip 출력
  IPAddress ip = WiFi.localIP(); 
  Serial.print("IP Address: ");
  Serial.println(ip);
  
  // 수신 강도 출력
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void loop()
{
  int h = dht.readHumidity();
  int t = dht.readTemperature();
  
  int vout = analogRead(2); //ADC값
  float volt = 5.0 / 1023.0 * vout; //ADC to V
  double toluene = pow(10, ((-2.071) + 0.672 * (volt)));
  double formaldehyde = pow(10, ((-0.867) + 1.274 * (volt)));

  int chksum = 0, res = 0;
  unsigned char pms[32] = {0, };
  char buf[10];
  int PM25 = 0;
  
  if(mySerial.available() >= 32)
  {
    chksum = 0;
    pms[0] = mySerial.read();
    if(pms[0] != 0x42) return;
    chksum += pms[0];

    pms[1] = mySerial.read();
    if(pms[1] != 0x4d) return;
    chksum += pms[1];

    for(int j = 2; j < 32; j++)
    {
      pms[j] = mySerial.read();
      if(j < 30)
      {
        chksum += pms[j];
      }
    }
    if(pms[30] != (unsigned char)(chksum >> 8) || pms[31] != (unsigned char)(chksum))
    {
      Serial.println("*---------------------------------------*");
      Serial.println("*****checksum error*****");
      return res;
    }
    if(pms[0] != 0x42 || pms[1] != 0x4d)
    {
      Serial.println("*---------------------------------------*");
      Serial.println("*****start value error*****");
      return res;
    }
    PM25 = (pms[12] << 8) | pms[13];
    
    Serial.println("*---------------------------------------*");
    Serial.print("TEMP: ");       
    Serial.print(t);  // 온도 값 출력
    Serial.print("ºC");
    Serial.print("  ");
    Serial.print("HUM: ");     
    Serial.print(h);  // 습도 값 출력
    Serial.println("%");

    Serial.print("PM2.5: ");
    Serial.print(PM25); //초미세먼지 값 출력
    Serial.println("ug/m^3");
        
    Serial.print("TOLU: ");
    Serial.print(toluene);  //톨루엔 값 출력
    Serial.println("ppm");
    Serial.print("FORMAL: ");
    Serial.print(formaldehyde); //포름알데히드 값 출력
    Serial.println("ppm");
  }
  
  lcd.setCursor(0,0);
  lcd.print("TEMP: ");
  lcd.print(t); //온도 값 LCD로 출력
  lcd.print((char)223);
  lcd.print("C ");
  lcd.setCursor(11,0); //LCD 뛰어쓰기
  lcd.print("HUM: ");
  lcd.print(h); //습도 값 LCD에 출력
  lcd.print("% ");
  
  lcd.setCursor(0,1);
  lcd.print("PM2.5: ");
  lcd.print(PM25); // 초미세먼지 값 LCD로 출력
  lcd.print("ug/m^3 ");

  lcd.setCursor(0,2);
  lcd.print("TOLU: ");
  lcd.print(toluene); // 톨루엔 값 LCD로 출력
  lcd.print("ppm ");
  lcd.setCursor(0,3);
  lcd.print("FORMAL: ");
  lcd.print(formaldehyde); // 포름알데히드 값 LCD로 출력
  lcd.print("ppm ");
  delay(2000);

  WiFiClient client = server.available();  // 들어오는 클라이언트를 수신한다.
  if (client) // 클라이언트를 수신 시
  {
    //Serial.println("new client");  // 클라이언트 접속 확인 메시지 출력
    boolean currentLineIsBlank = true;

    while (client.connected ()) 
    { 
      if (client.available()) 
      {
        char c = client.read();
        
        // 문자의 끝을 입력 받으면 http 요청이 종료되고, 답신을 보낼 수 있습니다.
        if (c == '\n' && currentLineIsBlank) 
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println("Refresh: 2"); // 1초당 페이지 refresh
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<meta charset=utf-8/>");
          client.print("<meta name=view content=width=device-width, ");
          client.println("initial-scale=1.0, maximum-scale=1.0, minimum-scale=1.0, user-scalable=no />");
          client.println("<html>");
          client.println("<head>");  
          client.println("<title>Sensor</title>");
          client.println("</head>");

          client.println("<center>");
          client.println("<h1>Sensor</h1>");
          client.println("<div data-role=content>");
          client.println("<br>");

          //온습도
          client.print("TEMP: ");
          client.print(t);  // 온도 값 출력
          client.println("&deg;C");
          client.println("<br>");
          client.print("HUM: ");
          client.print(h);  // 습도 값 출력
          client.println("%");
          client.println("<br>");

          //미세먼지
          client.print("PM2.5: ");
          client.print(PM25);  // 초미세먼지 값 출력
          client.println("ug/m^3");
          client.println("<br>");

          //톨루엔, 포름알데히드
          client.print("TOLU: ");
          client.print(toluene);  // 톨루엔 값 출력
          client.println("ppm");
          client.println("<br>");
          client.print("FORMAL: ");
          client.print(formaldehyde);  // 포름알데히드 값 출력
          client.println("ppm");
          client.println("<br>");

          client.println("</center>");
          client.println("</div>");
          client.println("</body>");
          client.println("</html>");
          break;
        }
        
        if (c == '\n') 
        { 
          currentLineIsBlank = true;
        }
        else if (c != '\r') 
        {
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
    //Serial.println("client disonnected");
  }
}
