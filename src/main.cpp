
#include <Windows.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <winbase.h>

#include <iostream>

int main() {
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hInput == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Could not get standard input handle. Code: "
                  << GetLastError() << std::endl;
        return 1;
    }

    char c;
    DWORD bytesRead;
    while (ReadFile(hInput, &c, sizeof(c), &bytesRead, NULL) && c != 'q');
    return 0;
}
