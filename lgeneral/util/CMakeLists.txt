#WARNING: All non-linux commands need checking

#include_directories ( .. )
#include_directories ( ../convdata )
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

# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found. Any number of sources could be listed here.
add_library (util ${util_src})

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
target_include_directories (util PUBLIC ${CMAKE_SOURCE_DIR})

