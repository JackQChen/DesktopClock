// for ESP8266
#include <Arduino.h>
#include <U8g2lib.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "icon.h"

//for ESP32
//#include <Arduino.h>
//#include <U8g2lib.h>
//#include <NTPClient.h>
//#include <WiFi.h>
//#include <WiFiUdp.h>
//#include <WiFiClientSecure.h>
//#include <HTTPClient.h>
//#include <ArduinoJson.h>
//#include "icon.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h> 
#endif

const char* ssid = "No_Comment";
const char* password = "---";

const char* cityCode = "101110102"; //ChangAn

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", /*GMT+8*/ 60 * 60 * 8, 30 * 60 * 1000);

HTTPClient httpClient;
DynamicJsonDocument doc(1024);

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);
int frame = -1, weather = 0;
unsigned long epochTime;
bool showColon = false;
String weekDays[7] = { "日", "一", "二","三", "四", "五", "六" };
String date = "---- -- --", h = "--", m = "--", w = "-", temp = "--", hum = "--", aqi = "---";

void setup() {
	Serial.begin(115200);
	while (!Serial);

	u8g2.begin();
	u8g2.enableUTF8Print();

	u8g2.setFont(u8g2_font_wqy16_t_gb2312);
	u8g2.setCursor(15, 36);
	u8g2.print("正在初始化...");
	u8g2.sendBuffer();

	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print("Connecting to WIFI...");
	}

	timeClient.begin();
}

void draw()
{
	u8g2.clearBuffer();

	//weather
	u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);
	u8g2.drawGlyph(3, 32, weather);

	//time
	u8g2.setFont(u8g2_font_logisoso30_tn);
	u8g2.setCursor(40, 30);
	u8g2.print(h);
	u8g2.setCursor(87, 30);
	u8g2.print(m);
	u8g2.setCursor(76, 28);
	showColon = !showColon;
	u8g2.print(showColon ? ":" : " ");

	//temperature
	u8g2.setFont(u8g2_font_VCR_OSD_tf);
	u8g2.setCursor(5, 52);
	u8g2.print(temp);
	u8g2.drawXBMP(30, 36, 16, 16, col_temp);

	//hum
	u8g2.setCursor(49, 52);
	u8g2.print(String(hum));
	u8g2.setFont(u8g2_font_luBS12_tr);
	u8g2.setCursor(74, 50);
	u8g2.print("%");

	//aqi
	u8g2.setFont(u8g2_font_VCR_OSD_tf);
	u8g2.setCursor(92, 52);
	u8g2.print(aqi);

	//date
	u8g2.setFont(u8g2_font_smart_patrol_nbp_tn);
	u8g2.setCursor(5, 64);
	u8g2.print(date);
	u8g2.setFont(u8g2_font_wqy12_t_gb2312);
	u8g2.setCursor(90, 63);
	u8g2.print("星期" + w);

	u8g2.sendBuffer();
}

String padLeft(int val)
{
	return padLeft(val, "0");
}

String padLeft(int val, String chr)
{
	return (val > 9 ? "" : chr) + String(val);
}

void loop() {
	frame++;
	if (frame >= 3600)
		frame = 0;

	//datetime
	if (frame % 60 == 0)
	{
		if (timeClient.update())
			epochTime = timeClient.getEpochTime();
		else
		{
			frame = -3;
			return;
		}
	}

	//datetime
	if (frame % 60 == 0)
	{
		struct tm* ptm = gmtime((time_t*)&epochTime);
		date = String(1900 + ptm->tm_year) + "-" + padLeft(1 + ptm->tm_mon) + "-" + padLeft(ptm->tm_mday);
		h = padLeft(timeClient.getHours());
		m = padLeft(timeClient.getMinutes());
		w = weekDays[timeClient.getDay()];
	}

	//weather
	if (frame % 1800 == 0)
	{
		httpClient.begin("http://d1.weather.com.cn/sk_2d/" + String(cityCode) + ".html?_=" + String(epochTime));
		httpClient.addHeader("Referer", "http://www.weather.com.cn/weather1d/" + String(cityCode) + ".shtml");
		httpClient.setUserAgent("Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36");
		if (httpClient.GET() == HTTP_CODE_OK)
		{
			String payload = httpClient.getString();
			payload = payload.substring(payload.indexOf("=") + 1);
			deserializeJson(doc, payload);
			temp = padLeft(doc["temp"].as<int>(), " ");
			hum = "52";
			aqi = doc["aqi"].as<String>();
			String strWeather = doc["weathere"].as<String>();
			strWeather.toLowerCase();
			if (strWeather == "foggy") weather = /*多云*/ 64;
			else if (strWeather == "cloudy") weather = /*晴转多云*/ 65;
			else if (strWeather == "sunny") weather = /*晴*/ 66;
			else if (strWeather == "sunny") weather = /*晴*/ 69;
			else if (strWeather == "rainy") weather = /*雨*/ 67;
		}
	}

	draw();
	delay(1000);
}
