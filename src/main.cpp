

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
#include <iomanip>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "llm_api_client.h"
/*** DEFINES ***/
#define CTRL_KEY(k) ((k) & 0x1f)
#define KILO_VERSION 1.0
#define TAB_STOP 4
#define SIDE_PANEL_WIDTH 5
/*** FUNCTION DECLARATION ***/

void editorSetStatusMessage(const std::string fmt, ...);
std::string editorPrompt(std::string prompt);

/*** DATA ***/

struct MessageHistory {
    std::vector<std::string> messages;
};

struct EditorConfig {
    DWORD ogInputMode;
    DWORD ogOutputMode;
    HANDLE hInput;
    HANDLE hOutput;
    SHORT screenWidth;
    SHORT screenHeight;
    DWORD rowoff;
    DWORD coloff;
    DWORD cx, cy, rx;
    std::filesystem::path filePath;
    DWORD dirty;
    std::vector<std::string> row;
    std::vector<std::string> render;
    std::time_t statusmsg_time;
};

struct EditorConfig EC;
struct MessageHistory MH;
enum editorKey {
    CARRIAGE = 13,
    ESCAPE = 27,
    BACKSPACE = 127,
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

    EC.screenWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    EC.screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}
DWORD getRowSize() {
    return (EC.cy >= EC.row.size()) ? 0 : (DWORD)EC.row[EC.cy].size();
}
/*** ROW OPERATIONS ***/

DWORD editorRowCxToRx(DWORD rowNumber, DWORD cx) {
    DWORD rx = 0;
    std::string &row = EC.row[rowNumber];

    for (DWORD j = 0; j < cx; j++) {
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

void editorInsertRow(DWORD insertAt, const std::string &newRow) {
    if (insertAt < 0 || insertAt > EC.row.size()) return;

    EC.row.insert(EC.row.begin() + insertAt, newRow);
    EC.render.insert(EC.render.begin() + insertAt, "");
    editorUpdateRow(insertAt);
    EC.dirty++;
}

void editorRowInsertChar(DWORD rowNumber, DWORD at, char c) {
    if (rowNumber >= EC.row.size()) {
        return;
    }
    std::string &row = EC.row[rowNumber];
    if (at > row.size()) at = (DWORD)row.size();
    row.insert(row.begin() + at, c);
    editorUpdateRow(rowNumber);
    EC.dirty++;
}

void editorRowRemoveChar(DWORD rowNumber, DWORD at) {
    std::string &row = EC.row[rowNumber];
    if (at >= row.size()) return;
    row.erase(row.begin() + at);
    editorUpdateRow(rowNumber);
    EC.dirty++;
}

void editorInsertChar(char c) {
    if (EC.cy == EC.row.size()) {
        editorInsertRow(EC.cy, "");
    }
    editorRowInsertChar(EC.cy, EC.cx, c);
    EC.cx++;
}

void editorRowAppendString(DWORD targetRowToAppend,
                           std::string &stringToAppend) {
    EC.row[targetRowToAppend] += stringToAppend;
    editorUpdateRow(targetRowToAppend);
    EC.dirty++;
}

void editorRowDelete(DWORD rowToDel) {
    if (rowToDel >= EC.row.size() || rowToDel >= EC.render.size()) return;

    EC.row.erase(EC.row.begin() + rowToDel);
    EC.render.erase(EC.render.begin() + rowToDel);

    EC.dirty++;
}

void editorRemoveChar() {
    if (EC.cy == EC.row.size() || EC.row.size() == 0) return;
    if (EC.cx == 0 && EC.cy == 0) return;
    if (EC.cx > 0) {
        editorRowRemoveChar(EC.cy, EC.cx - 1);
        EC.cx--;
    } else {
        EC.cx = (DWORD)EC.row[EC.cy - 1].size();
        editorRowAppendString(EC.cy - 1, EC.row[EC.cy]);
        editorRowDelete(EC.cy);
        EC.cy--;
    }
}

void editorInsertNewLine() {
    if (EC.cx == 0) {
        editorInsertRow(EC.cy, "");
    } else {
        editorInsertRow(EC.cy + 1, EC.row[EC.cy].substr(EC.cx));
        editorInsertRow(EC.cy + 1, EC.row[EC.cy].substr(0, EC.cx));
        editorRowDelete(EC.cy);
    }
    EC.cx = 0;
    EC.cy++;
}

/*** FILE I/O ***/

void editorOpen(const std::string filePath) {
    EC.filePath = std::filesystem::absolute(std::filesystem::path(filePath));
    std::ifstream infile(EC.filePath);
    if (!infile.is_open()) {
        die("File not opened");
    }
    std::string line;
    while (std::getline(infile, line)) {
        while (!line.empty() &&
               (line[line.size() - 1] == '\r' || line[line.size() - 1] == '\n'))
            line.pop_back();
        editorInsertRow(EC.row.size(), line);
    }
    infile.close();
    EC.dirty = 0;
}

void editorRowsToString(std::string &data) {
    for (auto itr = EC.row.begin(); itr != EC.row.end(); itr++) {
        data += *itr;
        data += "\r\n";
    }
}

void editorSave() {
    if (EC.filePath.empty()) {
        EC.filePath = editorPrompt("Save as: %s");
        if (EC.filePath.empty()) {
            editorSetStatusMessage("Save aborted");
            return;
        }
    }

    std::string data;
    editorRowsToString(data);

    std::ofstream outfile(EC.filePath);
    if (!outfile.is_open()) {
        editorSetStatusMessage("File NOT saved. File would not open.");
    }

    outfile << data;
    outfile.close();
    EC.dirty = 0;
    editorSetStatusMessage("Wrote %d bytes to %s.", data.size(),
                           EC.filePath.filename().string().c_str());
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

void editorDrawSidePanel(std::string &outputBuffer, const int lineNumber) {
    std::stringstream ss;

    ss << std::setw(SIDE_PANEL_WIDTH - 1) << std::right << std::setfill(' ')
       << lineNumber;
    abAppend(outputBuffer, "\x1b[48;2;31;40m");
    abAppend(outputBuffer, ss.str() + ' ');
    abAppend(outputBuffer, "\x1b[0m");
}
void editorDrawRows(std::string &outputBuffer) {
    int y;
    for (y = 0; y < EC.screenHeight; y++) {
        int fileRow = y + EC.rowoff;
        if (EC.render.size() == 0 && fileRow == 0) {
            editorDrawSidePanel(outputBuffer, 1);
        }
        if (fileRow < EC.render.size()) {
            editorDrawSidePanel(outputBuffer, fileRow + 1);
            int actualSize = (int)(EC.render[fileRow].size() - EC.coloff);
            int sizeToPrint =
                actualSize < EC.screenWidth ? actualSize : EC.screenWidth;
            if (sizeToPrint > 0) {
                abAppend(outputBuffer,
                         EC.render[fileRow].substr(EC.coloff, sizeToPrint));
            }
        } else if (EC.row.size() == 0 && fileRow != 0) {
            abAppend(outputBuffer, "~");
            if (y == EC.screenHeight / 2) {
                int midWidth = EC.screenWidth / 2;
                std::string welcome = "TTE editor windows -- version " +
                                      std::to_string(KILO_VERSION);
                int distance = midWidth - (welcome.size() / 2);
                welcome = std::string(distance, ' ') + welcome;
                abAppend(outputBuffer, welcome);
            }
        } else {
            if (fileRow != 0) {
                abAppend(outputBuffer, "~");
            }
        }
        clearLine(outputBuffer);
        abAppend(outputBuffer, "\r\n");
    }
}

void editorDrawStatusBar(std::string &outputBuffer) {
    const DWORD MAX_FILENAME_SIZE = 20;
    std::string fileName =
        EC.filePath.empty() ? "[No Name]" : EC.filePath.filename().string();
    std::string initalSpacer = " ";

    fileName = fileName.size() > MAX_FILENAME_SIZE
                   ? fileName.substr(0, MAX_FILENAME_SIZE)
                   : fileName;
    fileName += EC.dirty > 0 ? " [+] " : "";

    int scrollPercent = ((EC.cy + 1) / (float)EC.row.size()) * 100;

    std::string fileLocationStatus = std::to_string(scrollPercent) + "%   " +
                                     std::to_string(EC.cy + 1) + ":" +
                                     std::to_string(EC.cx + 1) + " ";

    int len = initalSpacer.size() + fileName.size() + fileLocationStatus.size();
    while (len < EC.screenWidth) {
        fileName += " ";
        len++;
    }

    abAppend(outputBuffer, "\x1b[7m" + initalSpacer + fileName +
                               fileLocationStatus + "\x1b[m" + "\r\n");
}

void editorDrawMessageBar(std::string &outputBuffer) {
    clearLine(outputBuffer);
    std::string lastMessage =
        MH.messages.size() > 0 ? *(MH.messages.end() - 1) : "";
    if (lastMessage.size() && time(NULL) - EC.statusmsg_time < 5) {
        abAppend(outputBuffer,
                 lastMessage.size() >= EC.screenWidth - 1
                     ? " " + lastMessage.substr(0, EC.screenWidth - 1)
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

    if (EC.cy >= EC.screenHeight + EC.rowoff) {
        EC.rowoff = EC.cy - EC.screenHeight + 1;
    }

    if (EC.rx < EC.coloff) {
        EC.coloff = EC.rx;
    }

    if (EC.rx >= EC.screenWidth + EC.coloff) {
        EC.coloff = EC.rx - EC.screenWidth + 1;
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
    int vcx = EC.rx - EC.coloff + 1 + SIDE_PANEL_WIDTH;

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

    if (c == '\r') {
        return CARRIAGE;
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
            int times = EC.screenHeight;
            for (; times > 0; times--) {
                editorMoveCursor(ARROW_UP);
            }
        } break;
        case PAGE_DOWN: {
            int times = EC.screenHeight;
            for (; times > 0; times--) {
                editorMoveCursor(ARROW_DOWN);
            }
        } break;
        case HOME:
            EC.cx = 0;
            break;
        case END:
            if (EC.cy < EC.row.size()) {
                EC.cx = (DWORD)EC.row[EC.cy].size();
            }
            break;
    }
    rowSize = getRowSize();
    if (EC.cx > rowSize) {
        EC.cx = rowSize > 0 ? rowSize - 1 : 0;
    }
}

std::string editorPrompt(std::string promptLabel) {
    std::string prompt = "";
    while (1) {
        editorSetStatusMessage(promptLabel, prompt.c_str());
        editorRefreshScreen();

        int key = editorReadKey();
        if (key == DEL_KEY || key == BACKSPACE) {
            prompt.pop_back();
        } else if (key == '\x1b') {
            editorSetStatusMessage("");
            return "";
        } else if (key == '\r') {
            if (prompt.size() != 0) {
                editorSetStatusMessage("");
                return prompt;
            }
        } else if (!iscntrl(key) && key < 128) {
            prompt += (char)key;
        }
    }
}

void editorAi() {
    std::string prompt = editorPrompt("Enter Prompt: %s");

    if (prompt.empty()) {
        editorSetStatusMessage("Text generation aborted.");
        return;
    }

    try {
        std::string response = CallGemini(prompt);
        std::string line = "";
        for (auto itr = response.begin(); itr != response.end(); itr++) {
            if (*itr == '\n' || *itr == '\r') {
                editorInsertRow(EC.row.size(), line);
                line = "";
                continue;
            }
            line += *itr;
        }
        EC.dirty++;
    } catch (const CustomError &e) {
        editorSetStatusMessage("Error: %s , Message: %s", e.errorType.c_str(),
                               e.what());
        return;
    } catch (const std::exception &e) {
        editorSetStatusMessage("Exception: %s", e.what());
        return;
    }
}

void editorProcessKeyPress() {
    int key = editorReadKey();
    switch (key) {
        case CTRL_KEY('q'):
            exitSucces();
            break;
        case CTRL_KEY('s'):
            editorSave();
            break;
        case CTRL_KEY('a'):
            editorAi();
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
        case BACKSPACE:
            editorRemoveChar();
            break;
        case DEL_KEY:
            editorMoveCursor(ARROW_RIGHT);
            editorRemoveChar();
            break;
        case CARRIAGE:
            editorInsertNewLine();
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
    EC.dirty = 0;
    EC.statusmsg_time = 0;
    getWindowSize();
    EC.screenHeight -= 1;  // Setting second last row for Status Bar
    EC.screenHeight -= 1;  // Setting last row for Message Bar
    EC.screenWidth -= SIDE_PANEL_WIDTH;
}

int main(int argc, char *argv[]) {
    initStdHandles();
    enableRawMode();
    initEditor();
    if (argc >= 2) {
        editorOpen(argv[1]);
    }
    editorSetStatusMessage(
        "HELP: Ctrl-Q = quit | Ctrl-A = Ai text generation | Ctrl-S = "
        "Save/Save as");
    while (true) {
        editorRefreshScreen();
        editorProcessKeyPress();
    }
    return 0;
}
