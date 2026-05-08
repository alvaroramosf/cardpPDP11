#ifndef OPTIONS_H
#define OPTIONS_H

#include <Arduino.h>

enum FontSize {
    FONT_NORMAL = 0, // Font0
    FONT_LARGE = 1,  // Font2
    FONT_SMALL = 2   // TomThumb or custom small
};

enum TermColor {
    COLOR_GREEN = 0,
    COLOR_AMBER = 1,
    COLOR_WHITE = 2,
    COLOR_PAPER = 3
};

enum TermMode {
    MODE_VT100 = 0,
    MODE_ENHANCED = 1
};

struct EmulatorOptions {
    int last_disk;
    FontSize font_size;
    TermColor term_color;
    int brightness;
    TermMode term_mode;
};

extern EmulatorOptions current_options;

void loadOptions();
void saveOptions();
void applyOptions();
void openOptionsMenu();
void redraw_terminal(); // defined in main.cpp

// Export total disk count and names from main.cpp
extern int cntr;
extern String Fnames[64];

// Notify main that soft reset is requested
extern bool request_soft_reset;
extern int soft_reset_disk_idx;

#endif // OPTIONS_H
