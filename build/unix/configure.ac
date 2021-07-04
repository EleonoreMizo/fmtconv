AC_INIT([fmtconv], [r22], [http://forum.doom9.org/showthread.php?t=166504], [fmtconv], [http://forum.doom9.org/showthread.php?t=166504])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz subdir-objects no-define])
AM_SILENT_RULES([yes])

LT_INIT([win32-dll disable-static])

: ${CXXFLAGS=""}

AC_PROG_CXX

AC_CANONICAL_HOST

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [Compilation options required for debugging. [default=no]]))

AC_LANG_PUSH([C++])
AS_IF([test "x$CXXSTD" = "x"], AX_CHECK_COMPILE_FLAG([-std=c++17], [CXXSTD="c++17"]))
AS_IF([test "x$CXXSTD" = "x"], AX_CHECK_COMPILE_FLAG([-std=c++14], [CXXSTD="c++14"]))
AS_IF([test "x$CXXSTD" = "x"], AC_MSG_ERROR([Minimum requirement: C++14]))
AX_CHECK_COMPILE_FLAG([-Wunused-private-field]        , [CXXFLAGS="$CXXFLAGS -Wno-unused-private-field"]        , , [-Werror])
AX_CHECK_COMPILE_FLAG([-Wunused-command-line-argument], [CXXFLAGS="$CXXFLAGS -Wno-unused-command-line-argument"], , [-Werror])
AX_CHECK_COMPILE_FLAG([-Wexpansion-to-defined]        , [CXXFLAGS="$CXXFLAGS -Wno-expansion-to-defined"]        , , [-Werror])
AC_LANG_POP([C++])

# It seems that -latomic is needed only for some versions of GCC < 5.3
AX_CHECK_LINK_FLAG([-latomic], [LIBS="$LIBS -latomic"])

AS_IF(
    [test "x$enable_debug" = "xyes"],
    [DEBUGCFLAGS="-O0 -g3 -ggdb"],
    [DEBUGCFLAGS="-O3 -g3 -DNDEBUG"]
)

X86="false"
PPC="false"
ARM="false"

AS_CASE(
    [$host_cpu],
    [i?86],     [BITS="32" X86="true"],
    [x86_64],   [BITS="64" X86="true"],
    [powerpc*], [          PPC="true"],
    [arm*],     [BITS="32" ARM="true"],
    [aarch64*], [BITS="64" ARM="true"]
)

AS_CASE(
    [$host_os],
    [cygwin*|mingw*],
    [AS_IF(
        [test "x$BITS" = "x32"],
        [
            PLUGINLDFLAGS="-Wl,--kill-at"
            STACKREALIGN="-mstackrealign"
        ]
    )]
)

AS_IF(
    [test "x$X86" = "xtrue"],
    [
        MFLAGS="-mfpmath=sse -msse2 -Wno-ignored-attributes"

        # We need this to use CMPXCHG16B for 2x64-bit CAS (compare and swap)
        AS_IF([test "x$BITS" = "x64"], [MFLAGS="$MFLAGS -mcx16"])
    ]
)

AS_IF(
    [test "x$ARM" = "xtrue"],
    [
        MFLAGS="-ftree-vectorize"

        AX_CHECK_COMPILE_FLAG([-mfpu=neon], [MFLAGS="$MFLAGS -mfpu=neon"])

        # GCC 7 emits some warnings about ABI changes when using std::vector
        AX_CHECK_COMPILE_FLAG([-Wpsabi -Werror], [MFLAGS="$MFLAGS -Wno-psabi"])
    ]
)

AC_SUBST([MFLAGS])
AC_SUBST([DEBUGCFLAGS])
AC_SUBST([CXXSTD])
AC_SUBST([PLUGINLDFLAGS])
AC_SUBST([STACKREALIGN])

AM_CONDITIONAL([X86], [test "x$X86" = "xtrue"])
AM_CONDITIONAL([ARM], [test "x$ARM" = "xtrue"])


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
