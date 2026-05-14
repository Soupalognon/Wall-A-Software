cmake_minimum_required(VERSION 3.14)
project(RobotCDRTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Exceptions are required: FreeRTOS stub uses TaskDelayEscape to exit task loops in tests
add_compile_options(-Wall -Wextra)

# Google Test — prefer system installation; fall back to FetchContent
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/v1.14.0.tar.gz
        URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Directory shortcuts
set(APP_DIR   "${CMAKE_SOURCE_DIR}/../App")
set(STUBS_DIR "${CMAKE_SOURCE_DIR}/Stubs")
set(MOCKS_DIR "${CMAKE_SOURCE_DIR}/Mocks")

# Common include paths — Stubs FIRST to shadow real FreeRTOS/HAL headers
set(COMMON_INCLUDES
    ${STUBS_DIR}                        # FreeRTOS.h, queue.h, task.h, stm32f4xx_hal.h stubs (must be first)
    ${APP_DIR}                          # App/ root: Tasks/, Services/, Drivers/, Interfaces/, Controllers/
    ${MOCKS_DIR}                        # MockBus.h, MockCommChannel.h, etc.
    ${CMAKE_SOURCE_DIR}                 # Tests/ root — so #include "Stubs/HalStub.h" resolves
    ${CMAKE_SOURCE_DIR}/../Core/Inc     # main.h and other CubeMX-generated headers
)

# ── BusFormatTest ──────────────────────────────────────────────────────────
add_executable(BusFormatTest
    Unit/BusFormatTest.cpp
    ${APP_DIR}/Services/BusFormat.cpp
)
target_include_directories(BusFormatTest PRIVATE ${COMMON_INCLUDES})
target_link_libraries(BusFormatTest PRIVATE GTest::gtest_main)

# ── ExternalCommTest ───────────────────────────────────────────────────────
add_executable(ExternalCommTest
    Unit/ExternalCommTest.cpp
    ${APP_DIR}/Tasks/ExternalComm.cpp
    ${APP_DIR}/Services/BusFormat.cpp
    Stubs/FreeRTOSStub.cpp
    Stubs/StaticDefs.cpp
)
target_include_directories(ExternalCommTest PRIVATE ${COMMON_INCLUDES})
target_link_libraries(ExternalCommTest PRIVATE GTest::gtest_main)

# ── ConcreteOdomHALTest ────────────────────────────────────────────────────
add_executable(ConcreteOdomHALTest
    Unit/ConcreteOdomHALTest.cpp
    ${APP_DIR}/Services/Odometry.cpp
    Stubs/FreeRTOSStub.cpp
)
target_include_directories(ConcreteOdomHALTest PRIVATE ${COMMON_INCLUDES})
target_link_libraries(ConcreteOdomHALTest PRIVATE GTest::gtest_main)

# ── OdoControlTest ────────────────────────────────────────────────────────
add_executable(OdoControlTest
    Unit/OdoControlTest.cpp
    ${APP_DIR}/Tasks/OdoControl.cpp
    ${APP_DIR}/Controllers/Pid.cpp
    ${APP_DIR}/Services/BusFormat.cpp
    Stubs/FreeRTOSStub.cpp
)
target_include_directories(OdoControlTest PRIVATE ${COMMON_INCLUDES})
target_link_libraries(OdoControlTest PRIVATE GTest::gtest_main)

# ── MotionPlannerTest ─────────────────────────────────────────────────────
add_executable(MotionPlannerTest
    Unit/MotionPlannerTest.cpp
    ${APP_DIR}/Tasks/MotionPlanner.cpp
    ${APP_DIR}/Services/BusFormat.cpp
    Stubs/FreeRTOSStub.cpp
)
target_include_directories(MotionPlannerTest PRIVATE ${COMMON_INCLUDES})
target_link_libraries(MotionPlannerTest PRIVATE GTest::gtest_main)

# ── SensorManagerTest ────────────────────────────────────────────────────
add_executable(SensorManagerTest
    Unit/SensorManagerTest.cpp
    ${APP_DIR}/Tasks/SensorManager.cpp
    ${APP_DIR}/Services/BusFormat.cpp
    Stubs/FreeRTOSStub.cpp)
target_include_directories(SensorManagerTest PRIVATE ${COMMON_INCLUDES})
target_link_libraries(SensorManagerTest PRIVATE GTest::gtest_main)

# ── Test discovery ─────────────────────────────────────────────────────────
include(GoogleTest)
gtest_discover_tests(BusFormatTest)
gtest_discover_tests(ExternalCommTest)
gtest_discover_tests(ConcreteOdomHALTest)
gtest_discover_tests(OdoControlTest)
gtest_discover_tests(MotionPlannerTest)
gtest_discover_tests(SensorManagerTest)
