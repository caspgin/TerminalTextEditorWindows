
#include <Windows.h>
#include <consoleapi.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <winbase.h>

#include <cctype>
#include <cstdlib>
#include <iostream>

DWORD originalMode;
HANDLE hInput;

void disableRawMode() { SetConsoleMode(hInput, originalMode); }

void enableRawMode() {
    // Turn off echo
    GetConsoleMode(hInput, &originalMode);
    std::atexit(disableRawMode);

    DWORD rawMode = 0;
    rawMode |= ENABLE_WINDOW_INPUT;
    rawMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;

    SetConsoleMode(hInput, rawMode);
}

int main() {
    hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hInput == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Could not get standard input handle. Code: "
                  << GetLastError() << std::endl;
        return 1;
    }

    enableRawMode();

    INPUT_RECORD inputRecord;
    DWORD recordRead;
    while (ReadConsoleInput(hInput, &inputRecord, sizeof(inputRecord),
                            &recordRead)) {
        KEY_EVENT_RECORD& keyevent = inputRecord.Event.KeyEvent;
        if (keyevent.bKeyDown) {
            char c = keyevent.uChar.AsciiChar;
            WORD vkCode = keyevent.wVirtualKeyCode;
            DWORD controlState = keyevent.dwControlKeyState;
            bool ctrlPressed =
                (controlState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;

            if (ctrlPressed) {
                std::cout << "Ctrl+" << (char)vkCode << " (VK:" << vkCode
                          << ")\n";

                // Exit on Ctrl+Q
                if (vkCode == 'Q') {
                    break;
                }
            } else if (c != 0) {
                // Regular character
                if (std::iscntrl(c)) {
                    std::cout << "Control char: " << (int)c << "\n";
                } else {
                    std::cout << "Character: " << c << " (" << (int)c << ")\n";
                }

                if (c == 'q') {
                    break;
                }
            } else {
                // Special key (arrow keys, function keys, etc.)
                std::cout << "Special key VK: " << vkCode << "\t"
                          << (char)vkCode << "\n";
            }
        }
    }
    return 0;
}
