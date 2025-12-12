#include "Terminal.h"

#include <consoleapi.h>
#include <handleapi.h>
#include <winbase.h>

#include "config.h"

Terminal::Terminal() : windowWidth(0), windowHeight(0) {
    inputHandle = GetStdHandle(STD_INPUT_HANDLE);
    outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!gotHandlesSuccessfully()) {
        // throw Error
    }
    if (!enableRawMode()) {
        // throw Error
    }
    if (!getWindowSize()) {
        // Throw error
    }
}

Terminal::~Terminal() { disableRawMode(); }

SHORT Terminal::getScreenWidth() const { return windowWidth; }
SHORT Terminal::getScreenHeight() const { return windowHeight; }

bool Terminal::gotHandlesSuccessfully() {
    if (inputHandle == INVALID_HANDLE_VALUE ||
        outputHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    return true;
}

bool Terminal::enableRawMode() {
    if (!GetConsoleMode(inputHandle, &originalInputMode) ||
        !GetConsoleMode(outputHandle, &originalOutputMode)) {
        return false;
    }
    DWORD rawInputMode = 0;
    rawInputMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    rawInputMode |= ENABLE_INSERT_MODE;
    rawInputMode |= ENABLE_WINDOW_INPUT;

    DWORD rawOutputMode = 0;
    rawOutputMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    rawOutputMode |= ENABLE_PROCESSED_OUTPUT;
    if (!SetConsoleMode(inputHandle, rawInputMode) ||
        !SetConsoleMode(outputHandle, rawOutputMode)) {
        return false;
    }

    return true;
}

void Terminal::disableRawMode() {
    SetConsoleMode(inputHandle, originalInputMode);
    SetConsoleMode(outputHandle, originalOutputMode);
}

bool Terminal::getWindowSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    bool gotScreenInfo = GetConsoleScreenBufferInfo(outputHandle, &csbi);
    if (!gotScreenInfo) {
        return false;
    }
    windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    return true;
}

bool Terminal::write(std::string& outputBuffer) {
    DWORD charactersWritten;
    bool success = WriteConsole(outputHandle, outputBuffer.data(),
                                outputBuffer.size(), &charactersWritten, NULL);
    return success;
}

int Terminal::readKey() {
    char characterRead;
    DWORD bytesRead;

    if (WaitForSingleObject(inputHandle, 0) != WAIT_OBJECT_0) {
        return static_cast<int>(AppConfig::editorKey::NO_KEY);
    }

    if (!ReadFile(inputHandle, &characterRead, sizeof(characterRead),
                  &bytesRead, NULL)) {
        // editorSetStatusMessage("Error in Reading.");
        return static_cast<int>(AppConfig::editorKey::NO_KEY);
    }

    if (characterRead == '\r') {
        return static_cast<int>(AppConfig::editorKey::CARRIAGE);
    }

    if (characterRead == '\x1b') {
        char seq[3];
        if (WaitForSingleObject(inputHandle, 0) != WAIT_OBJECT_0) {
            return static_cast<int>(AppConfig::editorKey::ESCAPE);
        }
        if (!ReadFile(inputHandle, &seq[0], sizeof(seq[0]), &bytesRead, NULL)) {
            // editorSetStatusMessage("Error in Reading");
            return static_cast<int>(AppConfig::editorKey::NO_KEY);
        }
        if (WaitForSingleObject(inputHandle, 0) != WAIT_OBJECT_0) {
            return static_cast<int>(AppConfig::editorKey::UNKNOWN_KEY);
        }
        if (!ReadFile(inputHandle, &seq[1], sizeof(seq[1]), &bytesRead, NULL)) {
            // editorSetStatusMessage("Error in Reading");
            return static_cast<int>(AppConfig::editorKey::UNKNOWN_KEY);
        }

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (WaitForSingleObject(inputHandle, 0) != WAIT_OBJECT_0) {
                    return static_cast<int>(AppConfig::editorKey::UNKNOWN_KEY);
                }
                if (!ReadFile(inputHandle, &seq[2], sizeof(seq[2]), &bytesRead,
                              NULL)) {
                    return static_cast<int>(AppConfig::editorKey::UNKNOWN_KEY);
                }
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1':
                            return static_cast<int>(AppConfig::editorKey::HOME);
                        case '4':
                            return static_cast<int>(AppConfig::editorKey::END);
                        case '3':
                            return static_cast<int>(
                                AppConfig::editorKey::DEL_KEY);
                        case '5':
                            return static_cast<int>(
                                AppConfig::editorKey::PAGE_UP);
                        case '6':
                            return static_cast<int>(
                                AppConfig::editorKey::PAGE_DOWN);
                        case '7':
                            return static_cast<int>(AppConfig::editorKey::HOME);
                        case '8':
                            return static_cast<int>(AppConfig::editorKey::END);
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A':
                        return static_cast<int>(AppConfig::editorKey::ARROW_UP);
                    case 'B':
                        return static_cast<int>(
                            AppConfig::editorKey::ARROW_DOWN);
                    case 'C':
                        return static_cast<int>(
                            AppConfig::editorKey::ARROW_RIGHT);
                    case 'D':
                        return static_cast<int>(
                            AppConfig::editorKey::ARROW_LEFT);
                    case 'H':
                        return static_cast<int>(AppConfig::editorKey::HOME);
                    case 'F':
                        return static_cast<int>(AppConfig::editorKey::END);
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H':
                    return static_cast<int>(AppConfig::editorKey::HOME);
                case 'F':
                    return static_cast<int>(AppConfig::editorKey::END);
            }
        }
        return static_cast<int>(AppConfig::editorKey::ESCAPE);
    }
    return characterRead;
}
