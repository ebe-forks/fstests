AC_PREREQ(2.53)
AC_INIT([fstests], 0.1, [mallum@o-hand.com])
AC_CONFIG_SRCDIR([test-x.c])

AM_INIT_AUTOMAKE()

AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

PKG_CHECK_MODULES(FREETYPE, freetype2)
PKG_CHECK_MODULES(XLIBS, x11 xext xft)
PKG_CHECK_MODULES(PANGO, pango pangoxft)
PKG_CHECK_MODULES(GTK, gtk+-2.0)

if test "x$GCC" = "xyes"; then
        GCC_FLAGS="-g -Wall"
fi

AC_SUBST(GCC_FLAGS)

# Checks for header files.
AC_OUTPUT([Makefile])
