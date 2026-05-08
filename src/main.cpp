// PDP-11/40 + 11/24 emulator — ported to M5Stack Cardputer
// Original: Isysxp/PDP11-on-the-M5-Core (M5Core2, touch screen)
// Port:     M5Stack Cardputer (ESP32-S3, 240×135 ST7789V2, physical 56-key keyboard)
//
// HARDWARE RESET: The physical side button on the Cardputer performs a hard reset
// of the ESP32-S3 at silicon level — no software code needed.
//
// SD card CS pin: GPIO 12 (handled internally by M5Cardputer BSP).
// WiFi credentials: /Wifi.txt on SD card (line1 = SSID, line2 = password).

#include "M5Cardputer.h"
#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <ESP32Time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Forward declarations ──────────────────────────────────────────────────────
int startup(char *rkfile, char *rlfile, int bootdev);

using namespace std;

// ── Canvas for PDP-11 terminal output ────────────────────────────────────────
// Font0 is a 6×8 monospaced bitmap font → ~39 columns × 16 rows at 240×135
M5Canvas canvas(&M5Cardputer.Display);

// ── Shared state ─────────────────────────────────────────────────────────────
String Fnames[64];
int    cntr    = 0;   // total .RK05/.RL02 images found on SD
int    SelFile = 0;   // currently highlighted menu entry
extern int runFlag;
String ssid, pswd;

static uint32_t lastPush = 0;
static bool display_dirty = false;

void console_output_char(char c) {
    if (c == '\a' || c == 0x07) {
        M5Cardputer.Speaker.tone(880, 150);
        return;
    }
    canvas.print(c);
    display_dirty = true;
}

// Called from the emulator loop periodically so we don't stall the CPU
void flush_console() {
    if (display_dirty && (millis() - lastPush >= 40)) { // Max 25 FPS
        canvas.pushSprite(0, 0);
        lastPush = millis();
        display_dirty = false;
    }
}

// ── Keyboard injection into KL11 ─────────────────────────────────────────────
// keypressed is declared here; kl11.cpp externs it.
// keypressed is owned by kl11.cpp (where it is used); declared extern here
extern bool keypressed;
static char kbuf = 0;

void kl11_rx_char(char c) {
    kbuf      = c;
    keypressed = true;
}
char kl11_get_kbuf() {
    char c = kbuf;
    kbuf   = 0;
    return c;
}

// ── SD scan for disk images ───────────────────────────────────────────────────
static void scanDiskImages(fs::FS &fs, const char *dirname) {
    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) return;
    File file = root.openNextFile();
    while (file && cntr < 64) {
        if (!file.isDirectory()) {
            const char *name = file.name();
            // Strip leading '/' — modern Arduino-ESP32 SD includes it,
            // we add it back explicitly when building the full path.
            if (name[0] == '/') name++;
            if (strcasestr(name, ".rk05") || strcasestr(name, ".rl0")) {
                Fnames[cntr++] = name;
                Serial.printf("  [%d] %s  (%lu B)\r\n", cntr, name,
                              (unsigned long)file.size());
            }
        }
        file = root.openNextFile();
    }
}

// ── Menu rendering ────────────────────────────────────────────────────────────
// ; = up, . = down, Enter = boot.  Selected entry: green bg / black text.
static void drawMenu() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextSize(1);

    // Title bar
    M5Cardputer.Display.setTextColor(TFT_CYAN, BLACK);
    M5Cardputer.Display.setCursor(4, 2);
    M5Cardputer.Display.print("PDP-11  \xbb  Select disk image");
    M5Cardputer.Display.drawFastHLine(0, 12, 240, TFT_CYAN);

    const int lineH      = 9;
    const int startY     = 15;
    const int maxVisible = (135 - startY - 12) / lineH;

    int firstVisible = 0;
    if (SelFile >= maxVisible) firstVisible = SelFile - maxVisible + 1;

    for (int i = firstVisible;
         i < cntr && (i - firstVisible) < maxVisible; i++) {
        int y = startY + (i - firstVisible) * lineH;
        if (i == SelFile) {
            M5Cardputer.Display.fillRect(0, y - 1, 240, lineH, TFT_GREEN);
            M5Cardputer.Display.setTextColor(BLACK, TFT_GREEN);
        } else {
            M5Cardputer.Display.setTextColor(TFT_GREEN, BLACK);
        }
        M5Cardputer.Display.setCursor(4, y);
        String label = String(i + 1) + ". " + Fnames[i];
        if (label.length() > 38) label = label.substring(0, 37) + "~";
        M5Cardputer.Display.print(label);
    }

    // Hint bar at bottom
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, BLACK);
    M5Cardputer.Display.setCursor(4, 135 - 10);
    M5Cardputer.Display.print("; Up   . Down   Enter Boot");
}

// ── setup() ──────────────────────────────────────────────────────────────────
void setup() {
    char rkfile[64] = {0};
    char rlfile[64] = {0};
    int  bootdev    = 0;

    // Hardware init
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);          // true = enable keyboard
    M5Cardputer.Display.setRotation(1);    // landscape 240×135
    M5Cardputer.Display.setBrightness(200);
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Speaker.setVolume(128);

    Serial.begin(115200);
    // Small delay so CDC-USB serial host can attach
    delay(500);
    Serial.println("Cardputer PDP-11 starting...");

    // Canvas for the emulator terminal (1-bit color depth to save ~60KB RAM)
    canvas.setColorDepth(1);
    canvas.createSprite(240, 135);
    canvas.setPaletteColor(0, BLACK);
    canvas.setPaletteColor(1, TFT_GREEN);
    canvas.setTextScroll(true);
    canvas.setTextColor(1, 0); // Color 1 (Green), Bg 0 (Black)
    canvas.setFont(&fonts::Font0);  // 6×8 mono → ~39 col × 16 rows
    canvas.fillSprite(0);
    canvas.pushSprite(0, 0);

    // Mount SD card
    // Cardputer SD SPI pins: SCK=40, MISO=39, MOSI=14, CS=12
    // These are non-default, so SPI must be explicitly initialized first.
    SPI.begin(40, 39, 14, 12);  // SCK, MISO, MOSI, CS
    if (!SD.begin(12, SPI, 25000000)) {
        Serial.println("SD Card mount failed!");
        M5Cardputer.Display.setTextColor(TFT_RED, BLACK);
        M5Cardputer.Display.setCursor(4, 4);
        M5Cardputer.Display.print("SD Card mount failed!");
        M5Cardputer.Display.setCursor(4, 14);
        M5Cardputer.Display.print("Check: FAT32, card seated");
        while (1) delay(1000);
    }
    Serial.printf("SD: %llu MB  Free heap: %d  Free PSRAM: %lu\r\n",
                  SD.totalBytes() / (1024ULL * 1024ULL),
                  ESP.getFreeHeap(),
                  (unsigned long)ESP.getFreePsram());

    // Scan SD for disk images
    scanDiskImages(SD, "/");
    if (cntr == 0) {
        Serial.println("No .RK05/.RL02 images on SD!");
        M5Cardputer.Display.setTextColor(TFT_RED, BLACK);
        M5Cardputer.Display.setCursor(4, 4);
        M5Cardputer.Display.print("No disk images on SD!");
        while (1) delay(1000);
    }

    // Keyboard-driven disk selection menu
    SelFile = 0;
    drawMenu();
    bool menuDone = false;
    while (!menuDone) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            bool redraw = false;
            for (auto ch : status.word) {
                if (ch == ';') {
                    if (SelFile > 0) { SelFile--; redraw = true; }
                } else if (ch == '.') {
                    if (SelFile < cntr - 1) { SelFile++; redraw = true; }
                }
            }
            if (status.enter) menuDone = true;
            if (redraw) drawMenu();
        }
        delay(20);
    }

    // Derive boot device from file extension
    // Fnames[] stores names WITHOUT leading '/'; we prepend it here.
    const char *selName = Fnames[SelFile].c_str();
    strcpy(rkfile, "/Empty_RK05.dsk");
    strcpy(rlfile, "/Empty_RL02.dsk");  // RL02 default (might not exist, handled in rk11/rl11)

    if (strcasestr(selName, ".rk")) {
        snprintf(rkfile, sizeof(rkfile), "/%s", selName);
        bootdev = 0;
    } else if (strcasestr(selName, ".rl")) {
        snprintf(rlfile, sizeof(rlfile), "/%s", selName);
        bootdev = 1;
    }

    Serial.printf("rkfile='%s'  rlfile='%s'  bootdev=%d\r\n", rkfile, rlfile, bootdev);

    // Verify disk file exists before handing to emulator
    {
        File test = SD.open(rkfile, FILE_READ);
        if (!test) {
            Serial.printf("ERROR: cannot open %s\r\n", rkfile);
            canvas.fillSprite(0);
            canvas.setCursor(0, 0);
            canvas.setTextColor(1, 0);
            canvas.printf("Cannot open:\n%s\n", rkfile);
            canvas.pushSprite(0, 0);
            while (1) delay(1000);
        }
        Serial.printf("Disk OK: %s (%lu B)\r\n", rkfile, (unsigned long)test.size());
        test.close();
    }

    // Switch display to terminal canvas
    canvas.fillSprite(0);
    canvas.setCursor(0, 0);
    canvas.setTextColor(1, 0);
    canvas.printf("Booting %s...\r\n", selName);
    canvas.pushSprite(0, 0);

    // Hand off to emulator — never returns
    startup(rkfile, rlfile, bootdev);
}

// loop() is never reached — startup() contains its own infinite loop.
// M5Cardputer.update() is called inside avr11.cpp's poll cycle.
void loop() {}
