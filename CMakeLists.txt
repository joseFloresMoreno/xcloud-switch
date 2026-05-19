cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED DEVKITPRO)
    if(DEFINED ENV{DEVKITPRO})
        set(DEVKITPRO $ENV{DEVKITPRO})
    else()
        set(DEVKITPRO "/opt/devkitpro")
    endif()
endif()

set(DEVKITA64 ${DEVKITPRO}/devkitA64)
set(CMAKE_C_COMPILER ${DEVKITA64}/bin/aarch64-none-elf-gcc)
set(CMAKE_CXX_COMPILER ${DEVKITA64}/bin/aarch64-none-elf-g++)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

project(hello-switch CXX)

set(LIBNX ${DEVKITPRO}/libnx)

set(ARCH_FLAGS "-march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE")
set(WARN_FLAGS "-Wall -Wextra -Wpedantic")
set(OPT_FLAGS "-O2 -ffunction-sections -fdata-sections")

set(CMAKE_CXX_FLAGS "${ARCH_FLAGS} ${WARN_FLAGS} ${OPT_FLAGS} -fomit-frame-pointer -fstrict-volatile-bitfields -std=c++17")
set(CMAKE_EXE_LINKER_FLAGS "-specs=${LIBNX}/switch.specs -g -Wl,-Map,output.map -Wl,--gc-sections")

include_directories(${LIBNX}/include)
link_directories(${LIBNX}/lib)

add_executable(hello-switch.elf
    src/main.cpp
)

target_link_libraries(hello-switch.elf
    nx
)

add_custom_command(TARGET hello-switch.elf POST_BUILD
    COMMAND ${DEVKITPRO}/tools/bin/nacptool --create "Hello Switch" "Jose Flores" "0.1" control.nacp
    COMMAND ${DEVKITPRO}/tools/bin/elf2nro hello-switch.elf hello-switch.nro --nacp=control.nacp
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating NRO..."
)

message(STATUS "")
message(STATUS "=== hello-switch Build Configuration ===")
message(STATUS "DEVKITPRO: ${DEVKITPRO}")
message(STATUS "DEVKITA64: ${DEVKITA64}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Minimal libnx hello world")
message(STATUS "========================================")
message(STATUS "")
