

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

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

/*** DEFINES ***/
#define CTRL_KEY(k) ((k) & 0x1f)
#define KILO_VERSION 1.0
#define TAB_STOP 4
/*** DATA ***/

struct MessageHistory {
    std::vector<std::string> messages;
};

struct EditorConfig {
    DWORD ogInputMode;
    DWORD ogOutputMode;
    HANDLE hInput;
    HANDLE hOutput;
    SHORT bWidth;
    SHORT bHeight;
    DWORD rowoff;
    DWORD coloff;
    DWORD cx, cy, rx;
    std::string fileName;
    std::vector<std::string> row;
    std::vector<std::string> render;
    std::time_t statusmsg_time;
};

struct EditorConfig EC;
struct MessageHistory MH;
enum editorKey {
    ESCAPE = 27,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN,
    HOME,
    END,
    DEL_KEY,
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

    if (!WriteConsole(EC.hOutput, outputBuffer.data(),
                      (DWORD)outputBuffer.size(), &charactersWritten, NULL)) {
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
DWORD getRowSize() {
    return (EC.cy >= EC.row.size()) ? 0 : (DWORD)EC.row[EC.cy].size();
}
/*** ROW OPERATIONS ***/

DWORD editorRowCxToRx(DWORD rowNumber, DWORD cx) {
    DWORD rx = 0;
    std::string &row = EC.row[rowNumber];

    for (int j = 0; j < cx; j++) {
        if (row[j] == '\t') rx += (TAB_STOP - 1) - (rx % TAB_STOP);
        rx++;
    }

    return rx;
}

void editorUpdateRow(DWORD rowNumber) {
    while (rowNumber >= EC.render.size()) {
        EC.render.push_back("");
    }
    std::string &row = EC.row[rowNumber];
    std::string &renderRow = EC.render[rowNumber];
    renderRow.clear();
    for (auto itr = row.begin(); itr != row.end(); itr++) {
        if (*itr == '\t') {
            renderRow += ' ';
            while (renderRow.size() % TAB_STOP != 0) {
                renderRow += ' ';
            }

        } else {
            renderRow += *itr;
        }
    }
}

void editorAppendRow(const std::string &newRow) {
    EC.row.push_back(newRow);
    editorUpdateRow(EC.row.size() - 1);
}

void editorRowInsertChar(DWORD rowNumber, DWORD at, char c) {
    std::string &row = EC.row[rowNumber];
    if (at > row.size()) at = row.size();
    row.insert(row.begin() + at, c);
    editorUpdateRow(rowNumber);
}

void editorInsertChar(char c) {
    if (EC.cy == EC.row.size()) {
        editorAppendRow("");
    }
    editorRowInsertChar(EC.cy, EC.cx, c);
    EC.cx++;
}

/*** FILE I/O ***/

void editorOpen(const std::string filePath) {
    EC.fileName = std::filesystem::path(filePath).filename().string();
    std::ifstream infile(filePath);
    if (!infile.is_open()) {
        die("File not opened");
    }
    std::string line;
    while (std::getline(infile, line)) {
        while (!line.empty() &&
               (line[line.size() - 1] == '\r' || line[line.size() - 1] == '\n'))
            line.pop_back();
        editorAppendRow(line);
    }
    infile.close();
}

/*** APPEND BUFFER ***/

void abAppend(std::string &apBuf, const std::string &appendData) {
    apBuf += appendData;
}

/*** OUTPUT ***/

void editorSetStatusMessage(const std::string fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int length = vsnprintf(NULL, 0, fmt.c_str(), args);
    va_end(args);
    if (length < 0) {
        die("status Message length negative");
    }
    std::vector<char> buffer(length + 1);
    va_list args2;
    va_start(args2, fmt);
    vsnprintf(buffer.data(), buffer.size(), fmt.c_str(), args2);
    va_end(args2);

    MH.messages.push_back(buffer.data());
    EC.statusmsg_time = time(NULL);
}

void clearLine(std::string &outputBuffer) { abAppend(outputBuffer, "\x1b[K"); }

void editorDrawRows(std::string &outputBuffer) {
    int y;
    for (y = 0; y < EC.bHeight; y++) {
        int fileRow = y + EC.rowoff;
        if (fileRow < EC.render.size()) {
            int actualSize = (int)(EC.render[fileRow].size() - EC.coloff);
            int sizeToPrint = actualSize < EC.bWidth ? actualSize : EC.bWidth;
            if (sizeToPrint > 0) {
                abAppend(outputBuffer,
                         EC.render[fileRow].substr(EC.coloff, sizeToPrint));
            }
        } else if (EC.row.size() == 0) {
            abAppend(outputBuffer, "~");
            if (y == EC.bHeight / 2) {
                int midWidth = EC.bWidth / 2;
                std::string welcome = "TTE editor windows -- version " +
                                      std::to_string(KILO_VERSION);
                int distance = midWidth - (welcome.size() / 2);
                welcome = std::string(distance, ' ') + welcome;
                abAppend(outputBuffer, welcome);
            }
        } else {
            abAppend(outputBuffer, "~");
        }
        clearLine(outputBuffer);
        abAppend(outputBuffer, "\r\n");
    }
}

void editorDrawStatusBar(std::string &outputBuffer) {
    const DWORD MAX_FILENAME_SIZE = 20;
    EC.fileName = EC.fileName.empty() ? "[No Name]" : EC.fileName;
    std::string initalSpacer = " ";

    std::string fileNamePart = EC.fileName.size() > MAX_FILENAME_SIZE
                                   ? EC.fileName.substr(0, MAX_FILENAME_SIZE)
                                   : EC.fileName;

    int scrollPercent = ((EC.cy + 1) / (float)EC.row.size()) * 100;

    std::string fileLocationStatus = std::to_string(scrollPercent) + "%   " +
                                     std::to_string(EC.cy + 1) + ":" +
                                     std::to_string(EC.cx + 1) + " ";

    int len =
        initalSpacer.size() + fileNamePart.size() + fileLocationStatus.size();
    while (len < EC.bWidth) {
        fileNamePart += " ";
        len++;
    }

    abAppend(outputBuffer, "\x1b[7m" + initalSpacer + fileNamePart +
                               fileLocationStatus + "\x1b[m" + "\r\n");
}

void editorDrawMessageBar(std::string &outputBuffer) {
    clearLine(outputBuffer);
    std::string lastMessage =
        MH.messages.size() > 0 ? *(MH.messages.end() - 1) : "";
    if (lastMessage.size() && time(NULL) - EC.statusmsg_time < 5) {
        abAppend(outputBuffer, lastMessage.size() >= EC.bWidth - 1
                                   ? " " + lastMessage.substr(0, EC.bWidth - 1)
                                   : " " + lastMessage);
    }
}

void editorScroll() {
    EC.rx = 0;
    if (EC.cy < EC.row.size()) {
        EC.rx = editorRowCxToRx(EC.cy, EC.cx);
    }

    if (EC.cy < EC.rowoff) {
        EC.rowoff = EC.cy;
    }

    if (EC.cy >= EC.bHeight + EC.rowoff) {
        EC.rowoff = EC.cy - EC.bHeight + 1;
    }

    if (EC.rx < EC.coloff) {
        EC.coloff = EC.rx;
    }

    if (EC.rx >= EC.bWidth + EC.coloff) {
        EC.coloff = EC.rx - EC.bWidth + 1;
    }
}

void editorRefreshScreen() {
    editorScroll();
    DWORD charactersWritten;

    std::string apBuf;
    abAppend(apBuf, "\x1b[?25l");
    abAppend(apBuf, "\x1b[H");
    editorDrawRows(apBuf);
    editorDrawStatusBar(apBuf);
    editorDrawMessageBar(apBuf);
    int vcy = EC.cy - EC.rowoff + 1;
    int vcx = EC.rx - EC.coloff + 1;

    char buf[32];
    std::snprintf(buf, sizeof(buf), "\x1b[%d;%dH", vcy, vcx);
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
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (!ReadFile(EC.hInput, &seq[2], sizeof(seq[2]), &bytesRead,
                              NULL)) {
                    return ESCAPE;
                }
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1':
                            return HOME;
                        case '4':
                            return END;
                        case '3':
                            return DEL_KEY;
                        case '5':
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                        case '7':
                            return HOME;
                        case '8':
                            return END;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A':
                        return ARROW_UP;
                    case 'B':
                        return ARROW_DOWN;
                    case 'C':
                        return ARROW_RIGHT;
                    case 'D':
                        return ARROW_LEFT;
                    case 'H':
                        return HOME;
                    case 'F':
                        return END;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H':
                    return HOME;
                case 'F':
                    return END;
            }
        }
        return ESCAPE;
    }
    return c;
}

void editorMoveCursor(int key) {
    DWORD rowSize = getRowSize();
    switch (key) {
        case ARROW_LEFT:
            if (EC.cx != 0) {
                EC.cx--;
            } else {
                editorMoveCursor(ARROW_UP);
                rowSize = getRowSize();
                EC.cx = rowSize > 0 ? rowSize - 1 : 0;
            }
            break;
        case ARROW_RIGHT:
            if (rowSize > 0 && EC.cx < rowSize - 1) {
                EC.cx++;
            } else {
                editorMoveCursor(ARROW_DOWN);
                if (EC.cx != 0) EC.cx = 0;
            }
            break;
        case ARROW_UP:
            if (EC.cy != 0) {
                EC.cy--;
            }
            break;
        case ARROW_DOWN:
            if (EC.cy < EC.row.size()) {
                EC.cy++;
            }
            break;
        case PAGE_UP: {
            int times = EC.bHeight;
            for (; times > 0; times--) {
                editorMoveCursor(ARROW_UP);
            }
        } break;
        case PAGE_DOWN: {
            int times = EC.bHeight;
            for (; times > 0; times--) {
                editorMoveCursor(ARROW_DOWN);
            }
        } break;
        case HOME:
            EC.cx = 0;
            break;
        case END:
            if (EC.cy < EC.row.size()) {
                EC.cx = EC.row[EC.cy].size();
            }
            break;
    }
    rowSize = getRowSize();
    if (EC.cx > rowSize) {
        EC.cx = rowSize > 0 ? rowSize - 1 : 0;
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
        case PAGE_UP:
        case PAGE_DOWN:
        case HOME:
        case END:
            editorMoveCursor(key);
            break;
        case DEL_KEY:
            // TODO Implement Delete
            break;
        default:
            editorInsertChar(key);
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
    EC.rowoff = EC.coloff = 0;
    EC.fileName = "";
    EC.statusmsg_time = 0;
    getWindowSize();
    EC.bHeight -= 1;  // Setting second last row for Status Bar
    EC.bHeight -= 1;  // Setting last row for Message Bar
}

int main(int argc, char *argv[]) {
    initStdHandles();
    enableRawMode();
    initEditor();
    if (argc >= 2) {
        editorOpen(argv[1]);
    }
    editorSetStatusMessage("HELP: Ctrl-Q = quit");
    while (true) {
        editorRefreshScreen();
        editorProcessKeyPress();
    }
    return 0;
}
