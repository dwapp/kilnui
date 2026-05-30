cmake_minimum_required(VERSION 3.20)
project(kilnui C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optional warnings
add_compile_options(-Wall -Wextra -Wno-unused-parameter)

# ---- Find dependencies ----
find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

# ---- Compile GLSL → SPIR-V ----
find_program(GLSLC glslc REQUIRED)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUT ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT})

set(SHADER_SOURCES
    ${SHADER_DIR}/rect.vert.glsl
    ${SHADER_DIR}/rect.frag.glsl
    ${SHADER_DIR}/tex.vert.glsl
    ${SHADER_DIR}/tex.frag.glsl
)

foreach(GLSL ${SHADER_SOURCES})
    get_filename_component(FNAME ${GLSL} NAME)
    string(REPLACE ".glsl" ".spv" SPV_NAME ${FNAME})
    set(SPV_OUT ${SHADER_OUT}/${SPV_NAME})

    # Detect stage from filename
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

# ---- Library: kilnui ----
add_library(kilnui STATIC
    src/kilnui.c
    src/kilnui_render.c
    src/glyph_cache.c
)
target_include_directories(kilnui PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/3rdparty/clay
)
target_link_libraries(kilnui PUBLIC
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
)
# math library on Linux
if(UNIX AND NOT APPLE)
    target_link_libraries(kilnui PUBLIC m)
endif()

# ---- Library: ui_components (reusable widgets) ----
add_library(ui_components STATIC
    src/ui/button.c
    src/ui/input.c
    src/ui/checkbox.c
    src/ui/radio.c
    src/ui/label.c
    src/ui/container.c
    src/ui/slider.c
    src/ui/dropdown.c
    src/ui/progress.c
    src/ui/tooltip.c
)
target_include_directories(ui_components PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/3rdparty/clay
)
target_link_libraries(ui_components PUBLIC kilnui)

# ---- Demo executable ----
add_executable(clay_demo demo/main.c)
target_link_libraries(clay_demo PRIVATE kilnui)
add_dependencies(clay_demo shaders)

# ---- Component gallery demo ----
add_executable(component_gallery demo/component_gallery.c)
target_link_libraries(component_gallery PRIVATE kilnui ui_components)
add_dependencies(component_gallery shaders)

# Helper macro: copy shaders + assets next to any demo executable
macro(copy_demo_assets TARGET)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${SHADER_OUT} $<TARGET_FILE_DIR:${TARGET}>/shaders
        COMMENT "Copying shaders -> ${TARGET}"
    )
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${TARGET}>/assets
        COMMENT "Copying assets -> ${TARGET}"
    )
endmacro()

copy_demo_assets(clay_demo)
copy_demo_assets(component_gallery)
