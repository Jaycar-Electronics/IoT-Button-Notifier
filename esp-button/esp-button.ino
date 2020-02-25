#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define BUTTON_LED D6
#define BUTTON_INPUT D5
#define LCD_ADDR 0x27
#define BLINK_DELAY 700

// --------------------- network configuration

#define NETWORK_SSID "your wifi ssid"
#define NETWORK_PW "your wifi password"

// -------------------------------------------

ESP8266WebServer server(80);

bool blink = true;
unsigned long blink_timer = 0;
unsigned int blink_delay = 1000;

String line1 = "";
String line2 = "";

bool update_lcd = false;
bool button_tripped = false;

LiquidCrystal_PCF8574 lcd(LCD_ADDR);

// -----------------------------------------------------------

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(BUTTON_LED, OUTPUT);  // button output
  pinMode(BUTTON_INPUT, INPUT); // button in

  if (checkForLCD() != 0)
  {
    Serial.println("LCD not found, have you wired it correctly?");
    for (;;)
      ; //loop forever, causes it to hang and reset
  }

  lcd.begin(16, 2); // initialize the lcd
  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();

  //here we will try to connect to the network, then print IP
  //"0123456789ABCDEF"
  lcd.print("Connect 2 netwrk");
  WiFi.mode(WIFI_STA);
  WiFi.begin(NETWORK_SSID, NETWORK_PW);

  lcd.setCursor(0, 1);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    lcd.print(".");
  }

  //connected to the network, and everything seems fine
  // start the server and the DNS

  server.on("/lcd", HTTP_POST, []() {
    String t1 = server.arg("line1");
    String t2 = server.arg("line2");
    String opt = server.arg("opt");

    blink_delay = opt.toInt();

    if (blink_delay)
    {
      blink = true;
    }
    else
    {
      blink = false;
    }

    //below, we no longer have to use the decode
    //function, as we're using post requests
    //try GET requests instead, and you will have to
    //encode on the computer side and decode the device

    line1 = t1; //line1 = decode(t1);
    line2 = t2; //line2 = decode(t2);

    updateLCD();
    server.send(200, "set");
  });
  server.on("/lcd", HTTP_GET, []() {
    String output = line1 + "<BR>" + line2;

    server.send(200, output.c_str());
  });

  server.on("/", HTTP_GET, []() {
    if (button_tripped)
    {
      server.send(201, "button pressed");
    }
    else
    {
      server.send(200, "no button press");
    }
  });

  server.on("/acknowledge", HTTP_POST, []() {
    if (button_tripped)
    {
      button_tripped = false;
      server.send(200, "button reset");
    }
    else
    {
      server.send(403, "button not set");
    }
  });

  MDNS.begin("esp8266");
  server.begin();
  Serial.println("Server started on");
  Serial.println(WiFi.localIP());
  outputConnection();
}

void loop()
{

  if (blink)
  {
    if (blink_timer + blink_delay < millis())
    {
      digitalWrite(BUTTON_LED, !digitalRead(BUTTON_LED));
      blink_timer = millis();
    }
  }
  else
  {
    digitalWrite(BUTTON_LED, HIGH);
  }

  if (digitalRead(BUTTON_INPUT) == LOW)
  {
    button_tripped = true;
  }

  server.handleClient();
  MDNS.update();
}

void updateLCD()
{

  if (line1.length() == 0 && line2.length() == 0)
  {
    outputConnection();
    return;
  }

  lcd.home();
  lcd.clear();
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

int checkForLCD()
{
  //returns an int value on what the LCD says
  // if the LCD is there, it should return 0
  Wire.begin();
  Wire.beginTransmission(0x27);
  return Wire.endTransmission();
}

void outputConnection()
{
  lcd.home();
  lcd.clear();
  //"0123456789ABCDF"
  lcd.print("CON OK, waiting");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
}

String decode(String &in)
{

  String out = "";

  bool flag = false;
  short high = 0;
  short low = 0;
  char translated = 0;

  for (unsigned int i = 0; i < in.length(); i++)
  {

    char c = in.charAt(i);

    if (c == '%')
    {
      flag = true;
      continue;
    }
    if (flag)
    {
      if (high == 0)
      {
        high = (short)c; //first code
      }
      else
      {
        low = (short)c; //second code
      }

      if (high && low)
      {
        translated = characterCode(high, low);

        high = 0;
        low = 0;
        flag = false;

        if (translated)
        {
          out += translated;
        } //skip if we couldn't do it
      }

      continue;
    }

    out += in.charAt(i);
  }
  return out;
}
char characterCode(short high, short low)
{
  high = convert(high);
  low = convert(low);
  if (high == -1 || low == -1)
    return 0;

  return (high << 4 | low);
}
short convert(char c)
{
  if (c >= '0' && c <= '9')
    return (c - '0') & 0x0F;
  if (c >= 'A' && c <= 'F')
    return (10 + c - 'A') & 0x0F;
  return -1;
}
