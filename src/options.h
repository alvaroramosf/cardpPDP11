#ifndef OPTIONS_H
#define OPTIONS_H

#include <Arduino.h>

enum TermColor {
    COLOR_GREEN = 0,
    COLOR_AMBER = 1,
    COLOR_WHITE = 2,
    COLOR_PAPER = 3
};

struct EmulatorOptions {
    int last_disk;
    TermColor term_color;
    int brightness;
};

extern EmulatorOptions current_options;

void loadOptions();
void saveOptions();
void applyOptions();
void openOptionsMenu();
void openMonitorMode();

// Export total disk count and names from main.cpp
extern int cntr;
extern String Fnames[64];

// Notify main that soft reset is requested
extern bool request_soft_reset;
extern int soft_reset_disk_idx;

#endif // OPTIONS_H
