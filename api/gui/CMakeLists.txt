project(MultiServerMC_gui LANGUAGES CXX)

set(GUI_SOURCES
    DesktopServices.h
    DesktopServices.cpp

    # Icons
    icons/MSMCIcon.h
    icons/MSMCIcon.cpp
    icons/IconList.h
    icons/IconList.cpp
)
################################ COMPILE ################################

add_library(MultiServerMC_gui SHARED ${GUI_SOURCES})
set_target_properties(MultiServerMC_gui PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)

generate_export_header(MultiServerMC_gui)

# Link
target_link_libraries(MultiServerMC_gui MultiServerMC_iconfix MultiServerMC_logic Qt5::Gui)

# Mark and export headers
target_include_directories(MultiServerMC_gui PUBLIC "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")

# Install it
install(
    TARGETS MultiServerMC_gui
    RUNTIME DESTINATION ${LIBRARY_DEST_DIR}
    LIBRARY DESTINATION ${LIBRARY_DEST_DIR}
)
