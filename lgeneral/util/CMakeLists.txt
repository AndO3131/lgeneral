include_directories ( . )

set ( util_src
  localize.c
  localize.h
  hashtable.c
  hashtable.h
  hashtable_private.h
  hashtable_itr.c
  hashtable_itr.h
  hashtable_utility.c
  hashtable_utility.h
  paths.c
  paths.h
#  portability.c
)

# Create a library called "util" which includes the source files listed above.
# The extension is already found.
add_library (util ${util_src})

# Make sure the compiler can find include files for our "util" library
# when other libraries or executables link to it
target_include_directories (util PUBLIC ${CMAKE_SOURCE_DIR})

