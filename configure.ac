#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.69)
AC_INIT(cip-server, 0.1, gd@cloudwarehub.com)
AM_INIT_AUTOMAKE
#AC_CONFIG_SRCDIR([src/cip_server.c])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(uv, uv_run,
    [],
    [AC_MSG_ERROR([libuv libraries not found.])]
)

AC_CHECK_LIB(x264, x264_encoder_encode,
    [],
    [AC_MSG_ERROR([x264 libraries not found.])]
)

AC_CHECK_LIB([X11-xcb],[],AC_MSG_ERROR([ *** Unable to find xcb library]))

AC_CHECK_LIB([xcb-composite],[],AC_MSG_ERROR([ *** Unable to find xcb-composite library]))

AC_CHECK_LIB([xcb-damage],[],AC_MSG_ERROR([ *** Unable to find xcb-damage library]))

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
