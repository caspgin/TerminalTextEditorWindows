#include "Editor.h"

#include <iomanip>
#include <sstream>

#include "config.h"

Editor::Editor()
    : messageSetTime(0), rowoff(0), coloff(0), editorRunning(false) {
    viewPortWidth = terminal.getScreenWidth() - AppConfig::SIDE_PANEL_WIDTH;
    viewPortHeight =
        terminal.getScreenHeight() - 2;  // status bar and message bar
}

void Editor::run() {
    editorRunning = true;
    while (editorRunning) {
        renderScreen();
        processKeyPress();
    }

    std::string outputBuffer = "\x1b[2J\x1b[H";  // clear screen
    terminal.write(outputBuffer);
}

void Editor::open(const std::string filePath) {
    SaveResult result = activeBuffer.readFile(
        std::filesystem::absolute(std::filesystem::path(filePath)));

    switch (result) {
        case SaveResult::SUCCESS:
            setStatusMessage("File Read successfully!");
            break;
        case SaveResult::FILE_ERROR:
            setStatusMessage("Error reading/opening file");
            break;
        case SaveResult::NO_FILEPATH:
            setStatusMessage("Provided file does not exist");
            break;
        default:
            setStatusMessage("something went wrong");
    }
}

bool Editor::setStatusMessage(const std::string fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int length = vsnprintf(NULL, 0, fmt.c_str(), args);
    va_end(args);
    if (length < 0) {
        return false;
    }
    std::vector<char> buffer(length + 1);
    va_list args2;
    va_start(args2, fmt);
    vsnprintf(buffer.data(), buffer.size(), fmt.c_str(), args2);
    va_end(args2);

    messageHistory.push_back(buffer.data());
    messageSetTime = time(NULL);
    return true;
}

std::string Editor::promptInput(std::string promptLabel) {
    std::string prompt;
    while (true) {
        setStatusMessage(promptLabel, prompt.c_str());
        renderScreen(promptLabel.size() + prompt.size(), viewPortHeight + 2);

        int key = terminal.readKey();

        if (key == static_cast<int>(AppConfig::editorKey::NO_KEY) ||
            key == static_cast<int>(AppConfig::editorKey::UNKNOWN_KEY)) {
            continue;
        } else if (key == static_cast<int>(AppConfig::editorKey::DEL_KEY) ||
                   key == static_cast<int>(AppConfig::editorKey::BACKSPACE)) {
            if (!prompt.empty()) {
                prompt.pop_back();
            }
        } else if (key == static_cast<int>(AppConfig::editorKey::ESCAPE)) {
            setStatusMessage("");
            return "";
        } else if (key == '\r') {
            if (prompt.size() != 0) {
                setStatusMessage("");
                return prompt;
            }
        } else if (!iscntrl(key) && key < 128) {
            prompt += (char)key;
        }
    }
}

void Editor::renderScreen(int overrideCursorX, int overrideCursorY) {
    scrollToCursor();

    std::string apBuf;
    apBuf += "\x1b[?25l";  // Turn cursor off
    apBuf += "\x1b[H";     // Go to Home
    drawRows(apBuf);
    drawStatusBar(apBuf);
    drawMessageBar(apBuf);
    int cursorPosY = activeBuffer.getRenderY() - rowoff + 1;
    int cursorPosX =
        activeBuffer.getRenderX() - coloff + 1 + AppConfig::SIDE_PANEL_WIDTH;

    if (overrideCursorY != -1) {
        cursorPosY = overrideCursorY;
        cursorPosX = overrideCursorX;
    }

    char buf[32];
    std::snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cursorPosY, cursorPosX);
    apBuf += buf;
    apBuf += "\x1b[?25h";  // Turn cursor On

    terminal.write(apBuf);
}

void Editor::drawSidePanel(std::string& outputBuffer, const int lineNumber) {
    std::stringstream ss;

    ss << std::setw(AppConfig::SIDE_PANEL_WIDTH - 1) << std::right
       << std::setfill(' ') << lineNumber;
    outputBuffer += "\x1b[48;2;31;40m";
    outputBuffer += ss.str() + ' ';
    outputBuffer += "\x1b[0m";
}

void Editor::drawRows(std::string& outputBuffer) {
    auto renderBuffer = activeBuffer.getRenderBuffer();
    for (int y = 0; y < viewPortHeight; y++) {
        int fileRow = y + rowoff;
        if (renderBuffer.size() == 0 && fileRow == 0) {
            drawSidePanel(outputBuffer, 1);
        }
        if (fileRow < renderBuffer.size()) {
            drawSidePanel(outputBuffer, fileRow + 1);
            int actualSize = (int)(renderBuffer[fileRow].size() - coloff);
            int sizeToPrint =
                actualSize < viewPortWidth ? actualSize : viewPortWidth;
            if (sizeToPrint > 0) {
                outputBuffer +=
                    renderBuffer[fileRow].substr(coloff, sizeToPrint);
            }
        } else if (renderBuffer.size() == 0 && fileRow != 0) {
            outputBuffer += "~";
            if (y == viewPortHeight / 2) {
                int midWidth = viewPortWidth / 2;
                std::string welcome = "TTE editor windows -- version " +
                                      std::to_string(AppConfig::KILO_VERSION);
                int distance = midWidth - (welcome.size() / 2);
                welcome = std::string(distance, ' ') + welcome;
                outputBuffer += welcome;
            }
        } else {
            if (fileRow != 0) {
                outputBuffer += "~";
            }
        }
        outputBuffer += "\x1b[K";  // clear everything to the right of the line
        outputBuffer += "\r\n";
    }
}

// Calculate proper offsets to convert from render buffer space to screen space.
void Editor::scrollToCursor() {
    const DWORD renderY = activeBuffer.getRenderY();
    const DWORD renderX = activeBuffer.getRenderX();

    if (renderY > viewPortHeight + rowoff) {
        rowoff = renderY - viewPortHeight;
    }

    if (renderY < rowoff) {
        rowoff = renderY;
    }

    if (renderX > viewPortWidth + coloff) {
        coloff = renderX - viewPortWidth;
    }
    if (renderX < coloff) {
        coloff = renderX;
    }
}

void Editor::drawStatusBar(std::string& outputBuffer) {
    const DWORD MAX_FILENAME_SIZE = 20;

    std::string initalSpacer = " ";

    std::filesystem::path filePath;
    if (activeBuffer.getFilePath().has_value()) {
        filePath = activeBuffer.getFilePath().value();
    }

    std::string fileName =
        filePath.empty() ? "[No Name]" : filePath.filename().string();

    fileName = fileName.size() > MAX_FILENAME_SIZE
                   ? fileName.substr(0, MAX_FILENAME_SIZE)
                   : fileName;
    fileName += activeBuffer.getDirtyStatus() > 0 ? " [+] " : "";

    int scrollPercent = ((activeBuffer.getRenderY() + 1) /
                         (float)activeBuffer.getRenderBuffer().size()) *
                        100;

    std::string fileLocationStatus =
        std::to_string(scrollPercent) + "%   " +
        std::to_string(activeBuffer.getRenderY() + 1) + ":" +
        std::to_string(activeBuffer.getRenderX() + 1) + " ";

    int len = initalSpacer.size() + fileName.size() + fileLocationStatus.size();
    while (len < viewPortWidth) {
        fileName += " ";
        len++;
    }

    outputBuffer += "\x1b[7m" + initalSpacer + fileName + fileLocationStatus +
                    "\x1b[m" + "\r\n";
}

void Editor::processKeyPress() {
    int key = terminal.readKey();
    switch (key) {
        case static_cast<int>(AppConfig::editorKey::NO_KEY):
        case static_cast<int>(AppConfig::editorKey::UNKNOWN_KEY):
        case static_cast<int>(AppConfig::editorKey::ESCAPE):
            break;
        case AppConfig::ctrlkey('q'):
            editorRunning = false;
            break;
        case AppConfig::ctrlkey('s'):
            saveActiveBuffer();
            break;
        case AppConfig::ctrlkey('a'):
            // editorAi();
            break;
        case static_cast<int>(AppConfig::editorKey::ARROW_UP):
        case static_cast<int>(AppConfig::editorKey::ARROW_DOWN):
        case static_cast<int>(AppConfig::editorKey::ARROW_RIGHT):
        case static_cast<int>(AppConfig::editorKey::ARROW_LEFT):
        case static_cast<int>(AppConfig::editorKey::PAGE_UP):
        case static_cast<int>(AppConfig::editorKey::PAGE_DOWN):
        case static_cast<int>(AppConfig::editorKey::HOME):
        case static_cast<int>(AppConfig::editorKey::END):
            moveCursor(key);
            break;
        case static_cast<int>(AppConfig::editorKey::BACKSPACE):
            activeBuffer.deleteOperationAtCursor();
            break;
        case static_cast<int>(AppConfig::editorKey::DEL_KEY):
            moveCursor(static_cast<int>(AppConfig::editorKey::ARROW_RIGHT));
            activeBuffer.deleteOperationAtCursor();
            break;
        case static_cast<int>(AppConfig::editorKey::CARRIAGE):
            activeBuffer.insertNewLineAtCursor();
            break;
        default:
            activeBuffer.insertCharAtCursor(key);
            break;
    }
}

void Editor::moveCursor(int key) {
    switch (key) {
        case static_cast<int>(AppConfig::editorKey::ARROW_LEFT):
            activeBuffer.moveCursorLeft();
            break;
        case static_cast<int>(AppConfig::editorKey::ARROW_RIGHT):
            activeBuffer.moveCursorRight();
            break;
        case static_cast<int>(AppConfig::editorKey::ARROW_UP):
            activeBuffer.moveCursorUp();
            break;
        case static_cast<int>(AppConfig::editorKey::ARROW_DOWN):
            activeBuffer.moveCursorDown();
            break;
        case static_cast<int>(AppConfig::editorKey::PAGE_UP): {
            int times = viewPortHeight;
            for (; times > 0; times--) {
                activeBuffer.moveCursorUp();
            }
        } break;
        case static_cast<int>(AppConfig::editorKey::PAGE_DOWN): {
            int times = viewPortHeight;
            for (; times > 0; times--) {
                activeBuffer.moveCursorDown();
            }
        } break;
        case static_cast<int>(AppConfig::editorKey::HOME):
            activeBuffer.moveCursorToStartOfLine();
            break;
        case static_cast<int>(AppConfig::editorKey::END):
            activeBuffer.moveCursorToEndOfLine();
            break;
    }
}

void Editor::drawMessageBar(std::string& outputBuffer) {
    outputBuffer += "\x1b[K";  // clear everything to the right of the line

    std::time_t timeDifference = time(NULL) - messageSetTime;
    if (timeDifference > AppConfig::MESSAGE_DISPLAY_TIME) {
        return;
    }
    if (messageHistory.empty()) {
        return;
    }

    std::string lastMessage = messageHistory.back();
    if (lastMessage.size() >= viewPortWidth - 2) {
        outputBuffer += " " + lastMessage.substr(0, viewPortWidth - 5) + "... ";
    } else {
        outputBuffer += " " + lastMessage + " ";
    }
}

void Editor::saveActiveBuffer() {
    SaveResult saved = activeBuffer.save();
    if (saved == SaveResult::SUCCESS) {
        setStatusMessage("File Saved");
        return;
    } else if (saved == SaveResult::FILE_ERROR) {
        setStatusMessage("Could not open or completely write to file");
        return;
    }

    std::string givenPath = promptInput("Enter path : %s");

    if (givenPath.empty()) {
        setStatusMessage("Invalid file path provided. Saving aborted.");
        return;
    }

    saved = activeBuffer.saveAs(std::filesystem::path(givenPath));

    if (saved == SaveResult::SUCCESS) {
        setStatusMessage("File Saved");
        return;
    } else if (saved == SaveResult::FILE_ERROR ||
               saved == SaveResult::NO_FILEPATH) {
        setStatusMessage("Could not write/find to file.");
        return;
    }
}

void Editor::aiPrompt() {
    std::string prompt = promptInput("Enter Prompt:%s");

    if (prompt.empty()) {
        setStatusMessage("Text generation aborted.");
        return;
    }
    bool promptSent = aiClient.SendPrompt(prompt);
    if (!promptSent) {
        setStatusMessage("Prompt could not be sent, Ai busy!");
        return;
    }
    setStatusMessage("Ai working...");
    bool loop = true;
    while (loop) {
        renderScreen();
        LlmResponse res = aiClient.GetNextResponse();
        switch (res.status) {
            case LlmResponse::Status::RUNNING:
                for (char c : res.text) {
                    if (c == '\n') {
                        activeBuffer.insertNewLineAtCursor();
                    } else if (c == '\r') {
                        continue;
                    } else {
                        activeBuffer.insertCharAtCursor(c);
                    }
                }
                break;
            case LlmResponse::Status::FALIURE:
                setStatusMessage("Error! %s", res.error.what());
                loop = false;
                break;
            case LlmResponse::Status::FINISHED:
                loop = false;
                setStatusMessage("Ai responded successfully.");
                break;
            default:
                break;
        }
    }
    return;
}
