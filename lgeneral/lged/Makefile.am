LGED_REINF_PATH = $(top_srcdir)/lgeneral-redit/src
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = lged

LIBS = $(top_builddir)/util/libutil.a
AM_CPPFLAGS = -I$(top_srcdir) -I$(LGED_REINF_PATH)
lged_CFLAGS = -DCMDLINE_ONLY -DTOP_SRCDIR=\"$(top_srcdir)\"
lged_LDFLAGS =
lged_SOURCES = lged.c \
	$(LGED_REINF_PATH)/misc.c \
	$(LGED_REINF_PATH)/parser.c \
	$(LGED_REINF_PATH)/list.c

