AM_CFLAGS = $(WARN_CFLAGS)

noinst_PROGRAMS = smooth

smooth_CFLAGS = -I$(top_srcdir)/glm $(GLUT_CFLAGS)
smooth_SOURCES = dirent32.h gltb.c gltb.h gltx.c gltx.h smooth.c trackball.c trackball.h
smooth_LDADD = ../glm/libglm.la $(GLUT_LIBS)

