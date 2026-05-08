/*
MIT License

Copyright (c) 2021 Klaus Zerbe

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Ported to M5Stack Cardputer: removed M5Core2.h and virtual button (BtKey).
// The pre-boot menu is now implemented entirely in M5_11_SD.ino using the
// physical keyboard; this file is kept for Serial-based readline only.

#include <stdio.h>
#include <cstdlib>
#include <Arduino.h>
#include "M5Cardputer.h"

const uint startLineLength = 8;
const char eof = 255;

static void serial_putchar(char c) {
    Serial.write(c);
}

static char serial_getchar() {
    while (1) {
        M5Cardputer.update();
        if (Serial.available())
            return Serial.read();
        delay(5);
    }
}

/*
 *  read a line of any length from Serial (grows on heap)
 *
 *  @param fullDuplex input will echo on entry (terminal mode) when true
 *  @param linebreak  defaults to '\n'; use '\r' for raw terminals
 *  @return entered line on heap — caller must free()
 */
char *ReadLine(bool fullDuplex, char lineBreak) {
    char *pStart = (char *)malloc(startLineLength);
    char *pPos   = pStart;
    size_t maxLen = startLineLength;
    size_t len    = maxLen;
    int c;

    if (!pStart)
        return NULL;

    while (1) {
        c = serial_getchar();
        if (c == 255) continue;
        if (c == eof || c == lineBreak) break;

        if (fullDuplex)
            serial_putchar(c);

        if (--len == 0) {
            len = maxLen;
            char *pNew = (char *)realloc(pStart, maxLen *= 2);
            if (!pNew) {
                free(pStart);
                return NULL;
            }
            pPos   = pNew + (pPos - pStart);
            pStart = pNew;
        }

        if ((*pPos++ = c) == lineBreak)
            break;
    }

    *pPos = '\0';
    return pStart;
}