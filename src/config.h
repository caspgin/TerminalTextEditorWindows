#ifndef CONFIG_H
#define CONFIG_H

#define CTRL_KEY(k) ((k) & 0x1f);
namespace AppConfig {
constexpr int KILO_VERSION = 1.0;
constexpr int TAB_STOP = 4;
constexpr int SIDE_PANEL_WIDTH = 5;
constexpr int MESSAGE_DISPLAY_TIME = 5;
constexpr int ctrlkey(char k) { return ((k) & 0x1f); }
enum class editorKey {
    UNKNOWN_KEY = -2,
    NO_KEY = -1,
    CARRIAGE = 13,
    ESCAPE = 27,
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN,
    HOME,
    END,
    DEL_KEY,
};
}  // namespace AppConfig
//
#endif
