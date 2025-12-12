#include "Editor.h"

/*
 void die(LPCSTR lpMessage) {
    std::string clearScreen = "\x1b[2J\x1b[H";

    std::cerr << clearScreen << "Error: " << lpMessage
              << " ErrorCode: " << GetLastError() << std::endl;
    exit(1);
}
*/

int main(int argc, char *argv[]) {
    Editor editor;
    if (argc >= 2) {
        editor.open(argv[1]);
    }
    editor.setStatusMessage(
        "HELP: Ctrl-Q = quit | Ctrl-A = Ai text generation | Ctrl-S = "
        "Save/Save as");
    editor.run();
    return 0;
}
