import re

with open("src/options.cpp", "r") as f:
    content = f.read()

# We need to find the openMonitorMode function
start_idx = content.find("void openMonitorMode() {")
if start_idx == -1:
    print("Could not find openMonitorMode")
    exit(1)

# Find the end of the while (true) loop
end_idx = content.find("}\n\n", start_idx)

monitor_code = content[start_idx:end_idx]

# 1. Replace canvas.print with out_print
monitor_code = monitor_code.replace("canvas.print(", "out_print(")
# 2. Replace canvas.printf with out_printf
monitor_code = monitor_code.replace("canvas.printf(", "out_printf(")

# 3. Add the lambda functions at the top of openMonitorMode
lambdas = """void openMonitorMode() {
    extern M5Canvas canvas;
    extern void flush_console();
    
    auto out_print = [&](const String& s) {
        canvas.print(s);
        Serial.print(s);
    };
    auto out_print_c = [&](char c) {
        canvas.print(c);
        Serial.print(c);
    };
    auto out_printf = [&](const char* fmt, ...) {
        char buf[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        canvas.print(buf);
        Serial.print(buf);
    };
    
    // Fix char overload
    #define out_print(x) _Generic((x), char: out_print_c(x), default: out_print(x))
"""

# Wait, _Generic is C11, not C++. Let's just use C++ function overloading inside a struct.
lambdas_cpp = """void openMonitorMode() {
    extern M5Canvas canvas;
    extern void flush_console();
    
    struct Out {
        void print(const String& s) { canvas.print(s); Serial.print(s); }
        void print(char c) { canvas.print(c); Serial.print(c); }
        void print(int i) { canvas.print(i); Serial.print(i); }
        void printf(const char* fmt, ...) {
            char buf[128];
            va_list args;
            va_start(args, fmt);
            vsnprintf(buf, sizeof(buf), fmt, args);
            va_end(args);
            canvas.print(buf);
            Serial.print(buf);
        }
    } out;
"""

monitor_code = monitor_code.replace("void openMonitorMode() {\n    extern M5Canvas canvas;\n    extern void flush_console();", lambdas_cpp)
monitor_code = monitor_code.replace("out_print(", "out.print(")
monitor_code = monitor_code.replace("out_printf(", "out.printf(")

# 4. Modify the input loop to read from Serial too
input_loop_old = """        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
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
            if (status.enter) chars.push_back('\\r');"""

input_loop_new = """        bool has_chars = false;
        bool esc_pressed = false;
        std::vector<char> chars;
        
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            has_chars = true;
            auto status = M5Cardputer.Keyboard.keysState();
            esc_pressed = status.del;
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
            if (status.enter) chars.push_back('\\r');
        }
        
        while (Serial.available()) {
            has_chars = true;
            char c = Serial.read();
            if (c == 27) esc_pressed = true;
            else chars.push_back(c);
        }

        if (has_chars) {"""

monitor_code = monitor_code.replace(input_loop_old, input_loop_new)

content = content[:start_idx] + monitor_code + content[end_idx:]

with open("src/options.cpp", "w") as f:
    f.write(content)

print("Patch applied")
