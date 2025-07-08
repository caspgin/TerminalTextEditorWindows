
#include <Windows.h>
#include <winbase.h>

int main() {
    HANDLE OutputH = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE InputH = GetStdHandle(STD_INPUT_HANDLE);

    return 0;
}
