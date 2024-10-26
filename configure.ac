AC_PREREQ([2.69])                  
AC_INIT([YourProjectName], [1.0], [your.email@example.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign]) 


AC_PROG_CC                          

PKG_CHECK_MODULES(VORBIS, [vorbis vorbisfile])
PKG_CHECK_MODULES(OGG, [ogg])
PKG_CHECK_MODULES(MPG123, [libmpg123])
PKG_CHECK_MODULES(SDL2, [sdl2])


AC_MSG_CHECKING([for required libraries])
if test "x$VORBIS_LIBS" = "x" || test "x$OGG_LIBS" = "x" || test "x$MPG123_LIBS" = "x" || test "x$SDL2_LIBS" = "x"; then
    AC_MSG_ERROR([Some required libraries are missing.])
else
    AC_MSG_RESULT([all required libraries found])
fi

AC_SUBST(VORBIS_CFLAGS)
AC_SUBST(VORBIS_LIBS)
AC_SUBST(OGG_CFLAGS)
AC_SUBST(OGG_LIBS)
AC_SUBST(MPG123_CFLAGS)
AC_SUBST(MPG123_LIBS)
AC_SUBST(SDL2_CFLAGS)
AC_SUBST(SDL2_LIBS)


AC_CONFIG_FILES([Makefile])       
AC_OUTPUT                         

