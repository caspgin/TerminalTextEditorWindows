

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
    std::string debugMsg;
};

struct EditorConfig EC;

enum editorKey {
    ESCAPE = 27,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
};

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

void clearLine(std::string &outputBuffer) { abAppend(outputBuffer, "\x1b[K"); }

void editorDrawRows(std::string &outputBuffer) {
    int y;
    for (y = 0; y < EC.bHeight; y++) {
        if (y < EC.bHeight - 1) {
            abAppend(outputBuffer, "~");
        }
        if (y == EC.bHeight / 2) {
            int midWidth = EC.bWidth / 2;
            std::string welcome =
                "TTE editor windows -- version " + std::to_string(KILO_VERSION);
            int distance = midWidth - (welcome.size() / 2);
            welcome = std::string(distance, ' ') + welcome;
            abAppend(outputBuffer, welcome);
        }
        if (y == EC.bHeight - 1) {
            outputBuffer += EC.debugMsg;
        }
        clearLine(outputBuffer);
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

int editorReadKey() {
    char c;
    DWORD bytesRead;
    while (!ReadFile(EC.hInput, &c, sizeof(c), &bytesRead, NULL)) {
        die("read");
    }
    if (c == '\x1b') {
        char seq[3];
        if (!ReadFile(EC.hInput, &seq[0], sizeof(seq[0]), &bytesRead, NULL)) {
            return ESCAPE;
        }
        if (!ReadFile(EC.hInput, &seq[1], sizeof(seq[1]), &bytesRead, NULL)) {
            return ESCAPE;
        }

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
            }
        }
        return ESCAPE;
    }
    return c;
}
void editorMoveCursor(int key) {
    switch (key) {
        case ARROW_LEFT:
            if (EC.cx != 0) {
                EC.cx--;
            }
            break;
        case ARROW_RIGHT:
            if (EC.cx != EC.bWidth - 1) {
                EC.cx++;
            }
            break;
        case ARROW_UP:
            if (EC.cy != 0) {
                EC.cy--;
            }
            break;
        case ARROW_DOWN:
            if (EC.cy != EC.bHeight - 1) {
                EC.cy++;
            }
            break;
    }
}

void editorProcessKeyPress() {
    int key = editorReadKey();
    switch (key) {
        case CTRL_KEY('q'):
            exitSucces();
            break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case ARROW_LEFT:
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
    EC.debugMsg = "";
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
