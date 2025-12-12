#ifndef FILEBUFFER_H
#define FILEBUFFER_H

#include <windows.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

enum class SaveResult { SUCCESS, NO_FILEPATH, FILE_ERROR };

class FileBuffer {
   public:
    FileBuffer();
    void insertCharAtCursor(char characterToInsert);
    void deleteOperationAtCursor();
    void insertNewLineAtCursor();

    SaveResult save();
    SaveResult saveAs(std::filesystem::path givenPath);
    SaveResult readFile(std::filesystem::path givenPath);

    void moveCursorLeft();
    void moveCursorRight();
    void moveCursorUp();
    void moveCursorDown();
    void moveCursorToStartOfLine();
    void moveCursorToEndOfLine();

    DWORD getCursorX() const { return cursorX; }
    DWORD getCursorY() const { return cursorY; }
    DWORD getRenderX() const { return renderX; }
    DWORD getRenderY() const { return renderY; }

    const std::vector<std::string> &getRenderBuffer() const { return render; }
    DWORD getDirtyStatus() const { return dirty; }
    const std::optional<std::filesystem::path> getFilePath() const {
        return filepath;
    }

   private:
    void updateRenderRow(DWORD rowNumber);

    void insertRow(DWORD insertRowAt, const std::string &newRow);
    void appendStringToRow(DWORD targetRowToAppend,
                           std::string &stringToAppend);
    void deleteRow(DWORD rowToDel);
    void insertCharInRow(DWORD rowIndex, DWORD insertAtIndex,
                         char characterToInsert);

    void removeCharInRow(DWORD rowIndex, DWORD removeAtIndex);
    void syncRenderCursor();

   private:
    DWORD cursorX, cursorY, renderX, renderY;
    DWORD dirty;
    std::vector<std::string> row;
    std::vector<std::string> render;
    std::optional<std::filesystem::path> filepath;
};

#endif  // !FILEBUFFER_H
