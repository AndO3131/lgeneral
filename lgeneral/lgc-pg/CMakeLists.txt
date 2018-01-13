#WARNING: All non-linux commands need checking

include_directories ( ../convdata )
include_directories ( . )

set ( root_src
  main.c
  units.c
  shp.c
  nations.c
  terrain.c
  maps.c
  scenarios.c
  list.c
  misc.c
  parser.c
)

set ( data_install_dir bin )
if ( CMAKE_COMPILER_IS_GNUCC AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
  add_definitions ( -DNDEBUG )
endif ()
if ( CMAKE_COMPILER_IS_GNUCC AND ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR ENABLE_WARNING) )
    # Enable more GCC warnings if requested or we are doing a Debug build.
    add_definitions ( -Wall
      -Wsign-compare
      -Winit-self
      -Wmissing-include-dirs
      -Wunknown-pragmas
      -Wmissing-format-attribute
      -Wredundant-decls
      -Winline
      -Wformat-security
      -Wtype-limits
      -Wclobbered
      -Wempty-body
      -Wlogical-op
      -Wuninitialized
      -Wshadow
# These two warning options actually generate way too many warnings. Should be enabled later.
# -Wignored-qualifiers
# -Weffc++
      )
    if ( FATAL_WARNING )
      add_definitions ( -Werror )
    endif ( FATAL_WARNING )
endif()

if ( WIN32 )
  set ( application_type WIN32 )
endif ()

set ( lgc_pg_src ${root_src} )

set ( install_dest RUNTIME )
set ( set_exec_path ON )
set ( install_dest_dir bin )
if ( UNIX AND NOT APPLE )
  if ( "${DATADIR}" STREQUAL "" )
    set ( data_install_dir "share/lgeneral" )
  else ()
    set ( data_install_dir ${DATADIR} )
    add_definitions( -DDATADIR="${data_install_dir}/" )
  endif ()
endif ()

if ( set_exec_path )
  set ( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/lgc-pg )
endif ()

if ( FORCE_INSTALL_DATA_TO_BIN )
  set ( data_install_dir bin )
endif ()

add_executable ( lgc-pg ${application_type} ${lgc_pg_src} )
install ( TARGETS lgc-pg ${install_dest} DESTINATION bin )
# Extra link flags for Windows. They need to be set before the SDL/YAML link flags, otherwise you will get strange link errors ('Undefined reference to WinMain@16')
if ( WIN32 )
  set ( basic_windows_libs advapi32.lib shell32.lib shlwapi.lib )
  if ( MINGW )
    set ( basic_windows_libs ${basic_windows_libs} mingw32 -mwindows )
    set ( static_flags -static )
  endif ()
  set ( system_libs ${basic_windows_libs} SDLmain ${static_flags} )
endif ()
target_link_libraries ( lgc-pg util -lm ${system_libs} ${SDL_LIBRARY} )

add_custom_command ( TARGET lgc-pg
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/lgc-pg/convdata ${EXECUTABLE_OUTPUT_PATH}/convdata
  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/convdata/Makefile* 
)
#install ( DIRECTORY ${CMAKE_SOURCE_DIR}/bin/data DESTINATION ${data_install_dir} )

# Copy Windows DLLs to bin folder
if ( WIN32 )
  if ( CMAKE_CL_64 )
    file ( GLOB Windows_dll "${DEPS_DIR}/lib/x64/*.dll" )
  else ( )
    file ( GLOB Windows_dll "${DEPS_DIR}/lib/Win32/*.dll" )
  endif()
  foreach ( dll ${Windows_dll} )
    add_custom_command ( TARGET lgc-pg
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${dll} ${EXECUTABLE_OUTPUT_PATH} )
    install ( FILES ${dll} DESTINATION bin)
  endforeach()
endif ()
