

/*** INCLUDES ***/
#include <Windows.h>
#include <consoleapi.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <winbase.h>
#include <winnt.h>

#include <cctype>
#include <cstdlib>
#include <iostream>

/*** DEFINES ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** DATA ***/
DWORD ogInputMode;
DWORD ogOutputMode;
HANDLE hInput;
HANDLE hOutput;

/*** TERMINAL ***/
void die(LPCSTR lpMessage) {
    std::cerr << "Error: " << lpMessage << " ErrorCode: " << GetLastError()
              << std::endl;
    exit(1);
}

void disableRawMode() {
    SetConsoleMode(hInput, ogInputMode);
    SetConsoleMode(hOutput, ogOutputMode);
}

void enableRawMode() {
    // Turn off echo
    if (!GetConsoleMode(hInput, &ogInputMode) ||
        !GetConsoleMode(hOutput, &ogOutputMode)) {
        die("Could not get Console Mode");
    }
    std::atexit(disableRawMode);

    DWORD rawInputMode = 0;
    rawInputMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    rawInputMode |= ENABLE_INSERT_MODE;
    rawInputMode |= ENABLE_WINDOW_INPUT;

    DWORD rawOutputMode = 0;
    rawOutputMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    rawOutputMode |= ENABLE_PROCESSED_OUTPUT;
    if (!SetConsoleMode(hInput, rawInputMode) ||
        !SetConsoleMode(hOutput, rawOutputMode)) {
        die("Could not set Console Mode");
    }
}

/*** INIT ***/
int main() {
    hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hInput == INVALID_HANDLE_VALUE) {
        die("Could not get standard input handle.");
    }
    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOutput == INVALID_HANDLE_VALUE) {
        die("Could not get standard output handle.");
    }

    enableRawMode();

    char c;
    DWORD bytesRead;
    while (true) {
        if (!ReadFile(hInput, &c, sizeof(c), &bytesRead, NULL)) {
            die("read");
        }
        if (bytesRead > 0) {
            std::cout << "\n\nbytes Read are :" << bytesRead << "\n";
        }
        if (std::iscntrl(c)) {
            std::cout << (int)c << "\n";
        } else {
            std::cout << "character: " << c << "\t" << (int)c << "\n";
        }

        if (c == CTRL_KEY('q')) {
            break;
        }
    }
    return 0;
}
