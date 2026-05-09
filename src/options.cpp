#include "options.h"
#include <Preferences.h>
#include <M5Cardputer.h>
#include <M5GFX.h>
#include "kb11.h"

extern KB11 cpu;

EmulatorOptions current_options;
Preferences preferences;

bool request_soft_reset = false;
int soft_reset_disk_idx = 0;

void loadOptions() {
    preferences.begin("pdp11", false);
    current_options.last_disk = preferences.getInt("last_disk", 0);
    current_options.term_color = (TermColor)preferences.getInt("term_color", COLOR_GREEN);
    current_options.brightness = preferences.getInt("brightness", 200);
    preferences.end();
}

void saveOptions() {
    preferences.begin("pdp11", false);
    preferences.putInt("last_disk", current_options.last_disk);
    preferences.putInt("term_color", current_options.term_color);
    preferences.putInt("brightness", current_options.brightness);
    preferences.end();
}

void applyOptions() {
    M5Cardputer.Display.setBrightness(current_options.brightness);
    extern void update_canvas_colors();
    update_canvas_colors();
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

// Submenus removed to simplify

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

// Submenus removed to simplify

static void menuBattery() {
    bool redraw = true;
    while(true) {
        if (redraw) {
            drawMenuHeader("Battery Status");
            uint16_t fg = getMenuColor();
            uint16_t bg = getMenuBgColor();
            M5Cardputer.Display.setTextColor(fg, bg);
            
            float vol = M5Cardputer.Power.getBatteryVoltage() / 1000.0;
            
            // Manual calculation since hardware charging status isn't reliable on Cardputer
            int pct = (int)((vol - 3.3) / (4.15 - 3.3) * 100);
            if (pct > 100) pct = 100;
            if (pct < 0) pct = 0;
            
            M5Cardputer.Display.setCursor(4, 20);
            M5Cardputer.Display.printf("Est. Level: %d %%\n", pct);
            M5Cardputer.Display.setCursor(4, 35);
            M5Cardputer.Display.printf("Voltage:    %.2f V\n", vol);
            M5Cardputer.Display.setCursor(4, 50);
            M5Cardputer.Display.printf("Est. Level based on voltage.");
            
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

static void menuSystemInfo() {
    bool redraw = true;
    while(true) {
        if (redraw) {
            drawMenuHeader("System Info");
            uint16_t fg = getMenuColor();
            uint16_t bg = getMenuBgColor();
            M5Cardputer.Display.setTextColor(fg, bg);
            
            uint32_t free_ram = ESP.getFreeHeap();
            uint32_t min_free_ram = ESP.getMinFreeHeap();
            
            M5Cardputer.Display.setCursor(4, 20);
            M5Cardputer.Display.printf("Software Version: 0.1.0\n");
            M5Cardputer.Display.setCursor(4, 35);
            M5Cardputer.Display.printf("Free RAM:     %lu KB\n", (unsigned long)(free_ram / 1024));
            M5Cardputer.Display.setCursor(4, 50);
            M5Cardputer.Display.printf("Min Free RAM: %lu KB\n", (unsigned long)(min_free_ram / 1024));
            
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
    int num_items = 6;
    while(true) {
        if (redraw) {
            String disk_item = "Disk Image: " + Fnames[current_options.last_disk];
            const char* items[] = {
                disk_item.c_str(),
                "Text Colour",
                "Brightness",
                "System Info",
                "Battery Status",
                "System Reset"
            };
            
            drawMenuHeader("Main Menu");
            drawMenuList(num_items, sel, items);
            drawMenuFooter("; Up  . Down  Enter Select  Esc Exit (no save)");
            redraw = false;
        }
        
        M5Cardputer.update();
        bool g0_pressed = M5Cardputer.BtnA.wasPressed();
        
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
                    case 1: menuTerminalColor(); break;
                    case 2: menuBrightness(); break;
                    case 3: menuSystemInfo(); break;
                    case 4: menuBattery(); break;
                    case 5:
                        request_soft_reset = true;
                        soft_reset_disk_idx = current_options.last_disk;
                        waitForKeyRelease();
                        return;
                }
                redraw = true;
            }
            if (esc_pressed || g0_pressed) {
                saveOptions();
                applyOptions();
                if (backup.last_disk != current_options.last_disk) {
                    request_soft_reset = true;
                    soft_reset_disk_idx = current_options.last_disk;
                }
                waitForKeyRelease();
                return;
            }
        } else if (g0_pressed) {
            saveOptions();
            applyOptions();
            if (backup.last_disk != current_options.last_disk) {
                request_soft_reset = true;
                soft_reset_disk_idx = current_options.last_disk;
            }
            return;
        }
        delay(20);
    }
}

void openMonitorMode() {
    extern M5Canvas canvas;
    extern void flush_console();
    
    uint32_t input_val = 0;
    bool has_input = false;
    
    enum OpenType { NONE, MEMORY, REG, PSW_REG };
    OpenType open_type = NONE;
    uint32_t open_addr = 0; 
    bool is_open = false;
    
    bool expect_reg = false;
    
    canvas.print("\r\n[HALTED: ODT] Esc:Resume G0:Menu\r\n@");
    canvas.pushSprite(0, 0);
    
    while (true) {
        M5Cardputer.update();
        
        if (M5Cardputer.BtnA.wasPressed()) {
            waitForKeyRelease();
            openOptionsMenu();
            cpu.wtstate = false;
            canvas.print("[RUNNING]\r\n");
            canvas.pushSprite(0, 0);
            return;
        }
        
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto status = M5Cardputer.Keyboard.keysState();
            
            bool esc_pressed = status.del;
            std::vector<char> chars;
            
            for (auto c : status.word) {
                if (c == 27 || c == '`') esc_pressed = true;
                else {
                    if (status.ctrl) {
                        char cc = c;
                        if (cc >= 'a' && cc <= 'z') cc -= 0x60;
                        else if (cc >= 'A' && cc <= 'Z') cc -= 0x40;
                        chars.push_back(cc);
                    } else {
                        chars.push_back(c);
                    }
                }
            }
            if (status.enter) chars.push_back('\r');
            
            if (esc_pressed) {
                waitForKeyRelease();
                cpu.wtstate = false;
                canvas.print("[RUNNING]\r\n");
                canvas.pushSprite(0, 0);
                return;
            }
            
            for (char ch : chars) {
                char c = toupper(ch);
                if (c == '.') c = '\n'; // '.' acts as <LF> on Cardputer
                
                if (c == 'R' || c == '$') {
                    canvas.print(c);
                    expect_reg = true;
                    has_input = false;
                    input_val = 0;
                    if (!is_open) open_type = NONE;
                }
                else if (expect_reg) {
                    canvas.print(c);
                    expect_reg = false;
                    if (c >= '0' && c <= '7') {
                        input_val = c - '0';
                        has_input = true;
                        open_type = REG;
                    } else if (c == 'S') {
                        has_input = true;
                        open_type = PSW_REG;
                    } else {
                        canvas.print("\r\n?\r\n@");
                        input_val = 0;
                        has_input = false;
                        open_type = NONE;
                        is_open = false;
                    }
                }
                else if (c >= '0' && c <= '7') {
                    canvas.print(c);
                    input_val = (input_val << 3) | (c - '0');
                    input_val &= 0777777;
                    has_input = true;
                    if (open_type == REG && !is_open) {
                        open_type = NONE;
                    }
                }
                else if (c == '/') {
                    canvas.print(c);
                    if (open_type == REG && !is_open) {
                        open_addr = input_val;
                    } else if (open_type == PSW_REG && !is_open) {
                        // addr doesn't matter
                    } else {
                        if (has_input) open_addr = input_val;
                        open_type = MEMORY;
                    }
                    
                    is_open = true;
                    
                    if (open_type == REG) {
                        canvas.printf(" %06o ", cpu.R[open_addr]);
                    } else if (open_type == PSW_REG) {
                        canvas.printf(" %06o ", cpu.PSW);
                    } else {
                        if (open_addr & 1) { 
                            canvas.print("\r\n?\r\n@");
                            open_type = NONE;
                            is_open = false;
                        } else {
                            uint16_t val = cpu.unibus.read16(open_addr);
                            canvas.printf(" %06o ", val);
                        }
                    }
                    input_val = 0;
                    has_input = false;
                }
                else if (c == '\r') {
                    canvas.print("\r\n");
                    if (is_open && has_input) {
                        if (open_type == REG) cpu.R[open_addr] = input_val & 0177777;
                        else if (open_type == PSW_REG) cpu.PSW = input_val & 0177777;
                        else if (open_type == MEMORY) cpu.unibus.write16(open_addr, input_val & 0177777);
                    }
                    open_type = NONE;
                    is_open = false;
                    input_val = 0;
                    has_input = false;
                    canvas.print("@");
                }
                else if (c == '\n') {
                    canvas.print("\r\n");
                    if (is_open && has_input) {
                        if (open_type == REG) cpu.R[open_addr] = input_val & 0177777;
                        else if (open_type == PSW_REG) cpu.PSW = input_val & 0177777;
                        else if (open_type == MEMORY) cpu.unibus.write16(open_addr, input_val & 0177777);
                    }
                    
                    if (is_open) {
                        if (open_type == REG) {
                            open_addr = (open_addr + 1) & 7;
                            canvas.printf("R%d/ %06o ", open_addr, cpu.R[open_addr]);
                            input_val = 0;
                            has_input = false;
                        } else if (open_type == PSW_REG) {
                            open_type = NONE;
                            is_open = false;
                            input_val = 0;
                            has_input = false;
                            canvas.print("@");
                        } else if (open_type == MEMORY) {
                            open_addr = (open_addr + 2) & 0777777;
                            if (open_addr & 1) {
                                canvas.print("?\r\n@");
                                open_type = NONE;
                                is_open = false;
                            } else {
                                uint16_t val = cpu.unibus.read16(open_addr);
                                canvas.printf("%06o/ %06o ", open_addr, val);
                            }
                            input_val = 0;
                            has_input = false;
                        }
                    } else {
                        canvas.print("@");
                        input_val = 0;
                        has_input = false;
                    }
                }
                else if (c == 'G') {
                    canvas.print("G\r\n");
                    if (has_input) {
                        cpu.R[7] = input_val & 0177777;
                    }
                    cpu.wtstate = false;
                    canvas.print("[RUNNING]\r\n");
                    canvas.pushSprite(0, 0);
                    waitForKeyRelease();
                    return;
                }
                else if (c == 'P') {
                    canvas.print("P\r\n");
                    cpu.wtstate = false;
                    canvas.print("[RUNNING]\r\n");
                    canvas.pushSprite(0, 0);
                    waitForKeyRelease();
                    return;
                }
                else {
                    canvas.print(c);
                    canvas.print("\r\n?\r\n@");
                    input_val = 0;
                    has_input = false;
                    expect_reg = false;
                    open_type = NONE;
                    is_open = false;
                }
            }
            canvas.pushSprite(0, 0);
        }
        delay(20);
    }
}
