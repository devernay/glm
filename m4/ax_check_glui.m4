# @synopsis AX_CHECK_GLUI
#
# Check for GLUI.  If GLUI is found, the required compiler and linker flags
# are included in the output variables "GLUI_CFLAGS" and "GLUI_LIBS",
# respectively. This macro adds the configure option
# "--with-apple-opengl-framework", which users can use to indicate that
# Apple's OpenGL framework should be used on Mac OS X. If Apple's OpenGL
# framework is used, the symbol "HAVE_APPLE_OPENGL_FRAMEWORK" is defined.  If
# GLUI is not found, "no_glui" is set to "yes".
#
# @version 1.8
# @author Braden McDaniel <braden@xxxxxxxxxxxxx>
#
AC_DEFUN([AX_CHECK_GLUI],
[AC_REQUIRE([AX_CHECK_GLUT])dnl
AC_REQUIRE([AX_CHECK_GL])dnl
AC_REQUIRE([AC_PATH_XTRA])dnl

GLUI_CFLAGS="${GLUT_CFLAGS} ${GL_CFLAGS}"
GLUI_LIBS="${GLUT_LIBS} ${GL_LIBS}"

AC_PROG_CXX
AC_LANG_PUSH(C++)

ax_save_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${GLUI_CFLAGS} ${CPPFLAGS}"

AC_CACHE_CHECK([for GLUI library], [ax_cv_check_glui_libglui],
[ax_cv_check_glui_libglui="no"
ax_save_LIBS="${LIBS}"
LIBS=""
ax_check_libs="-lglui32 -lglui"
for ax_lib in ${ax_check_libs}; do
  if test X$ax_compiler_ms = Xyes; then
    ax_try_lib=`echo $ax_lib | sed -e 's/^-l//' -e 's/$/.lib/'`
  else
    ax_try_lib="${ax_lib}"
  fi
  LIBS="${ax_try_lib} ${GLUI_LIBS} ${ax_save_LIBS}"
  AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([[
# if HAVE_WINDOWS_H && defined(_WIN32)
#   include <windows.h>
# endif
# include <GL/glui.h>]],
                    [[GLUI_Master.set_glutSpecialFunc( NULL );glutMainLoop()]])],
  [ax_cv_check_glui_libglui="${ax_try_lib}"; break])

done
LIBS=${ax_save_LIBS}
])
CPPFLAGS="${ax_save_CPPFLAGS}"
AC_LANG_POP(C++)

if test "X${ax_cv_check_glui_libglui}" = "Xno"; then
  no_glui="yes"
  GLUI_CFLAGS=""
  GLUI_LIBS=""
else
  GLUI_LIBS="${ax_cv_check_glui_libglui} ${GLUI_LIBS}"
fi

AC_SUBST([GLUI_CFLAGS])
AC_SUBST([GLUI_LIBS])
])#

