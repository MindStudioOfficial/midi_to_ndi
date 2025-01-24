cmake_minimum_required(VERSION 3.14)

project(midi_to_ndi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_sources(${PROJECT_NAME} PRIVATE src/pch.cpp)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /wd"4100" /wd"4996")

set(VIRTUAL_MIDI_SRC "$ENV{LIB_SDK}/teVirtualMIDISDK/C-Binding")
set(NDI_SDK "C:/Program Files/NDI/NDI 6.1.1.0 SDK")

include_directories(
  "${VIRTUAL_MIDI_SRC}"
  "${NDI_SDK}/Include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  "${VIRTUAL_MIDI_SRC}/teVirtualMIDI64.lib"
  "${NDI_SDK}/Lib/x64/Processing.NDI.Lib.x64.lib"
  winmm.lib
)

target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${NDI_SDK}/Bin/x64/Processing.NDI.Lib.x64.dll"
  "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Processing.NDI.Lib.x64.dll"
)