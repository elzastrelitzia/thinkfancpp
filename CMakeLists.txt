cmake_minimum_required(VERSION 3.16)
project(thinkfancpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt6 - enable AUTOMOC for QObject-derived classes
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Concurrent Svg)

# GNU standard install directories
include(GNUInstallDirs)

# Testing
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
endif()

# Core library
add_library(thinkfancpp_core STATIC
    src/core/FanTypes.cpp
    src/core/FanTypes.h
    src/core/ProcFanParser.cpp
    src/core/ProcFanParser.h
    src/core/HelperArgs.cpp
    src/core/HelperArgs.h
    src/core/ThinkfanDetector.cpp
    src/core/ThinkfanDetector.h
    src/core/SensorReader.cpp
    src/core/SensorReader.h
    src/gui/PrivilegedFanController.cpp
    src/gui/PrivilegedFanController.h
    src/gui/MainWindow.cpp
    src/gui/MainWindow.h
    src/gui/Theme.cpp
    src/gui/Theme.h
    assets/thinkfancpp.qrc
)

target_include_directories(thinkfancpp_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Embed helper install path at compile time for robust resolution
get_filename_component(HELPER_INSTALL_FULL_BINDIR "${CMAKE_INSTALL_FULL_BINDIR}" ABSOLUTE)
target_compile_definitions(thinkfancpp_core PRIVATE
    HELPER_INSTALL_PATH="${HELPER_INSTALL_FULL_BINDIR}"
)

target_link_libraries(thinkfancpp_core PUBLIC Qt6::Core Qt6::Widgets Qt6::Concurrent Qt6::Svg)

# GUI executable
add_executable(thinkfancpp
    src/main.cpp
)

target_link_libraries(thinkfancpp PRIVATE thinkfancpp_core Qt6::Widgets Qt6::Core)

# Helper executable (privileged writer)
add_executable(thinkfancpp-helper
    helper/main.cpp
)

target_link_libraries(thinkfancpp-helper PRIVATE thinkfancpp_core)

# Smoke test
if(BUILD_TESTING)
    add_executable(test_smoke
        tests/test_smoke.cpp
    )
    target_link_libraries(test_smoke PRIVATE thinkfancpp_core)
    add_test(NAME test_smoke COMMAND test_smoke)

    # ProcFanParser test
    add_executable(test_proc_fan_parser
        tests/test_proc_fan_parser.cpp
    )
    target_link_libraries(test_proc_fan_parser PRIVATE thinkfancpp_core)
    add_test(NAME test_proc_fan_parser COMMAND test_proc_fan_parser)

    # HelperArgs test
    add_executable(test_helper_args
        tests/test_helper_args.cpp
    )
    target_link_libraries(test_helper_args PRIVATE thinkfancpp_core)
    add_test(NAME test_helper_args COMMAND test_helper_args)

    # ThinkfanDetector test
    add_executable(test_thinkfan_detector
        tests/test_thinkfan_detector.cpp
    )
    target_link_libraries(test_thinkfan_detector PRIVATE thinkfancpp_core)
    add_test(NAME test_thinkfan_detector COMMAND test_thinkfan_detector)

    # SensorReader test
    add_executable(test_sensor_reader
        tests/test_sensor_reader.cpp
    )
    target_link_libraries(test_sensor_reader PRIVATE thinkfancpp_core)
    add_test(NAME test_sensor_reader COMMAND test_sensor_reader)

    # Theme test
    add_executable(test_theme
        tests/test_theme.cpp
    )
    target_link_libraries(test_theme PRIVATE thinkfancpp_core)
    add_test(NAME test_theme COMMAND test_theme)
endif()

# Install rules
install(TARGETS thinkfancpp thinkfancpp-helper
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES polkit/com.thinkfancpp.helper.policy
    DESTINATION ${CMAKE_INSTALL_DATADIR}/polkit-1/actions
    RENAME com.thinkfancpp.helper.policy
)

install(FILES packaging/thinkfancpp.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES assets/logo-sizes/logo_1280x1280.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/1280x1280/apps
    RENAME thinkfancpp.png
)

install(FILES assets/logo-sizes/logo_1280x1280.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
    RENAME thinkfancpp.png
)
