cmake_minimum_required(VERSION 3.27)

project(oocdda LANGUAGES CXX)

# Dependencies
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)

# Library
add_library(
  oocdda_lib OBJECT
  src/addiction.h
  src/artifact.h
  src/bionics.cpp
  src/bionics.h
  src/bodypart.cpp
  src/bodypart.h
  src/color.h
  src/crafting.cpp
  src/crafting.h
  src/dialogue.h
  src/disease.h
  src/enums.h
  src/event.cpp
  src/event.h
  src/facdata.h
  src/faction.cpp
  src/faction.h
  src/field.cpp
  src/game.cpp
  src/game.h
  src/help.cpp
  src/item.cpp
  src/item.h
  src/itype.h
  src/itypedef.cpp
  src/iuse.cpp
  src/iuse.h
  src/keypress.cpp
  src/keypress.h
  src/line.cpp
  src/line.h
  src/map.cpp
  src/map.h
  src/mapdata.h
  src/mapgen.cpp
  src/mapitems.h
  src/mapitemsdef.cpp
  src/mission.h
  src/monattack.cpp
  src/monattack.h
  src/mondeath.cpp
  src/mondeath.h
  src/mongroup.h
  src/mongroupdef.cpp
  src/monitemsdef.cpp
  src/monmove.cpp
  src/monster.cpp
  src/monster.h
  src/mtype.h
  src/mtypedef.cpp
  src/newcharacter.cpp
  src/npc.cpp
  src/npc.h
  src/npcmove.cpp
  src/npctalk.cpp
  src/omdata.h
  src/output.cpp
  src/output.h
  src/overmap.cpp
  src/overmap.h
  src/player.cpp
  src/player.h
  src/pldata.h
  src/rng.cpp
  src/rng.h
  src/settlement.cpp
  src/settlement.h
  src/setvector.cpp
  src/setvector.h
  src/skill.cpp
  src/skill.h
  src/trap.h
  src/trapdef.cpp
  src/trapfunc.cpp
  src/tutorial.h
  src/window.h
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

include(CTest)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

add_custom_target(
  run-exe
  COMMAND oocdda_exe
  VERBATIM)

add_dependencies(run-exe oocdda_exe)
