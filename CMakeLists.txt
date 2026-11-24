cmake_minimum_required(VERSION 3.24)
project(conflict LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(SDL3 REQUIRED CONFIG)

if(MINGW)
  # Windows (MinGW): C/C++ runtime linked in, so only SDL3.dll goes next to the program.
  add_link_options(-static)
endif()

add_library(conflict_platform STATIC
  src/platform/ega.cpp
  src/platform/dosfile.cpp
  src/platform/assets.cpp
  src/platform/host.cpp)
target_include_directories(conflict_platform PUBLIC src)
target_link_libraries(conflict_platform PUBLIC SDL3::SDL3)
target_compile_options(conflict_platform PRIVATE -Wall -Wextra)

add_executable(conflict-viewer src/tools/viewer.cpp)
target_link_libraries(conflict-viewer PRIVATE conflict_platform)
target_compile_options(conflict-viewer PRIVATE -Wall -Wextra)

add_library(conflict_game STATIC
  src/game/dgroup.cpp
  src/game/fixes.cpp
  src/game/crt.cpp
  src/game/sys.cpp
  src/game/ui.cpp
  src/game/save.cpp
  src/game/startup.cpp
  src/game/turn.cpp
  src/game/news.cpp
  src/game/setup.cpp
  src/game/reports.cpp
  src/game/ai.cpp
  src/game/monthly.cpp
  src/game/newspaper.cpp
  src/game/affairs.cpp
  src/game/diplomacy.cpp
  src/game/endgame.cpp
  src/game/yearly.cpp
  src/game/arms.cpp
  src/game/policy.cpp
  src/game/month.cpp
  src/game/border.cpp
  src/game/war.cpp)
target_link_libraries(conflict_game PUBLIC conflict_platform)
target_compile_options(conflict_game PRIVATE -Wall -Wextra)

add_executable(conflict src/game/main.cpp)
target_link_libraries(conflict PRIVATE conflict_game)
target_compile_options(conflict PRIVATE -Wall -Wextra)
if(WIN32)
  # A windowed program (no console); errors are shown in message boxes. SDL3.dll is copied next
  # to it so the build folder runs as is.
  set_target_properties(conflict PROPERTIES WIN32_EXECUTABLE ON)
  add_custom_command(TARGET conflict POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:conflict>)
endif()

enable_testing()
add_executable(core_tests tests/core_tests.cpp)
target_link_libraries(core_tests PRIVATE conflict_game)
add_test(NAME core_tests COMMAND core_tests)
set_tests_properties(core_tests PROPERTIES
  ENVIRONMENT "CONFLICT_GAME_DIR=${CMAKE_SOURCE_DIR}/conflict"
  SKIP_RETURN_CODE 77)

add_executable(difftest tests/difftest.cpp)
target_link_libraries(difftest PRIVATE conflict_game)

# Differential tests against the original code (needs .venv with unicorn: see README).
if(EXISTS ${CMAKE_SOURCE_DIR}/.venv/bin/python)
  add_test(NAME difftest_quick
    COMMAND ${CMAKE_SOURCE_DIR}/.venv/bin/python ${CMAKE_SOURCE_DIR}/tools/difftest.py
            --build ${CMAKE_BINARY_DIR} --sweep nation_turn --seeds 1
            "new_game#1" "new_game#7" "random:7#3")
endif()

add_executable(fixes_tests tests/fixes_tests.cpp)
target_link_libraries(fixes_tests PRIVATE conflict_game)
add_test(NAME fixes_tests COMMAND fixes_tests)
set_tests_properties(fixes_tests PROPERTIES
  ENVIRONMENT "CONFLICT_GAME_DIR=${CMAKE_SOURCE_DIR}/conflict"
  SKIP_RETURN_CODE 77)
