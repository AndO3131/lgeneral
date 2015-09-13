#undef ENABLE_NLS
#undef HAVE_CATGETS
#undef HAVE_GETTEXT
#undef HAVE_LC_MESSAGES
#undef HAVE_STPCPY
#undef HAVE_LIBSM
#undef PACKAGE_LOCALE_DIR
#undef PACKAGE_DOC_DIR
#undef PACKAGE_DATA_DIR
#undef PACKAGE_PIXMAPS_DIR
#undef PACKAGE_HELP_DIR
#undef PACKAGE_MENU_DIR
#undef PACKAGE_SOURCE_DIR

@BOTTOM@

#ifdef HAVE_BROKEN_MKDIR
#  include <direct.h>
#  define mkdir(s,p) _mkdir(s)
#endif
