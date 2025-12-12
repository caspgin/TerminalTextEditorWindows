#ifndef EDITOR_H
#define EDITOR_H

#include <windows.h>
#include <winnt.h>

#include <ctime>

#include "FileBuffer.h"
#include "Terminal.h"
#include "llm_api_client.h"

class Editor {
   public:
    Editor();
    void run();
    void open(const std::string filePath);
    bool setStatusMessage(const std::string fmt, ...);
    std::string promptInput(std::string promptLabel);

   private:
    void renderScreen(int overrideCursorX = -1, int overrideCursorY = -1);
    void scrollToCursor();
    void drawRows(std::string& outputBuffer);
    void drawSidePanel(std::string& outputBuffer, const int lineNumber);
    void drawStatusBar(std::string& outputBuffer);
    void processKeyPress();
    void moveCursor(int key);
    void drawMessageBar(std::string& outputBuffer);
    void saveActiveBuffer();
    void aiPrompt();

   private:
    Terminal terminal;
    std::time_t messageSetTime;
    FileBuffer activeBuffer;
    DWORD rowoff, coloff;
    DWORD viewPortWidth, viewPortHeight;
    std::vector<std::string> messageHistory;
    bool editorRunning;
    std::filesystem::path filePath;
    LlmClient aiClient;
};

#endif  // !EDITOR_H
