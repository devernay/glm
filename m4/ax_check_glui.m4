dnl @synopsis AX_CHECK_GLUI
dnl
dnl Check for GLUI.  If GLUI is found, the required compiler and linker flags
dnl are included in the output variables "GLUI_CFLAGS" and "GLUI_LIBS",
dnl respectively, and the shell variable "no_glut" is set to the empty
dnl string.  Also, if the header "GL/glut.h" is found, the symbol
dnl "HAVE_GL_GLUI_H" is defined; otherwise, if the header "GLUI/glut.h" is
dnl found, "HAVE_GLUI_H" is defined. If GLUI is not found, "no_glut" is
dnl set to "yes".
dnl
dnl @copyright (C) 2003 Braden McDaniel
dnl @license GNU GPL
dnl @version $Id$
dnl @author Braden McDaniel <braden@endoframe.com>
dnl
AC_DEFUN([AX_CHECK_GLUI],
[AC_REQUIRE([AX_CHECK_GLUT])dnl
AC_REQUIRE([AC_PATH_XTRA])dnl
GLUI_LIBS="${GLUT_LIBS}"
GLUI_CFLAGS="${GLUT_CFLAGS}"

AC_LANG_PUSH(C++)
  
ax_save_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="${GLUI_CFLAGS} ${CPPFLAGS}"

AC_CHECK_HEADERS([GL/glui.h glui.h], [break])

AC_CACHE_CHECK([for GLUI library], [ax_cv_check_glut_libglut],
[ax_cv_check_glut_libglut="no"
ax_save_LIBS="${LIBS}"
LIBS=""
ax_check_libs="-lglui32 -lglui"
for ax_lib in ${ax_check_libs}; do
  LIBS="${ax_lib} ${GLUI_LIBS} ${ax_save_LIBS}"
  AC_TRY_LINK([
# ifdef _WIN32
#   include <windows.h>
# endif
# ifdef HAVE_GL_GLUI_H
#   include <GL/glui.h>
# else
#   include <glui.h>
# endif
],
  [GLUI_Master.set_glutSpecialFunc( NULL );glutMainLoop()],
  [ax_cv_check_glui_libglui="${ax_lib}" break])
  
done
LIBS=${ax_save_LIBS}
])
CPPFLAGS="${ax_save_CPPFLAGS}"
AC_LANG_POP(C++)

if test "X${ax_cv_check_glui_libglui}" = "Xno"; then
  no_glut="yes"
else
  GLUI_LIBS="${ax_cv_check_glui_libglui} ${GLUI_LIBS}"
fi

AC_SUBST([GLUI_CFLAGS])
AC_SUBST([GLUI_LIBS])
])dnl
