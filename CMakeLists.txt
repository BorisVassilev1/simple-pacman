# CMakeList.txt : CMake project for pacman, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if (NOT MSVC)
	set(COMPILE_ARGS -Wall -std=c++20 -O3)
else (NOT MSVC)
	set(COMPILE_ARGS -O3 -std=c++20)
endif (NOT MSVC)
set(CMAKE_CXX_STANDARD 20)

project ("pacman")

set(YGL_STATIC ON)
set(YGL_NO_ASSIMP ON)
add_subdirectory("./lib/yoghurtgl")

add_executable (pacman "pacman.cpp" "game/pacman-game.h" "game/pacman-game.cpp")
add_definitions(-DYGL_NO_ASSIMP)
target_link_libraries(pacman PRIVATE YoghurtGL)
if (MSVC)
	set_target_properties(pacman PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ../../../
	)
else (MSVC)
	set_target_properties(pacman PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ../
	)
	set_target_properties(YoghurtGL PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ../../../
	)
endif (MSVC)

target_link_options(pacman PRIVATE ${COMPILE_ARGS} -fdata-sections -dead-strip -ffunction-sections -Wl,--gc-sections)

#if (CMAKE_VERSION VERSION_GREATER 3.12)
#  set_property(TARGET pacman PROPERTY CXX_STANDARD 20)
#endif()

