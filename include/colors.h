#include <gtk/gtk.h>

// classic terminal theme 
#define COLOR_BG { 0.0, 0.0, 0.0, 1.0 }         // #000000
#define COLOR_FG { 1.0, 1.0, 1.0, 1.0 }         // #FFFFFF
// ANSI 24-bit color escape sequences
#define COLOR_BORDER "\033[38;2;51;255;0m"      // #33ff00
#define COLOR_LOGIN_TEXT "\033[1;37m"           // #FFFFFF
#define COLOR_PROMPT_TEXT "\033[38;2;51;255;0m" // #33ff00

// gruvbox theme 
//#define COLOR_BG { 0.157, 0.157, 0.157, 1.0 };     // #282828
//#define COLOR_FG { 0.922, 0.859, 0.698, 1.0 };     // #ebdbb2
//#define COLOR_BORDER "\033[38;2;235;219;178m";     // #ebdbb2
//#define COLOR_LOGIN_TEXT "\033[38;2;142;192;124m"; // #8ec07c
//#define COLOR_PROMPT_TEXT "\033[38;2;184;187;38m"; // #b8bb26
