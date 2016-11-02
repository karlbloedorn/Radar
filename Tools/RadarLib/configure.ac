AC_INIT(RadarLib, 0.1, coagentpai@gmail.com)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(RadarLib/)
AM_INIT_AUTOMAKE([foreign])
LT_INIT()

AX_CHECK_COMPILE_FLAG([-std=c11],
  [AX_APPEND_FLAG([-std=c11])],
  [AC_MSG_ERROR([-std=c11 not supported, you may have to set CFLAGS to enable C11 support.])
])
AC_DEFINE([_XOPEN_SOURCE], [700],
          [Defined for pthread functions])

AC_OUTPUT(Makefile RadarLib/Makefile)

