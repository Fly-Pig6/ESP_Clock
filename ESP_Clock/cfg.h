/* 配置文件
 */

#define WIFI_SSID "WiFi SSID"
#define WIFI_PASS "WiFi密码"
const String key = "产品密钥";
const String location = "城市名";

#define DRAK_MODE 0 /* 暗色模式开关: `1` 代表启用 */

#if DRAK_MODE

#define MAIN_COLOUR 0x39E7
#define BG_COLOUR 0x0000
#define TEXT_COLOUR 0xFFFF
#define ICON_COLOUR 0xFFFF

#else

#define MAIN_COLOUR 0x9D13
#define BG_COLOUR 0xFFFF
#define TEXT_COLOUR 0x0000
#define ICON_COLOUR 0x0000

#endif