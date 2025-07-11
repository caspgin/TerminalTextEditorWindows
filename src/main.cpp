

/*** INCLUDES ***/
#include <Windows.h>
#include <consoleapi.h>
#include <consoleapi2.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <winbase.h>
#include <wincontypes.h>
#include <winnt.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

/*** DEFINES ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** DATA ***/

struct EditorConfig {
    DWORD ogInputMode;
    DWORD ogOutputMode;
    HANDLE hInput;
    HANDLE hOutput;
    SHORT bWidth;
    SHORT bHeight;
};

struct EditorConfig EC;

/*** TERMINAL ***/
void die(LPCSTR lpMessage) {
    std::string clearScreen = "\x1b[2J\x1b[H";

    std::cerr << clearScreen << "Error: " << lpMessage
              << " ErrorCode: " << GetLastError() << std::endl;
    exit(1);
}

void disableRawMode() {
    SetConsoleMode(EC.hInput, EC.ogInputMode);
    SetConsoleMode(EC.hOutput, EC.ogOutputMode);
}

void enableRawMode() {
    // Turn off echo
    if (!GetConsoleMode(EC.hInput, &EC.ogInputMode) ||
        !GetConsoleMode(EC.hOutput, &EC.ogOutputMode)) {
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
    if (!SetConsoleMode(EC.hInput, rawInputMode) ||
        !SetConsoleMode(EC.hOutput, rawOutputMode)) {
        die("Could not set Console Mode");
    }
}

void exitSucces() {
    DWORD charactersWritten;

    std::string outputBuffer = "\x1b[2J\x1b[H";

    if (!WriteConsole(EC.hOutput, outputBuffer.data(), outputBuffer.size(),
                      &charactersWritten, NULL)) {
        die("Write");
    }
    exit(0);
}

void getWindowSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(EC.hOutput, &csbi)) {
        die("Screen Buffer Info not available");
    }

    EC.bWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    EC.bHeight = csbi.srWindow.Top - csbi.srWindow.Bottom + 1;
}

/*** OUTPUT ***/

void editorDrawRows(std::string &outputBuffer) {
    int y;
    for (y = 0; y < EC.bHeight; y++) {
        outputBuffer += "~\r\n";
    }
}

void editorRefreshScreen() {
    DWORD charactersWritten;

    std::string outputBuffer = "\x1b[2J\x1b[H";
    editorDrawRows(outputBuffer);
    outputBuffer += "\x1b[H";

    if (!WriteConsole(EC.hOutput, outputBuffer.data(), outputBuffer.size(),
                      &charactersWritten, NULL)) {
        die("Write");
    }
}

/*** INPUT ***/

char editorReadKey() {
    char c;
    DWORD bytesRead;
    while (!ReadFile(EC.hInput, &c, sizeof(c), &bytesRead, NULL)) {
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
void initStdHandles() {
    EC.hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (EC.hInput == INVALID_HANDLE_VALUE) {
        die("Could not get standard input handle.");
    }
    EC.hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (EC.hOutput == INVALID_HANDLE_VALUE) {
        die("Could not get standard output handle.");
    }
}
void initEditor() { getWindowSize(); }

int main() {
    initStdHandles();
    enableRawMode();
    initEditor();
    while (true) {
        editorRefreshScreen();
        editorProcessKeyPress();
    }
    return 0;
}
