# SPDX-License-Identifier: MIT
# Demo executables — built only when KILNUI_BUILD_DEMOS=ON

# Helper macro: copy shaders + assets next to a demo executable
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

# ---- component_gallery ----
add_executable(component_gallery component_gallery.c)
target_link_libraries(component_gallery PRIVATE kilnui::ui_components)
add_dependencies(component_gallery shaders)
copy_demo_assets(component_gallery)

# ---- design_system_demo ----
add_executable(design_system_demo design_system_demo.c)
target_link_libraries(design_system_demo PRIVATE kilnui::ui_components)
add_dependencies(design_system_demo shaders)
copy_demo_assets(design_system_demo)

# ---- font_demo ----
add_executable(font_demo font_demo.c)
target_link_libraries(font_demo PRIVATE kilnui::ui_components)
add_dependencies(font_demo shaders)
copy_demo_assets(font_demo)

# ---- input_demo ----
add_executable(input_demo input_demo.c)
target_link_libraries(input_demo PRIVATE kilnui::ui_components)
add_dependencies(input_demo shaders)
copy_demo_assets(input_demo)
