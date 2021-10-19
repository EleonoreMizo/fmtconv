AC_INIT([fmtconv], [r26], [http://forum.doom9.org/showthread.php?t=166504], [fmtconv], [http://forum.doom9.org/showthread.php?t=166504])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz subdir-objects no-define])
AM_SILENT_RULES([yes])

LT_INIT([win32-dll disable-static])

: ${CXXFLAGS=""}
: ${CFLAGS=""}

AC_PROG_CXX
AC_PROG_CC

AC_CANONICAL_HOST

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [Compilation options required for debugging. [default=no]]))
AC_ARG_ENABLE([clang], AS_HELP_STRING([--enable-clang], [Use Clang as compiler along with libc++. [default=no]]))



X86="false"
PPC="false"
ARM="false"
WIN="false"
UNX="false"
MAC="false"
CLG="false"

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
    [cygwin*|mingw*],                [WIN="true"],
    [darwin*],                       [MAC="true"],
    [*linux*|gnu*|dragonfly*|*bsd*], [UNX="true"]
)

AS_IF(
    [test "x$enable_debug" = "xyes"],
    [
        DEBUGCFLAGS="-O0 -g3 -ggdb"
        AC_MSG_NOTICE([Debug mode enabled.])
    ],
    [DEBUGCFLAGS="-O3 -g3 -DNDEBUG"]
)

AS_IF(
    [test "x$enable_clang" = "xyes"],
    [
        CLG="true"
        CXX="clang++"
        CC="clang"
        LD="clang++"
        MFLAGS="$MFLAGS -fexperimental-new-pass-manager -mllvm -inline-threshold=1000"
        COMPWARNFLAGS=""
        AC_MSG_NOTICE([Using clang as compiler.])
    ],
    [COMPWARNFLAGS="-Wduplicated-cond -Wduplicated-branches -Wlogical-op"]
)

AC_LANG_PUSH([C++])
#AS_IF([test "x$CXXSTD" = "x"], AX_CHECK_COMPILE_FLAG([-std=c++20], [CXXSTD="c++20"]))
#AS_IF([test "x$CXXSTD" = "x"], AX_CHECK_COMPILE_FLAG([-std=c++17], [CXXSTD="c++17"]))
AS_IF([test "x$CXXSTD" = "x"], AX_CHECK_COMPILE_FLAG([-std=c++14], [CXXSTD="c++14"]))
AS_IF([test "x$CXXSTD" = "x"], AC_MSG_ERROR([Minimum requirement: C++14]))
AC_LANG_POP([C++])

# It seems that -latomic is needed only for some versions of GCC < 5.3
AX_CHECK_LINK_FLAG([-latomic], [LIBS="$LIBS -latomic"])

AS_IF(
    [test "x$WIN" = "xtrue"],
    [
        AS_IF(
            [test "x$BITS" = "x32"],
            [
                PLUGINLDFLAGS="-Wl,--kill-at"
                STACKREALIGN="-mstackrealign"
            ]
        )
    ]
)

AS_IF(
    [test "x$X86" = "xtrue"],
    [
        MFLAGS="$MFLAGS -mfpmath=sse -msse2"
        COMPWARNFLAGS="$COMPWARNFLAGS -Wno-ignored-attributes"
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
        AX_CHECK_COMPILE_FLAG([-Wpsabi], [COMPWARNFLAGS="$COMPWARNFLAGS -Wno-psabi"], , [-Werror])
    ]
)

AX_CHECK_COMPILE_FLAG([-Wmisleading-indentation]          , [COMPWARNFLAGS="$COMPWARNFLAGS -Wmisleading-indentation"]          , , [-Werror])
# Clang only
AX_CHECK_COMPILE_FLAG([-Wno-implicit-int-float-conversion], [COMPWARNFLAGS="$COMPWARNFLAGS -Wno-implicit-int-float-conversion"], , [-Werror])

AC_SUBST([CXXSTD])
AC_SUBST([EXTRA_CXXFLAGS])
AC_SUBST([LDFLAGS])
AC_SUBST([MFLAGS])
AC_SUBST([DEBUGCFLAGS])
AC_SUBST([PLUGINLDFLAGS])
AC_SUBST([STACKREALIGN])

AM_CONDITIONAL([X86], [test "x$X86" = "xtrue"])
AM_CONDITIONAL([ARM], [test "x$ARM" = "xtrue"])
AM_CONDITIONAL([UNX], [test "x$UNX" = "xtrue"])
AM_CONDITIONAL([WIN], [test "x$WIN" = "xtrue"])
AM_CONDITIONAL([CLG], [test "x$CLG" = "xtrue"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
