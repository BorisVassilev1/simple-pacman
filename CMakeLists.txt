# CMakeList.txt : CMake project for pacman, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(COMPILE_ARGS -Wall -std=c++17)

project ("pacman")

add_subdirectory("./lib/yoghurtgl")

add_executable (pacman "pacman.cpp"  "game/pacman-game.h" "game/pacman-game.cpp")
target_link_libraries(pacman PRIVATE YoghurtGL)
set_target_properties(pacman PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../../../
)

target_link_options(pacman PRIVATE ${COMPILE_ARGS})
#target_link_options(pacman PRIVATE ${LINK_ARGS})


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET pacman PROPERTY CXX_STANDARD 20)
endif()

