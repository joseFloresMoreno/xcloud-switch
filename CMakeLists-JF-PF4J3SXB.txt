cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED DEVKITPRO)
    if(DEFINED ENV{DEVKITPRO})
        set(DEVKITPRO $ENV{DEVKITPRO})
    else()
        set(DEVKITPRO "/opt/devkitpro")
    endif()
endif()

# Cross-compilation: tell CMake this is not a native Windows/Linux build
# so it does not inject host-OS linker flags (e.g. Windows PE --major-image-version)
set(CMAKE_SYSTEM_NAME      "Generic" CACHE STRING "" FORCE)
set(CMAKE_SYSTEM_PROCESSOR "aarch64" CACHE STRING "" FORCE)

set(DEVKITA64 ${DEVKITPRO}/devkitA64)
set(CMAKE_C_COMPILER   ${DEVKITA64}/bin/aarch64-none-elf-gcc)
set(CMAKE_CXX_COMPILER ${DEVKITA64}/bin/aarch64-none-elf-g++)
set(CMAKE_C_COMPILER_WORKS   1)
set(CMAKE_CXX_COMPILER_WORKS 1)

project(greenlight-switch C CXX)

set(LIBNX    ${DEVKITPRO}/libnx)
set(PORTLIBS ${DEVKITPRO}/portlibs/switch)

set(ARCH_FLAGS "-march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE")
set(WARN_FLAGS "-Wall -Wextra -Wpedantic")
set(OPT_FLAGS  "-O2 -ffunction-sections -fdata-sections")
set(EXTRA_DEFS "-D_GNU_SOURCE -D__SWITCH__")

set(CMAKE_C_FLAGS   "${ARCH_FLAGS} ${WARN_FLAGS} ${OPT_FLAGS} ${EXTRA_DEFS} -fomit-frame-pointer -fstrict-volatile-bitfields -std=c11")
set(CMAKE_CXX_FLAGS "${ARCH_FLAGS} ${WARN_FLAGS} ${OPT_FLAGS} ${EXTRA_DEFS} -fomit-frame-pointer -fstrict-volatile-bitfields -std=c++17")
set(CMAKE_EXE_LINKER_FLAGS "-specs=${LIBNX}/switch.specs -g -Wl,-Map,output.map -Wl,--gc-sections")

include_directories(${LIBNX}/include)
include_directories(${PORTLIBS}/include)
include_directories(${PORTLIBS}/include/SDL2)
include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${LIBNX}/lib)
link_directories(${PORTLIBS}/lib)

# -----------------------------------------------------------------------
# Feature detection
# -----------------------------------------------------------------------

# switch-curl — real HTTPS for Xbox API calls
# Install: sudo dkp-pacman -S switch-curl
if(EXISTS "${PORTLIBS}/lib/libcurl.a")
    set(CURL_FOUND TRUE)
    message(STATUS "switch-curl found — building with real HTTPS")
else()
    set(CURL_FOUND FALSE)
    message(STATUS "switch-curl NOT found — xcloud_api runs as stub")
    message(STATUS "  To enable: sudo dkp-pacman -S switch-curl")
endif()

# SDL2 + SDL2_ttf — graphical UI (Moonlight/Plutonium style)
# Install: sudo dkp-pacman -S switch-sdl2 switch-sdl2_ttf switch-freetype
if(EXISTS "${PORTLIBS}/lib/libSDL2.a" AND EXISTS "${PORTLIBS}/lib/libSDL2_ttf.a")
    set(SDL2_FOUND TRUE)
    message(STATUS "SDL2 + SDL2_ttf found — building graphical UI")
else()
    set(SDL2_FOUND FALSE)
    message(STATUS "SDL2/SDL2_ttf NOT found — falling back to console UI")
    message(STATUS "  To enable: sudo dkp-pacman -S switch-sdl2 switch-sdl2_ttf switch-freetype")
endif()

# -----------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------

set(SOURCES
    src/main.cpp
    src/session.c
    src/xcloud_api.c
    src/xbox_auth.c
    src/webrtc_sdp.c
    src/webrtc_ice.c
    src/webrtc_client.c
    src/audio_decoder.c
    src/video_decoder.c
    src/input_handler.c
    src/xbox_host.c
)

# Use SDL2 graphical UI when available, otherwise console fallback
if(SDL2_FOUND)
    list(APPEND SOURCES src/ui_sdl.cpp)
else()
    list(APPEND SOURCES src/ui.c)
endif()

add_executable(greenlight-switch.elf ${SOURCES})

# -----------------------------------------------------------------------
# Compile definitions & link libraries
# -----------------------------------------------------------------------

if(CURL_FOUND)
    target_compile_definitions(greenlight-switch.elf PRIVATE HAVE_CURL CURL_STATICLIB)
endif()

if(SDL2_FOUND)
    target_compile_definitions(greenlight-switch.elf PRIVATE HAVE_SDL2)
endif()

# Link order matters with static libs: dependents before providers.
# nx (libnx) must come last so curl and SDL2 can resolve its symbols.
set(LINK_LIBS "")

if(SDL2_FOUND)
    # SDL2_ttf → harfbuzz, freetype → bz2, png; SDL2 → EGL, drm_nouveau
    list(APPEND LINK_LIBS SDL2_ttf harfbuzz freetype bz2 png SDL2 EGL glapi drm_nouveau)
endif()

if(CURL_FOUND)
    list(APPEND LINK_LIBS curl z)
endif()

# libnx always last — provides Switch OS/crypto symbols needed by curl and SDL2
# z (zlib) only needed when curl or SDL2 is present
if(CURL_FOUND OR SDL2_FOUND)
    list(APPEND LINK_LIBS z)
endif()
list(APPEND LINK_LIBS nx)

target_link_libraries(greenlight-switch.elf ${LINK_LIBS})

# -----------------------------------------------------------------------
# NRO packaging
# -----------------------------------------------------------------------

add_custom_command(TARGET greenlight-switch.elf POST_BUILD
    COMMAND ${DEVKITPRO}/tools/bin/nacptool
            --create "Greenlight Switch" "Jose Flores" "0.1"
            greenlight.nacp
    COMMAND ${DEVKITPRO}/tools/bin/elf2nro
            $<TARGET_FILE:greenlight-switch.elf> greenlight-switch.nro
            --nacp=greenlight.nacp
            --icon=${CMAKE_SOURCE_DIR}/icon.jpg
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Packaging NRO..."
)

# -----------------------------------------------------------------------
# Build summary
# -----------------------------------------------------------------------

message(STATUS "")
message(STATUS "=== greenlight-switch Build Configuration ===")
message(STATUS "DEVKITPRO : ${DEVKITPRO}")
message(STATUS "Compiler  : ${CMAKE_CXX_COMPILER}")
message(STATUS "curl      : ${CURL_FOUND}")
message(STATUS "SDL2 UI   : ${SDL2_FOUND}")
message(STATUS "Modules   : session, xcloud_api, xbox_auth, webrtc(stub), audio(stub), video(stub), input, xbox_host")
message(STATUS "============================================")
message(STATUS "")
