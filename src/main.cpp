

/*** INCLUDES ***/
#include <Windows.h>
#include <consoleapi.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <winbase.h>
#include <winnt.h>

#include <cstdlib>
#include <iostream>
#include <string>

/*** DEFINES ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** DATA ***/
DWORD ogInputMode;
DWORD ogOutputMode;
HANDLE hInput;
HANDLE hOutput;

/*** TERMINAL ***/
void die(LPCSTR lpMessage) {
    std::string clearScrrenSequnce = "\x1b[2J\x1b[H";

    std::cerr << clearScrrenSequnce << "Error: " << lpMessage
              << " ErrorCode: " << GetLastError() << std::endl;
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

void exitSucces() {
    DWORD charactersWritten;

    std::string outputBuffer = "\x1b[2J\x1b[H";

    if (!WriteConsole(hOutput, outputBuffer.data(), outputBuffer.size(),
                      &charactersWritten, NULL)) {
        die("Write");
    }
    exit(0);
}

/*** OUTPUT ***/

void editorDrawRows(std::string &outputBuffer) {
    int y;
    for (y = 0; y < 24; y++) {
        outputBuffer += "~\r\n";
    }
}

void editorRefreshScreen() {
    DWORD charactersWritten;

    std::string outputBuffer = "\x1b[2J\x1b[H";
    editorDrawRows(outputBuffer);
    outputBuffer += "\x1b[H";

    if (!WriteConsole(hOutput, outputBuffer.data(), outputBuffer.size(),
                      &charactersWritten, NULL)) {
        die("Write");
    }
}

/*** INPUT ***/

char editorReadKey() {
    char c;
    DWORD bytesRead;
    while (!ReadFile(hInput, &c, sizeof(c), &bytesRead, NULL)) {
        die("read");
    }
    return c;
}

void editorProcessKeyPress() {
    char key = editorReadKey();

    switch (key) {
        case CTRL_KEY('q'):
            exitSucces();
            break;
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

    while (true) {
        editorRefreshScreen();
        editorProcessKeyPress();
    }
    return 0;
}
