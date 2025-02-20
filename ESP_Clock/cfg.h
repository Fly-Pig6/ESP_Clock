/* 配置文件
 */

#define WIFI_SSID "WiFi名"
#define WIFI_PASS "WiFi密码"
const String key = "API密钥";
const String location = "城市名";

#define DRAK_MODE 1 /* 暗色模式开关: `1` 代表启用 */

#if DRAK_MODE

#define MAIN_COLOUR 0xA4D6
#define BG_COLOUR 0x204D
#define TEXT_COLOUR 0xFFFF
#define ICON_COLOUR 0xFFFF

#else

#define MAIN_COLOUR 0x9D13
#define BG_COLOUR 0xFFFF
#define TEXT_COLOUR 0x0000
#define ICON_COLOUR 0x0000

#endif
