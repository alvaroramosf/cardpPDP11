#ifndef OPTIONS_H
#define OPTIONS_H

#include <Arduino.h>

enum TermColor {
    COLOR_GREEN = 0,
    COLOR_AMBER = 1,
    COLOR_WHITE = 2,
    COLOR_PAPER = 3
};

enum CpuModel {
    CPU_PDP1140 = 0,  // PDP-11/40: 18-bit Unibus, no MFPT
    CPU_PDP1123 = 1   // PDP-11/23: 22-bit Q-Bus (F-11), MFPT returns 1
};

struct EmulatorOptions {
    int last_disk;
    TermColor term_color;
    int brightness;
    CpuModel cpu_model;
};

extern EmulatorOptions current_options;

void loadOptions();
void saveOptions();
void applyOptions();
void openOptionsMenu();

// Export total disk count and names from main.cpp
extern int cntr;
extern String Fnames[64];

// Notify main that soft reset is requested
extern bool request_soft_reset;
extern int soft_reset_disk_idx;

#endif // OPTIONS_H
