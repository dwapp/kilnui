cmake_minimum_required(VERSION 3.20)
project(kilnui VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---- Options ----
option(KILNUI_BUILD_DEMOS "Build demo executables" OFF)

# ---- Find dependencies ----
find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

# ---- Compile GLSL → SPIR-V ----
find_program(GLSLC glslc REQUIRED)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUT ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT})

set(SHADER_SOURCES
    ${SHADER_DIR}/rect.vert.glsl
    ${SHADER_DIR}/rect.frag.glsl
    ${SHADER_DIR}/tex.vert.glsl
    ${SHADER_DIR}/tex.frag.glsl
    ${SHADER_DIR}/shadow.frag.glsl
    ${SHADER_DIR}/border.frag.glsl
    ${SHADER_DIR}/image_rounded.frag.glsl
    ${SHADER_DIR}/gradient.frag.glsl
    ${SHADER_DIR}/circle.frag.glsl
    ${SHADER_DIR}/line.vert.glsl
    ${SHADER_DIR}/line.frag.glsl
    ${SHADER_DIR}/backdrop_blur.frag.glsl
    ${SHADER_DIR}/scroll_shadow.frag.glsl
)

foreach(GLSL ${SHADER_SOURCES})
    get_filename_component(FNAME ${GLSL} NAME)
    string(REPLACE ".glsl" ".spv" SPV_NAME ${FNAME})
    set(SPV_OUT ${SHADER_OUT}/${SPV_NAME})

    if(FNAME MATCHES "\\.vert\\.")
        set(STAGE vert)
    else()
        set(STAGE frag)
    endif()

    add_custom_command(
        OUTPUT ${SPV_OUT}
        COMMAND ${GLSLC} -fshader-stage=${STAGE} ${GLSL} -o ${SPV_OUT}
        DEPENDS ${GLSL}
        COMMENT "Compiling ${FNAME} -> ${SPV_NAME}"
    )
    list(APPEND SPV_OUTPUTS ${SPV_OUT})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_OUTPUTS})

# ---- Library: kilnui (core renderer) ----
add_library(kilnui STATIC
    src/kilnui.c
    src/kilnui_render.c
    src/glyph_cache.c
)
add_library(kilnui::kilnui ALIAS kilnui)

target_include_directories(kilnui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/kilnui>
    $<INSTALL_INTERFACE:include/kilnui/clay>
)
target_include_directories(kilnui SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/clay>
)

target_compile_options(kilnui PRIVATE -Wall -Wextra -Wno-unused-parameter)

target_link_libraries(kilnui PUBLIC
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
)
if(UNIX AND NOT APPLE)
    target_link_libraries(kilnui PUBLIC m)
endif()

# ---- Library: ui_components (reusable widgets) ----
add_library(ui_components STATIC
    src/ui/button.c
    src/ui/input.c
    src/ui/checkbox.c
    src/ui/radio.c
    src/ui/container.c
    src/ui/slider.c
    src/ui/dropdown.c
    src/ui/progress.c
    src/ui/tooltip.c
    src/ui/design_system.c
    src/ui/typography.c
    src/ui/theme.c
)
add_library(kilnui::ui_components ALIAS ui_components)

target_include_directories(ui_components PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/kilnui>
    $<INSTALL_INTERFACE:include/kilnui/clay>
)
target_include_directories(ui_components SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/clay>
)

target_link_libraries(ui_components PUBLIC kilnui)

target_compile_options(ui_components PRIVATE -Wall -Wextra -Wno-unused-parameter)

# ---- Install: libraries ----
include(GNUInstallDirs)

install(TARGETS kilnui ui_components
    EXPORT kilnuiTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# ---- Install: public headers ----
install(FILES
    src/kilnui.h
    src/glyph_cache.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kilnui
)

install(FILES
    src/ui/ui.h
    src/ui/ui_internal.h
    src/ui/button.h
    src/ui/checkbox.h
    src/ui/container.h
    src/ui/design_system.h
    src/ui/dropdown.h
    src/ui/icons.h
    src/ui/input.h
    src/ui/progress.h
    src/ui/radio.h
    src/ui/slider.h
    src/ui/theme.h
    src/ui/tooltip.h
    src/ui/typography.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kilnui/ui
)

# Install vendored clay.h
install(FILES
    3rdparty/clay/clay.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kilnui/clay
)

# ---- Install: shaders (pre-compiled SPIR-V) ----
install(FILES ${SPV_OUTPUTS}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/kilnui/shaders
)

# ---- Install: assets ----
install(DIRECTORY assets/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/kilnui/assets
    FILES_MATCHING PATTERN "*.md"
)

# ---- Install: CMake package config ----
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/kilnuiConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/kilnuiConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/kilnuiConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kilnui
)

install(EXPORT kilnuiTargets
    FILE kilnuiTargets.cmake
    NAMESPACE kilnui::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kilnui
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kilnuiConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/kilnuiConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kilnui
)

# ---- Install: pkg-config ----
set(PKG_CONFIG_REQUIRES "sdl3 sdl3-ttf")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/kilnui.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/kilnui.pc
    @ONLY
)

set(PKG_CONFIG_REQUIRES_PRIVATE "kilnui")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ui_components.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/ui_components.pc
    @ONLY
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kilnui.pc
    ${CMAKE_CURRENT_BINARY_DIR}/ui_components.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# ---- Demos (opt-in) ----
if(KILNUI_BUILD_DEMOS)
    add_subdirectory(demo)
endif()
