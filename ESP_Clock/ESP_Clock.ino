/* bilibili@可爱的狗子汪
 */

#define XIAO_ESP32C3_USED  // 如果编译错误提示缺少库, 注释掉这一行重新编译

#include <WiFi.h>
#ifndef XIAO_ESP32C3_USED
#include <NetworkClientSecure.h>
#else
#include <WiFiClientSecure.h>
#endif

#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <RBD_Timer.h>
#include "bmp.h"
#include "fonts.h"

#define TIME (1 << 1)
#define DATE (1 << 2)
#define FORECAST (1 << 3)

/************** 按需要修改以下部分 *********************/

#define WIFI_SSID "< WiFi_SSID >"  // <- ! 必须修改该字串
#define WIFI_PASS "< WiFi密码 >"   // <- ! 必须修改该字串
#define UTC_OFFSET 8
#define UPDATE_INTERVAL_M 15

/*
 * [心知天气 - 登录](https://www.seniverse.com/login)
 * [心知天气 - 控制台](https://www.seniverse.com/dashboard)
 */

const char* ntpServer = "ntp.aliyun.com";
const char* host = "api.seniverse.com";
const String key = "< API_KEY >";        // <- ! 必须修改该字串
const String location = "< 所在城市 >";  // <- ! 必须修改该字串

/*****************************************************/
#ifndef XIAO_ESP32C3_USED
NetworkClientSecure client;
#else
WiFiClientSecure client;
#endif

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
TFT_eSprite st = TFT_eSprite(&tft);

RBD::Timer timer1(60000 * UPDATE_INTERVAL_M);  // 获取心知天气信息间隔
RBD::Timer timer2(10000);                      // 滚动文字的驻屏显示时间
RBD::Timer timer3(25);                         // 限制滚动文字滚动太快, 每一帧与上一帧的间隔时间

typedef struct {
  String text;
  int code;
  int high;
  int low;
  String windDirection;
  double windSpeed;
  int humidity;
} forecast;
forecast day[3];
forecast now;
String stext[3];
int stextIndex = 0;

char ot[9];
char t[9];
char odate[11];
char date[11];
char wday[4];
uint8_t updateThis = 0xFF;
int tcount = 0;
bool canScroll = false;

void setup() {
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLUE);
  tft.println("Please wait...");

  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(1500);

  client.setInsecure();  // HTTPS不安全连接

  timer1.restart();  // 计时器
  timer2.restart();
  timer3.restart();
  configTime(3600 * UTC_OFFSET, 0, ntpServer);  // NTP服务器
  updateForecastDaily();
  updateWeather();

  tft.fillScreen(TFT_BLACK);
  spr.setColorDepth(8);
  spr.createSprite(260, 48);

  st.setColorDepth(8);
  st.createSprite(260, 17);
  st.fillSprite(TFT_BLACK);
  st.setTextColor(TFT_WHITE);
  st.setTextDatum(TC_DATUM);
}

/*
 * void loop()
 */
void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {  // 获取NTP时间
    return;
  }
  // 更新时间刷新时间文字
  strftime(t, 9, "%X", &timeinfo);
  if (strcmp(ot, t) != 0) {
    strcpy(ot, t);
    strftime(wday, 4, "%a", &timeinfo);
    strftime(date, 11, "%F", &timeinfo);
    if (strcmp(odate, date) != 0) {
      strcpy(odate, date);
      updateThis |= DATE;
    }
    updateThis |= TIME;
  }

  if (timer1.onRestart()) {
    updateForecastDaily();
    updateWeather();
    updateThis |= FORECAST;
  }

  if (timer2.onRestart())
    canScroll = true;

  display();  // 刷新显示
}

/*
 * 根据API返回的JSON数据中的`code`字段, 绘制相应的天气图标
 * "bmp.h"中修改对应图标或者添加更多天气图标!
 */
const uint8_t* toIconBitmap(int codeNum) {
  switch (codeNum) {
    case 0: return _0;  // 晴
    case 1: return _0;
    case 2: return _0;
    case 3: return _0;
    case 4: return _4;  // 多云
    case 5: return _5;  // 晴间多云
    case 6: return _5;
    case 7: return _5;
    case 8: return _5;
    case 9: return _9;    // 阴
    default: return _99;  // 未知
  }
}

void set_stext() {
  stext[0] = now.text + "  " + String(now.low) + "`C";
  stext[1] = String(day[0].windDirection) + "  " + String(day[0].windSpeed) + " km/h";
  stext[2] = (WiFi.localIP()).toString();
}

/* 
 * 在屏幕上绘制图像, 文字等
 */
void display() {
  tft.resetViewport();
  // 圆角矩形框架
  tft.drawRoundRect(5, 5, 310, 120, 9, 0x39E7);
  tft.drawRoundRect(5, 130, 100, 105, 9, 0x39E7);
  tft.drawRoundRect(110, 130, 100, 105, 9, 0x39E7);
  tft.drawRoundRect(215, 130, 100, 105, 9, 0x39E7);

  // 滚动文字
  tft.setViewport(5, 5, 310, 120);
  tft.drawXBitmap(-10, 82, clock48x48, 48, 48, 0x39E7);
  st.pushSprite(25, 14);
  // 文字滚动效果
  if (timer3.onRestart() && canScroll) {
    st.scroll(0, -1);
    tcount--;
  }
  // 复位文字
  if (tcount <= 0) {
    tcount = 17;
    canScroll = false;
    set_stext();
    st.drawString(stext[stextIndex], 130, 0, 2);
    stextIndex = (stextIndex < 2 ? stextIndex + 1 : 0);
  }
  // 时间文字
  if (TIME & updateThis) {
    spr.fillSprite(TFT_BLACK);
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(TFT_WHITE);
    spr.loadFont(gravity_bold);
    spr.drawString(t, 130, 24, 7);
    spr.unloadFont();
    spr.pushSprite(25, 36);
  }
  // 日期文字
  if (DATE & updateThis) {
    tft.setTextDatum(BC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String(wday) + " " + String(date), 155, 110);
  }
  // 绘制底部天气栏
  if (FORECAST & updateThis) {
    tft.resetViewport();
    tft.fillRect(5, 130, 310, 105, TFT_BLACK);
    for (int i = 0; i < 3; i++) {
      tft.resetViewport();
      tft.setViewport(5 + 105 * i, 130, 100, 105);
      tft.drawXBitmap(26, 10, toIconBitmap(day[i].code), WIDTH, HEIGHT, TFT_WHITE);

      tft.drawXBitmap(7, 62, thermometer16x16, 16, 16, TFT_SKYBLUE);
      tft.setTextDatum(TL_DATUM);
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE);
      tft.drawString(String(day[i].low) + "` / " + String(day[i].high) + "`", 30, 62, 2);

      tft.drawXBitmap(7, 84, hygrometer16x16, 16, 16, TFT_ORANGE);
      tft.drawString(String(day[i].humidity) + " %", 30, 84, 2);
    }
  }
  updateThis = 0x00;  // 暂停刷新
}

/*
 * 向指定地址发送GET请求, 并跳过响应头
 */
bool sendRequest(const char* host, String path) {
  if (!client.connect(host, 443)) {
    return false;
  }
  client.println(
    "GET " + path + " HTTP/1.1");
  client.println("Host: " + String(host));
  client.println("Connection: close");
  client.println();

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  return true;
}

/*
 * 更新当前天气数据
 */
void updateWeather() {
  if (!sendRequest(
        host, "/v3/weather/now.json?key=" + key + "&location=" + location + "&language=en")) {
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print("deserializeJson() failed !");
    client.stop();
    return;
  }

  now.text = doc["results"][0]["now"]["text"].as<String>();
  now.low = doc["results"][0]["now"]["temperature"].as<int>();

  client.stop();
}

/*
 * 更新三日天气数据
 */
void updateForecastDaily() {
  if (!sendRequest(
        host, "/v3/weather/daily.json?key=" + key + "&location=" + location + "&language=en")) {
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print("deserializeJson() failed !");
    client.stop();
    return;
  }

  for (int i = 0; i < 3; i++) {
    day[i].text = doc["results"][0]["daily"][i]["text_day"].as<String>();
    day[i].code = doc["results"][0]["daily"][i]["code_day"].as<int>();
    day[i].high = doc["results"][0]["daily"][i]["high"].as<int>();
    day[i].low = doc["results"][0]["daily"][i]["low"].as<int>();
    day[i].windDirection = doc["results"][0]["daily"][i]["wind_direction"].as<String>();
    day[i].windSpeed = doc["results"][0]["daily"][i]["wind_speed"].as<double>();
    day[i].humidity = doc["results"][0]["daily"][i]["humidity"].as<int>();
  }

  client.stop();
}
