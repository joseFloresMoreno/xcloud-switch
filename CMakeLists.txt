cmake_minimum_required(VERSION 3.15)

# Set up DevKitPro toolchain for Switch (ARM64 AArch64)
if(NOT DEFINED DEVKITPRO)
  set(DEVKITPRO $ENV{DEVKITPRO})
  if(NOT DEVKITPRO)
    if(EXISTS "/opt/devkitpro")
      set(DEVKITPRO "/opt/devkitpro")
    elseif(EXISTS "$ENV{HOME}/devkitpro")
      set(DEVKITPRO "$ENV{HOME}/devkitpro")
    else()
      message(FATAL_ERROR "DEVKITPRO not found")
    endif()
  endif()
endif()

# Use devkitA64 for Switch (ARM64)
set(DEVKITA64 ${DEVKITPRO}/devkitA64)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER ${DEVKITA64}/bin/aarch64-none-elf-gcc)
set(CMAKE_CXX_COMPILER ${DEVKITA64}/bin/aarch64-none-elf-g++)
set(CMAKE_AR ${DEVKITA64}/bin/aarch64-none-elf-ar)
set(CMAKE_RANLIB ${DEVKITA64}/bin/aarch64-none-elf-ranlib)

project(xcloud-switch C CXX)

# Compiler flags for Switch (AArch64 - ARM64)
set(SWITCH_COMPILE_FLAGS "-Wall -Wno-shift-count-overflow -O2 -fomit-frame-pointer -ffunction-sections -fdata-sections -fPIE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SWITCH_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SWITCH_COMPILE_FLAGS}")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -specs=${DEVKITPRO}/libnx/switch.specs -Wl,--gc-sections -pie")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${DEVKITPRO}/libnx/include)
include_directories(${DEVKITPRO}/portlibs/switch/include)

# Link directories
link_directories(${DEVKITPRO}/libnx/lib)
link_directories(${DEVKITPRO}/portlibs/switch/lib)

# Source files
set(SOURCES
    src/main.c
    src/xcloud_api.c
    src/session.c
    src/webrtc_client.c
    src/video_decoder.c
    src/audio_decoder.c
    src/input_handler.c
    src/ui.c
)

# Create executable
add_executable(xcloud-switch.elf ${SOURCES})

# Link libraries
target_link_libraries(xcloud-switch.elf
    nx
)

# Print configuration
message(STATUS "")
message(STATUS "=== xcloud-switch Build Configuration (ARM64/AArch64) ===")
message(STATUS "DEVKITPRO: ${DEVKITPRO}")
message(STATUS "DEVKITA64: ${DEVKITA64}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "============================================================")
message(STATUS "")
