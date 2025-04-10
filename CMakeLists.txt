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
	set(COMPILE_ARGS -Wall -std=c++23 -O3)
else (NOT MSVC)
	set(COMPILE_ARGS -O3 -std=c++23)
endif (NOT MSVC)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("pacman")

set(YGL_STATIC ON)
set(YGL_NO_ASSIMP ON)
add_subdirectory("./lib/yoghurtgl")

if(WIN32)
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-O3")
	set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "")
else(WIN32)
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fPIC -stdlib=libc++ -O3")
	set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-fPIC")
endif (WIN32)

if (MSVC)
	set(COMPILE_ARGS /Wall)
else ()
	set(COMPILE_ARGS
		#-fsanitize=address
	)
	set(LINK_ARGS
		#-fsanitize=address
	)
	set(COMPILE_ARGS ${COMPILE_ARGS} -Wall -Wpedantic --pedantic-errors -g -O3 -std=c++2a)
endif ()

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

target_compile_options(pacman PRIVATE ${COMPILE_ARGS} -fdata-sections -ffunction-sections)
target_link_options(pacman PRIVATE ${COMPILE_ARGS} -fdata-sections -dead-strip -ffunction-sections -Wl,--gc-sections)
