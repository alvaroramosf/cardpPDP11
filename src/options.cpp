#include "options.h"
#include <Preferences.h>
#include <M5Cardputer.h>
#include <M5GFX.h>

EmulatorOptions current_options;
Preferences preferences;

bool request_soft_reset = false;
int soft_reset_disk_idx = 0;

void loadOptions() {
    preferences.begin("pdp11", false);
    current_options.last_disk = preferences.getInt("last_disk", 0);
    current_options.font_size = (FontSize)preferences.getInt("font_size", FONT_NORMAL);
    current_options.term_color = (TermColor)preferences.getInt("term_color", COLOR_GREEN);
    current_options.brightness = preferences.getInt("brightness", 200);
    current_options.term_mode = (TermMode)preferences.getInt("term_mode", MODE_VT100);
    preferences.end();
}

void saveOptions() {
    preferences.begin("pdp11", false);
    preferences.putInt("last_disk", current_options.last_disk);
    preferences.putInt("font_size", current_options.font_size);
    preferences.putInt("term_color", current_options.term_color);
    preferences.putInt("brightness", current_options.brightness);
    preferences.putInt("term_mode", current_options.term_mode);
    preferences.end();
}

void applyOptions() {
    M5Cardputer.Display.setBrightness(current_options.brightness);
    redraw_terminal();
}

static void waitForKeyRelease() {
    while (M5Cardputer.Keyboard.isPressed()) {
        M5Cardputer.update();
        delay(10);
    }
}

static uint16_t getMenuColor() {
    if (current_options.term_color == COLOR_AMBER) return 0xFFB000;
    if (current_options.term_color == COLOR_WHITE) return TFT_WHITE;
    if (current_options.term_color == COLOR_PAPER) return BLACK;
    return TFT_GREEN;
}

static uint16_t getMenuBgColor() {
    if (current_options.term_color == COLOR_PAPER) return 0xFFFFCC;
    return BLACK;
}

// Menu system helpers
static void drawMenuHeader(const char* title) {
    uint16_t fg = getMenuColor();
    uint16_t bg = getMenuBgColor();
    M5Cardputer.Display.fillScreen(bg);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setFont(&fonts::Font0); // Default font for menus
    
    M5Cardputer.Display.setTextColor(fg, bg);
    M5Cardputer.Display.setCursor(4, 2);
    M5Cardputer.Display.print("Emulator Options \xbb ");
    M5Cardputer.Display.print(title);
    M5Cardputer.Display.drawFastHLine(0, 12, 240, fg);
}

static void drawMenuFooter(const char* hints) {
    uint16_t bg = getMenuBgColor();
    // Dark grey doesn't look great on white background, so use a slightly dim color based on bg
    uint16_t fg = (bg == BLACK) ? TFT_DARKGREY : 0x7BEF;
    M5Cardputer.Display.setTextColor(fg, bg);
    M5Cardputer.Display.setCursor(4, 135 - 10);
    M5Cardputer.Display.print(hints);
}

static void drawMenuList(int num_items, int selected, const char* items[], int active_idx = -1) {
    uint16_t fg = getMenuColor();
    uint16_t bg = getMenuBgColor();
    const int lineH = 9;
    const int startY = 15;
    const int maxVisible = (135 - startY - 12) / lineH;

    int firstVisible = 0;
    if (selected >= maxVisible) firstVisible = selected - maxVisible + 1;

    for (int i = firstVisible; i < num_items && (i - firstVisible) < maxVisible; i++) {
        int y = startY + (i - firstVisible) * lineH;
        
        bool isActive = (i == active_idx);
        String label = String(i + 1) + ". " + items[i];
        if (isActive) label += " *";
        
        if (i == selected) {
            M5Cardputer.Display.fillRect(0, y - 1, 240, lineH, fg);
            M5Cardputer.Display.setTextColor(bg, fg);
        } else {
            M5Cardputer.Display.setTextColor(fg, bg);
        }
        
        M5Cardputer.Display.setCursor(4, y);
        M5Cardputer.Display.print(label);
    }
}

// Submenus
static void menuDiskImage() {
    int sel = current_options.last_disk;
    if (sel >= cntr) sel = 0;
    
    bool redraw = true;
    while(true) {
        if (redraw) {
            drawMenuHeader("Disk Image");
            // Convert String[] to const char*[]
            const char* c_items[64];
            for (int i=0; i<cntr; i++) c_items[i] = Fnames[i].c_str();
            drawMenuList(cntr, sel, c_items, current_options.last_disk);
            drawMenuFooter("; Up  . Down  Enter Select  Esc Back");
            redraw = false;
        }
        
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto status = M5Cardputer.Keyboard.keysState();
            for (auto ch : status.word) {
                if (ch == ';') { if (sel > 0) { sel--; redraw = true; } }
                if (ch == '.') { if (sel < cntr - 1) { sel++; redraw = true; } }
            }
            if (status.enter) {
                current_options.last_disk = sel;
                waitForKeyRelease();
                return;
            }
            bool esc_pressed = status.del;
            for (auto ch : status.word) {
                if (ch == 27 || ch == '`') esc_pressed = true;
            }
            if (esc_pressed) {
                waitForKeyRelease();
                return;
            }
        }
        delay(20);
    }
}

static void menuFontSize() {
    int sel = current_options.font_size;
    const char* items[] = {"Normal (6x8, default)", "Large (Font2)", "Small (TomThumb)"};
    bool redraw = true;
    while(true) {
        if (redraw) {
            drawMenuHeader("Font Size");
            drawMenuList(3, sel, items, current_options.font_size);
            drawMenuFooter("; Up  . Down  Enter Select  Esc Back");
            redraw = false;
        }
        
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto status = M5Cardputer.Keyboard.keysState();
            for (auto ch : status.word) {
                if (ch == ';') { if (sel > 0) { sel--; redraw = true; } }
                if (ch == '.') { if (sel < 2) { sel++; redraw = true; } }
            }
            if (status.enter) {
                current_options.font_size = (FontSize)sel;
                waitForKeyRelease();
                return;
            }
            bool esc_pressed = status.del;
            for (auto ch : status.word) {
                if (ch == 27 || ch == '`') esc_pressed = true;
            }
            if (esc_pressed) {
                waitForKeyRelease();
                return;
            }
        }
        delay(20);
    }
}

static void menuTerminalColor() {
    int sel = current_options.term_color;
    const char* items[] = {"Green (Phosphor)", "Amber (Phosphor)", "White", "Paper (Dark on Light)"};
    bool redraw = true;
    while(true) {
        if (redraw) {
            drawMenuHeader("Text and Terminal Colour");
            drawMenuList(4, sel, items, current_options.term_color);
            drawMenuFooter("; Up  . Down  Enter Select  Esc Back");
            redraw = false;
        }
        
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto status = M5Cardputer.Keyboard.keysState();
            for (auto ch : status.word) {
                if (ch == ';') { if (sel > 0) { sel--; redraw = true; } }
                if (ch == '.') { if (sel < 3) { sel++; redraw = true; } }
            }
            if (status.enter) {
                current_options.term_color = (TermColor)sel;
                waitForKeyRelease();
                return;
            }
            bool esc_pressed = status.del;
            for (auto ch : status.word) {
                if (ch == 27 || ch == '`') esc_pressed = true;
            }
            if (esc_pressed) {
                waitForKeyRelease();
                return;
            }
        }
        delay(20);
    }
}

static void menuBrightness() {
    int sel = 0;
    int bvals[] = {51, 102, 153, 204, 255}; // ~20, 40, 60, 80, 100%
    for(int i=0; i<5; i++) { if (current_options.brightness <= bvals[i]) { sel = i; break; } }
    
    const char* items[] = {"20%", "40%", "60%", "80%", "100%"};
    bool redraw = true;
    while(true) {
        if (redraw) {
            drawMenuHeader("Brightness");
            drawMenuList(5, sel, items, sel);
            drawMenuFooter("; Up  . Down  Enter Select  Esc Back");
            redraw = false;
        }
        
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto status = M5Cardputer.Keyboard.keysState();
            for (auto ch : status.word) {
                if (ch == ';') { if (sel > 0) { sel--; redraw = true; } }
                if (ch == '.') { if (sel < 4) { sel++; redraw = true; } }
            }
            if (status.enter) {
                current_options.brightness = bvals[sel];
                M5Cardputer.Display.setBrightness(bvals[sel]); // apply immediately
                waitForKeyRelease();
                return;
            }
            bool esc_pressed = status.del;
            for (auto ch : status.word) {
                if (ch == 27 || ch == '`') esc_pressed = true;
            }
            if (esc_pressed) {
                waitForKeyRelease();
                return;
            }
        }
        delay(20);
    }
}

static void menuTerminalMode() {
    int sel = current_options.term_mode;
    const char* items[] = {"VT100 (Historical, RAW)", "Enhanced (Local Echo/History)"};
    bool redraw = true;
    while(true) {
        if (redraw) {
            drawMenuHeader("Terminal Mode");
            drawMenuList(2, sel, items, current_options.term_mode);
            drawMenuFooter("; Up  . Down  Enter Select  Esc Back");
            redraw = false;
        }
        
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto status = M5Cardputer.Keyboard.keysState();
            for (auto ch : status.word) {
                if (ch == ';') { if (sel > 0) { sel--; redraw = true; } }
                if (ch == '.') { if (sel < 1) { sel++; redraw = true; } }
            }
            if (status.enter) {
                current_options.term_mode = (TermMode)sel;
                waitForKeyRelease();
                return;
            }
            bool esc_pressed = status.del;
            for (auto ch : status.word) {
                if (ch == 27 || ch == '`') esc_pressed = true;
            }
            if (esc_pressed) {
                waitForKeyRelease();
                return;
            }
        }
        delay(20);
    }
}

static void menuBattery() {
    bool redraw = true;
    while(true) {
        if (redraw) {
            drawMenuHeader("Battery Status");
            uint16_t fg = getMenuColor();
            uint16_t bg = getMenuBgColor();
            M5Cardputer.Display.setTextColor(fg, bg);
            int bat = M5Cardputer.Power.getBatteryLevel();
            float vol = M5Cardputer.Power.getBatteryVoltage() / 1000.0;
            bool chg = M5Cardputer.Power.isCharging();
            
            M5Cardputer.Display.setCursor(4, 20);
            M5Cardputer.Display.printf("Level:   %d %%\n", bat);
            M5Cardputer.Display.setCursor(4, 35);
            M5Cardputer.Display.printf("Voltage: %.2f V\n", vol);
            M5Cardputer.Display.setCursor(4, 50);
            M5Cardputer.Display.printf("Status:  %s\n", chg ? "Charging" : "Discharging");
            
            drawMenuFooter("Esc Back");
            redraw = false;
        }
        
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto status = M5Cardputer.Keyboard.keysState();
            bool esc_pressed = status.del;
            for (auto ch : status.word) {
                if (ch == 27 || ch == '`') esc_pressed = true;
            }
            if (esc_pressed) {
                waitForKeyRelease();
                return;
            }
        }
        delay(20);
    }
}

// Main Options Menu
void openOptionsMenu() {
    int sel = 0;
    
    // Create a backup of current options so we can cancel without saving
    EmulatorOptions backup = current_options;
    
    bool redraw = true;
    int num_items = 8;
    while(true) {
        if (redraw) {
            String disk_item = "Disk Image: " + Fnames[current_options.last_disk];
            const char* items[] = {
                disk_item.c_str(),
                "Font Size",
                "Text and Terminal Colour",
                "Brightness",
                "Terminal Mode",
                "Battery Status",
                "Save and run emulator",
                "Exit (without saving)"
            };
            
            drawMenuHeader("Main Menu");
            drawMenuList(num_items, sel, items);
            drawMenuFooter("; Up  . Down  Enter Select  Esc Exit (no save)");
            redraw = false;
        }
        
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto status = M5Cardputer.Keyboard.keysState();
            bool handled = false;
            bool esc_pressed = status.del;
            for (auto ch : status.word) {
                if (ch == ';') { if (sel > 0) { sel--; redraw = true; handled = true; } }
                if (ch == '.') { if (sel < num_items - 1) { sel++; redraw = true; handled = true; } }
                if (ch == 27 || ch == '`') esc_pressed = true;
            }
            if (status.enter && !handled) {
                switch(sel) {
                    case 0: menuDiskImage(); break;
                    case 1: menuFontSize(); break;
                    case 2: menuTerminalColor(); break;
                    case 3: menuBrightness(); break;
                    case 4: menuTerminalMode(); break;
                    case 5: menuBattery(); break;
                    case 6: // Save and run
                        saveOptions();
                        applyOptions();
                        waitForKeyRelease();
                        // if disk changed, soft reset
                        if (backup.last_disk != current_options.last_disk) {
                            request_soft_reset = true;
                            soft_reset_disk_idx = current_options.last_disk;
                        }
                        return;
                    case 7: // Exit without saving
                        current_options = backup;
                        applyOptions();
                        waitForKeyRelease();
                        return;
                }
                redraw = true;
            }
            if (esc_pressed) {
                current_options = backup;
                applyOptions();
                waitForKeyRelease();
                return;
            }
        }
        delay(20);
    }
}
