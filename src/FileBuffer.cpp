#include "FileBuffer.h"

#include <fstream>

#include "config.h"

FileBuffer::FileBuffer()
    : cursorX(0), cursorY(0), renderX(0), renderY(0), dirty(0) {}

// Insert Character where the cursor is
void FileBuffer::insertCharAtCursor(char characterToInsert) {
    if (cursorY == row.size()) {
        insertRow(cursorY, "");
    }
    insertCharInRow(cursorY, cursorX, characterToInsert);
    cursorX++;
    syncRenderCursor();
}

// Perform a delete operation at cursor
// 1. if at the very start x=0, y=0 then do nothing
// 2. if in a line then delete a character
// 3. if at the start of a line then combine that line with previous one
void FileBuffer::deleteOperationAtCursor() {
    // if there are no lines i.e. no data or the cursor is beyond the total
    // lines - do nothing
    if (row.size() == 0) {
        return;
    }
    if (cursorX == 0 && cursorY == 0) {
        return;
    }
    if (cursorY == row.size() && row.size() != 0) {
        cursorY--;
        cursorX = row[cursorY].size();
    } else if (cursorX > 0) {
        removeCharInRow(cursorY, cursorX - 1);
        cursorX--;
    } else {
        cursorX = (DWORD)row[cursorY - 1].size();
        appendStringToRow(cursorY - 1, row[cursorY]);
        deleteRow(cursorY);
        cursorY--;
    }
    syncRenderCursor();
}

// insert a new line at cursor position
// 1. if cursor is at start of a line, then insert a new line there and move
// that line below
// 2. if cursor is at end of a line, then just insert a new line below
// 3. if cursor is in middle
//		1. creates 2 new lines and copies the  data  after and
// before the cursor in the same line
//		2. deletes the current line
void FileBuffer::insertNewLineAtCursor() {
    if (cursorX == 0) {
        insertRow(cursorY, "");
    } else if (cursorX == row[cursorY].size()) {
        insertRow(cursorY + 1, "");
    } else {
        insertRow(cursorY + 1, row[cursorY].substr(cursorX));
        insertRow(cursorY + 1, row[cursorY].substr(0, cursorX));
        deleteRow(cursorY);
    }
    cursorX = 0;
    cursorY++;
    syncRenderCursor();
}
SaveResult FileBuffer::save() {
    if (!filepath.has_value()) {
        return SaveResult::NO_FILEPATH;
    }
    if (!dirty) {
        return SaveResult::SUCCESS;
    }
    std::ofstream outfile(filepath.value());
    if (!outfile) {
        return SaveResult::FILE_ERROR;
    }

    for (auto line = row.begin(); line != row.end(); line++) {
        outfile << *line << "\n";
    }
    if (!outfile) {
        return SaveResult::FILE_ERROR;
    }

    dirty = 0;
    return SaveResult::SUCCESS;
}

SaveResult FileBuffer::saveAs(std::filesystem::path givenPath) {
    namespace fs = std::filesystem;
    if (!givenPath.is_absolute()) {
        givenPath = fs::current_path() / givenPath;
    }
    std::error_code ec;
    fs::create_directories(givenPath.parent_path(), ec);
    if (ec) {
        return SaveResult::NO_FILEPATH;
    }
    std::optional<fs::path> oldPath = filepath;
    filepath = givenPath;
    auto saved = save();
    if (saved == SaveResult::NO_FILEPATH || saved == SaveResult::FILE_ERROR) {
        filepath = oldPath;
    }
    return saved;
}

SaveResult FileBuffer::readFile(std::filesystem::path givenPath) {
    namespace fs = std::filesystem;
    if (!fs::exists(givenPath)) {
        return SaveResult::NO_FILEPATH;
    }

    std::ifstream infile(givenPath);
    if (!infile) {
        return SaveResult::FILE_ERROR;
    }
    std::string line;
    while (std::getline(infile, line)) {
        while (!line.empty() &&
               (line[line.size() - 1] == '\r' || line[line.size() - 1] == '\n'))
            line.pop_back();
        insertRow(row.size(), line);
    }

    if (!infile.eof()) {
        return SaveResult::FILE_ERROR;
    }

    infile.close();
    dirty = 0;
    filepath = givenPath;
    return SaveResult::SUCCESS;
}

void FileBuffer::moveCursorLeft() {
    if (cursorX != 0) {
        cursorX--;
    } else {
        moveCursorUp();
        cursorX = row[cursorY].size();
    }
    syncRenderCursor();
}

void FileBuffer::moveCursorRight() {
    int currentRowSize = row[cursorY].size();
    if (currentRowSize > 0 && cursorX < currentRowSize - 1) {
        cursorX++;
    } else {
        moveCursorDown();
        cursorX = 0;
    }
    syncRenderCursor();
}

void FileBuffer::moveCursorUp() {
    if (cursorY > 0) {
        cursorY--;
        syncRenderCursor();
    }
}

void FileBuffer::moveCursorDown() {
    if (cursorY < row.size() - 1) {
        cursorY++;
        syncRenderCursor();
    }
}

void FileBuffer::moveCursorToStartOfLine() {
    cursorX = 0;
    syncRenderCursor();
}

void FileBuffer::moveCursorToEndOfLine() {
    if (cursorY < row.size()) {
        cursorX = row[cursorY].size();
        syncRenderCursor();
    }
}

// syncs logical cursorX and cursorY to renderX and renderY cursor.
void FileBuffer::syncRenderCursor() {
    renderY = cursorY;

    renderX = 0;

    std::string &line = this->row[cursorY];

    for (DWORD j = 0; j < cursorX; j++) {
        if (line[j] == '\t')
            renderX +=
                (AppConfig::TAB_STOP - 1) - (renderX % AppConfig::TAB_STOP);
        renderX++;
    }
}

// PRIVATE FUNCTION IMPLEMENTATIONS
//
//
//

// updates the render buffer for a specific row.
// Recalculates the specific render row.
void FileBuffer::updateRenderRow(DWORD rowNumber) {
    while (rowNumber >= render.size()) {
        render.push_back("");
    }
    std::string &row = this->row[rowNumber];
    std::string &renderRow = this->render[rowNumber];
    renderRow.clear();
    for (auto itr = row.begin(); itr != row.end(); itr++) {
        if (*itr == '\t') {
            renderRow += ' ';
            while (renderRow.size() % AppConfig::TAB_STOP != 0) {
                renderRow += ' ';
            }

        } else {
            renderRow += *itr;
        }
    }
}

// insert A new line(row) at insertRowAt index
// Can insert at the end (row.size()), anywhere in middle or at start (0)
void FileBuffer::insertRow(DWORD insertRowAt, const std::string &newRow) {
    if (insertRowAt < 0 || insertRowAt > row.size()) return;
    row.insert(row.begin() + insertRowAt, newRow);
    render.insert(render.begin() + insertRowAt, "");
    updateRenderRow(insertRowAt);
    dirty++;
}
// Append to an existing line(row)
void FileBuffer::appendStringToRow(DWORD targetRowToAppend,
                                   std::string &stringToAppend) {
    if (targetRowToAppend >= row.size() || targetRowToAppend < 0 ||
        row.size() == 0) {
        return;
    }
    row[targetRowToAppend] += stringToAppend;
    updateRenderRow(targetRowToAppend);
    dirty++;
}

// Delete a Row
void FileBuffer::deleteRow(DWORD rowToDel) {
    if (rowToDel >= row.size() || rowToDel < 0) {
        return;
    }
    row.erase(row.begin() + rowToDel);
    render.erase(render.begin() + rowToDel);
    dirty++;
}

// insert a character in a line(row). Insert it at insertAtIndex of the
// line.
void FileBuffer::insertCharInRow(DWORD rowIndex, DWORD insertAtIndex,
                                 char characterToInsert) {
    if (rowIndex >= row.size() || rowIndex < 0) {
        return;
    }
    std::string &line = this->row[rowIndex];
    if (insertAtIndex > line.size()) {
        insertAtIndex = (DWORD)row.size();
    }
    line.insert(line.begin() + insertAtIndex, characterToInsert);
    updateRenderRow(rowIndex);
    dirty++;
}

// Remove a character from a line(rowIndex).
void FileBuffer::removeCharInRow(DWORD rowIndex, DWORD removeAtIndex) {
    if (rowIndex >= row.size() || rowIndex < 0) {
        return;
    }

    std::string &line = row[rowIndex];
    if (removeAtIndex >= line.size()) {
        return;
    }
    line.erase(line.begin() + removeAtIndex);
    updateRenderRow(rowIndex);
    dirty++;
}
