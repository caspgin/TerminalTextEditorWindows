
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

    DWORD rawMode = originalMode;
    rawMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    rawMode &= ~ENABLE_ECHO_INPUT;
    rawMode &= ~ENABLE_LINE_INPUT;
    rawMode &= ~ENABLE_PROCESSED_INPUT;
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

    char c;
    DWORD bytesRead;
    while (ReadFile(hInput, &c, sizeof(c), &bytesRead, NULL) && c != 'q') {
        if (bytesRead > 0) {
            std::cout << "\n\nbytes Read are :" << bytesRead << "\n";
        }
        if (std::iscntrl(c)) {
            std::cout << (int)c << "\n";
        } else {
            std::cout << "character: " << c << "\t" << (int)c << "\n";
        }
    }
    return 0;
}
