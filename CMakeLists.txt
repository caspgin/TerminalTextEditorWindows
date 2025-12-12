cmake_minimum_required(VERSION 3.10)

project(TerminalTextEditorWindows LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Ensure strict compliance
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message("compiler is ${CMAKE_CXX_COMPILER}")

add_subdirectory(extern/llmLib)

file(GLOB_RECURSE SOURCES_AND_HEADERS
"./src/*.cpp"
"./src/*.h")

add_executable(${PROJECT_NAME} ${SOURCES_AND_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PUBLIC llm_client)

