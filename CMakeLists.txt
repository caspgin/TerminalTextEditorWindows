cmake_minimum_required(VERSION 3.10)

project(TerminalTextEditorWindows CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Ensure strict compliance
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message("compiler is ${CMAKE_CXX_COMPILER}")

add_subdirectory(extern/llmLib)

# Add source files
# This tells CMake to create an executable named 'MyConsoleProject'
# from the 'src/main.cpp' source file.
add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC llm_client)

