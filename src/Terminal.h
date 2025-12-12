#ifndef TERMINAL_H
#define TERMINAL_H

#include <windows.h>
#include <winnt.h>

#include <string>

class Terminal {
   public:
    Terminal();
    ~Terminal();
    SHORT getScreenHeight() const;
    SHORT getScreenWidth() const;
    bool write(std::string& outputBuffer);
    int readKey();

   private:
    bool enableRawMode();
    bool gotHandlesSuccessfully();
    void disableRawMode();
    bool getWindowSize();

   private:
    HANDLE inputHandle;
    HANDLE outputHandle;
    DWORD originalInputMode;
    DWORD originalOutputMode;
    SHORT windowWidth;
    SHORT windowHeight;
};

#endif
