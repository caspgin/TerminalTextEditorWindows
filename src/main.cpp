

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

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
/*** DEFINES ***/
#define CTRL_KEY(k) ((k) & 0x1f)
#define KILO_VERSION 1.0
/*** DATA ***/

struct EditorConfig {
    DWORD ogInputMode;
    DWORD ogOutputMode;
    HANDLE hInput;
    HANDLE hOutput;
    SHORT bWidth;
    SHORT bHeight;
    DWORD cx, cy;
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
    EC.bHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

/*** APPEND BUFFER ***/

std::string apBuf;

void abAppend(std::string &apBuf, const std::string &appendData) {
    apBuf += appendData;
}

/*** OUTPUT ***/

void editorDrawRows(std::string &outputBuffer) {
    int y;
    for (y = 0; y < EC.bHeight; y++) {
        abAppend(outputBuffer, "~");
        if (y == EC.bHeight / 2) {
            int midWidth = EC.bWidth / 2;
            std::string welcome =
                "TTE editor windows -- version " + std::to_string(KILO_VERSION);
            int distance = midWidth - (welcome.size() / 2);
            welcome = std::string(distance, ' ') + welcome;
            abAppend(outputBuffer, welcome);
        }

        abAppend(outputBuffer, "\x1b[K");
        if (y < EC.bHeight - 1) {
            abAppend(outputBuffer, "\r\n");
        }
    }
}

void editorRefreshScreen() {
    DWORD charactersWritten;

    abAppend(apBuf, "\x1b[?25l");
    abAppend(apBuf, "\x1b[H");

    editorDrawRows(apBuf);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (int)EC.cy + 1,
                  (int)EC.cx + 1);
    abAppend(apBuf, buf);
    abAppend(apBuf, "\x1b[?25h");
    if (!WriteConsole(EC.hOutput, apBuf.data(), apBuf.size(),
                      &charactersWritten, NULL)) {
        die("Write");
    }
    apBuf.clear();
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
void editorMoveCursor(char key) {
    switch (key) {
        case 'a':
            EC.cx--;
            break;
        case 'd':
            EC.cx++;

            break;
        case 'w':
            EC.cy--;
            break;
        case 's':
            EC.cy++;
            break;
    }
}

void editorProcessKeyPress() {
    char key = editorReadKey();
    switch (key) {
        case CTRL_KEY('q'):
            exitSucces();
            break;
        case '\x1b':
            writeToBuffer(key);
            break;
        case 'w':
        case 's':
        case 'a':
        case 'd':
            editorMoveCursor(key);
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
void initEditor() {
    EC.cx = 0;
    EC.cy = 0;
    getWindowSize();
}

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
