# TTE: The Terminal Text Editor for Windows 💻

## Overview
TTE is a minimalist terminal-based text editor built from the ground up for Windows. It provides a clean interface for editing text files directly from the command line. It is developed as a learning project. 

## Current Features ✨

* **Core Editing:** Basic text insertion, deletion, and navigation.
* **File I/O:** Open and save files.
* **Line Numbers:** A static side panel displays line numbers for easy navigation and reference.
* **Status Bar:** A dynamic status bar at the bottom of the screen displays the current file name, dirty status (`[+]` for unsaved changes), and cursor position.
* **Message Bar:** A dedicated line below the status bar provides user feedback and status messages.
* **AI Integration:** A key feature of TTE is its integration with the Gemini API. Users can trigger AI text generation with a simple key command (`Ctrl-A`) and insert the generated content directly into the editor.

---

## Planned Features & Future Development 🚀

TTE is an ongoing project, and I have exciting plans to expand its capabilities and enhance the user experience. The following features are currently on the roadmap:

* **Line Wrapping:** Implement a feature to visually wrap long lines of text to fit within the terminal window, improving readability without adding actual newlines to the file.
* **Better AI Integration:**
    * **Visual Feedback:** Add a visual indicator (e.g., a "Thinking..." message) to let the user know when the AI is processing their request.
    * **Contextual Generation:** Allow users to select a portion of text within the editor and use it as contextual input for the AI prompt.
* **Multiple Buffers/Panes:** Introduce support for multiple open files (buffers) and the ability to view them side-by-side in different panes. This is a crucial feature for more complex workflows.
* **Syntax Highlighting:** Add basic syntax highlighting for common programming languages to improve code readability.
* **Clipboard Support:** Implement copy, cut, and paste functionality.
* **Search and Replace:** Add the ability to search for specific strings within the document and replace them.

---

## Usage ⌨️

* **Start the editor:**
    ```
    ./tte [filename]
    ```
    If no filename is provided, a new, empty buffer will be created.

* **Controls:**
    * **Ctrl-Q:** Quit the editor.
    * **Ctrl-S:** Save the current file. If the file is new, it will prompt you for a filename.
    * **Ctrl-A:** Trigger the AI prompt to generate text.

---

## Building the Project 🛠️

The project is built using C++ and CMake. To build TTE, you'll need MSVC C++ compiler and CMake installed on your system.

```
# Clone the repository
git clone [https://github.com/caspgin/TerminalTextEditorWindows.git]

# Navigate to the project directory
cd TerminalTextEditorWindows

# run the batch file
./run.bat
```
or you can use the Visual Studio IDE (I have not tested on the IDE but should work probably).
* Note: You'll need to set up the necessary environment variables and libraries for the Gemini API client (llm_api_client.h).
