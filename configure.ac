AC_INIT([glm],[0.3.2])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_C_INLINE

# Turn off shared libraries during beta-testing, since they
# make the build process take too long.
AC_DISABLE_SHARED
LT_INIT

AC_CHECK_FUNCS(strdup)

# if test $target_os = cygwin; then
#   AC_MSG_NOTICE([using Cygwin OpenGL])
#   GL_CFLAGS=
#   GL_LIBS='-lglu32 -lopengl32'
#   GLUT_CFLAGS='$(GL_CFLAGS)'
#   GLUT_LIBS=-lglut32
# else
  AX_CHECK_GLUT
# fi
AX_CHECK_GLUI
AM_CONDITIONAL(HAVE_GLUT, test "x$GLUT_LIBS" != x)
AM_CONDITIONAL(HAVE_GLUI, test "x$GLUI_LIBS" != x)


# Test for libjpeg
AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
  jpeg_ok=yes,
  jpeg_ok=no
  AC_MSG_WARN([*** Unable to find JPEG library (http://www.ijg.org/)***]), -lm)
if test "$jpeg_ok" = yes; then
  AC_MSG_CHECKING([for jpeglib.h])
  AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#include <jpeglib.h>]])],[jpeg_ok=yes],[jpeg_ok=no])
  AC_MSG_RESULT($jpeg_ok)
  if test "$jpeg_ok" = yes; then 
    JPEGLIBS="-ljpeg"
    SUPPORT_LIBS="$SUPPORT_LIBS -ljpeg";
    AC_DEFINE(HAVE_LIBJPEG, 1, [Define if you have the IJG JPEG library])
  else
    AC_MSG_WARN([JPEG image loading disabled])
  fi
fi
AC_SUBST(JPEGLIBS)

# Test for libpng
AC_CHECK_LIB(png, png_create_read_struct, have_libpng=yes, , -lz)
if test x$have_libpng != xyes; then
    AC_MSG_WARN([*** Unable to find PNG library (http://www.libpng.org/pub/png/libpng.html)])
fi
AC_CHECK_LIB(z, uncompress, have_libz=yes)
if test x$have_libz != xyes; then
    AC_MSG_WARN([*** Unable to find Zlib library (http://www.gzip.org/zlib/)])
fi
if test x$have_libpng = xyes -a x$have_libz = xyes; then
    PNGLIBS="-lpng -lz"
    SUPPORT_LIBS="$SUPPORT_LIBS -lpng -lz"
    AC_DEFINE(HAVE_LIBPNG, 1, [Define if you have the PNG library])
else
    AC_MSG_WARN([PNG image loading disabled])
fi

# Test for libSDL_image
ac_sdl_image_desired=false
AC_ARG_WITH(
  sdl-image,
  AS_HELP_STRING([--with-sdl-image=DIR],[use sdl-image for loading texture files]),
  [case $withval in
   yes) ac_sdl_image_desired=true ;;
   no)  ac_sdl_image_desired=false ;;
   *)   ac_sdl_image_desired=true
        ac_sdl_image_extrapath=$withval ;;
  esac],
  [])
if [ $ac_sdl_image_desired = true ]; then
  AC_CHECK_LIB(SDL_image, IMG_Load,
    SDL_image_ok=yes,
    SDL_image_ok=no
    AC_MSG_WARN([*** SDL_IMAGE library not found ***]), -lm)
  if test "$SDL_image_ok" = yes; then
    AC_MSG_CHECKING([for SDL/SDL_image.h])
    AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#include <SDL/SDL_image.h>]])],[SDL_image_ok=yes],[SDL_image_ok=no])
    AC_MSG_RESULT($SDL_image_ok)
    if test "$SDL_image_ok" = yes; then 
      SDL_IMAGELIBS="-lSDL_image"
      SUPPORT_LIBS="$SUPPORT_LIBS -lSDL_image";
      AC_DEFINE(HAVE_LIBSDL_IMAGE, 1, [Define if you have the SDL_image library])
    else
      AC_MSG_WARN([*** SDL_IMAGE header file not found ***])
    fi
  fi
fi
AC_SUBST(SDL_IMAGELIBS)

# Test for DevIL
ac_devil_desired=true
AC_ARG_WITH(
  devil,
  AS_HELP_STRING([--with-devil=DIR],[use devil for loading texture files]),
  [case $withval in
   yes) ac_devil_desired=true ;;
   no)  ac_devil_desired=false ;;
   *)   ac_devil_desired=true
        ac_devil_extrapath=$withval ;;
  esac],
  [])
if [ $ac_devil_desired = true ]; then
  AC_CHECK_LIB(IL, ilInit,
    DevIL_ok=yes,
    DevIL_ok=no
    AC_MSG_WARN(*** DevIL library not found ***), -lm)
  if test "$DevIL_ok" = yes; then
    AC_MSG_CHECKING([for IL/il.h])
    AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#include <IL/il.h>]])],[DevIL_ok=yes],[DevIL_ok=no])
    AC_MSG_RESULT($DevIL_ok)
    if test "$DevIL_ok" = yes; then 
      DEVILLIBS="-lIL"
      SUPPORT_LIBS="$SUPPORT_LIBS -lIL";
      AC_DEFINE(HAVE_DEVIL, 1, [Define if you have the DevIL library])
    else
      AC_MSG_WARN([*** DevIL header file not found ***])
    fi
  fi
fi
AC_SUBST(DEVILLIBS)

# Test for simage (www.coin3d.org)
sim_ac_simage_desired=false
SIM_AC_HAVE_SIMAGE_IFELSE(
	SIMAGE_IMAGELIBS="$sim_ac_simage_libs"
	SUPPORT_LIBS="$SUPPORT_LIBS $SIMAGE_IMAGELIBS";
	AC_DEFINE(HAVE_LIBSIMAGE, 1, [Define if you have the simage library])
	,
	AC_MSG_WARN([*** Not using the simage library for loading textures (http://www.coin3d.org)])
	)

AM_CONDITIONAL(HAVE_GIF, test "x$GIFLIBS" != x)
AM_CONDITIONAL(HAVE_TIFF, test "x$TIFFLIBS" != x)
# AM_CONDITIONAL(HAVE_PNG, test "x$PNGLIBS" != x)
# AM_CONDITIONAL(HAVE_JPEG, test "x$JPEGLIBS" != x)
AM_CONDITIONAL(HAVE_SDL_IMAGE, test "x$SDL_IMAGELIBS" != x)

AC_SUBST(SUPPORT_LIBS)

AC_CONFIG_FILES([ Makefile
glm/Makefile
examples/Makefile
m4/Makefile
glm.spec
])
AC_OUTPUT


