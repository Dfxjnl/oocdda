cmake_minimum_required(VERSION 3.27)

project(oocdda LANGUAGES CXX)

# Dependencies
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)

# Library
add_library(
  oocdda_lib OBJECT
  src/addiction.hpp
  src/artifact.hpp
  src/bionics.cpp
  src/bionics.hpp
  src/bodypart.cpp
  src/bodypart.hpp
  src/color.hpp
  src/crafting.cpp
  src/crafting.hpp
  src/dialogue.hpp
  src/disease.hpp
  src/enums.hpp
  src/event.cpp
  src/event.hpp
  src/facdata.hpp
  src/faction.cpp
  src/faction.hpp
  src/field.cpp
  src/file_utils.cpp
  src/file_utils.hpp
  src/game.cpp
  src/game.hpp
  src/help.cpp
  src/item.cpp
  src/item.hpp
  src/itype.hpp
  src/itypedef.cpp
  src/iuse.cpp
  src/iuse.hpp
  src/keypress.cpp
  src/keypress.hpp
  src/line.cpp
  src/line.hpp
  src/map.cpp
  src/map.hpp
  src/mapdata.hpp
  src/mapgen.cpp
  src/mapitems.hpp
  src/mapitemsdef.cpp
  src/mission.hpp
  src/mondeath.cpp
  src/mondeath.hpp
  src/mongroup.hpp
  src/mongroupdef.cpp
  src/monitemsdef.cpp
  src/monmove.cpp
  src/monster.cpp
  src/monster.hpp
  src/monster_attack.cpp
  src/monster_attack.hpp
  src/monster_type.hpp
  src/mtypedef.cpp
  src/newcharacter.cpp
  src/npc.cpp
  src/npc.hpp
  src/npcmove.cpp
  src/npctalk.cpp
  src/omdata.hpp
  src/output.cpp
  src/output.hpp
  src/overmap.cpp
  src/overmap.hpp
  src/player.cpp
  src/player.hpp
  src/pldata.hpp
  src/rng.cpp
  src/rng.hpp
  src/settlement.cpp
  src/settlement.hpp
  src/setvector.cpp
  src/setvector.hpp
  src/skill.cpp
  src/skill.hpp
  src/trap.hpp
  src/trapdef.cpp
  src/trapfunc.cpp
  src/tutorial.hpp
  src/wish.cpp)

target_compile_features(oocdda_lib PUBLIC cxx_std_20)
target_compile_options(oocdda_lib PRIVATE ${CURSES_CFLAGS})

target_include_directories(
  oocdda_lib PUBLIC "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
                    ${CURSES_INCLUDE_DIRS})

target_link_libraries(oocdda_lib PUBLIC ${CURSES_LIBRARIES})

# Executable
add_executable(oocdda_exe src/main.cpp)
add_executable(oocdda::exe ALIAS oocdda_exe)
set_target_properties(oocdda_exe PROPERTIES OUTPUT_NAME oocdda)
target_compile_features(oocdda_exe PRIVATE cxx_std_20)
target_link_libraries(oocdda_exe PRIVATE oocdda_lib)

add_custom_command(
  TARGET oocdda_exe
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/data
    ${CMAKE_CURRENT_BINARY_DIR}/data DEPENDS ${CMAKE_CURRENT_BINARY_DIR/data}
  COMMENT "symbolic link ${CMAKE_SOURCE_DIR}/data =>
    ${CMAKE_CURRENT_BINARY_DIR}/data")

include(CTest)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

add_custom_target(
  run-exe
  COMMAND oocdda_exe
  VERBATIM)

add_dependencies(run-exe oocdda_exe)
