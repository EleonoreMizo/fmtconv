AC_INIT([fmtconv], [r11], [http://forum.doom9.org/showthread.php?t=166504], [fmtconv], [http://forum.doom9.org/showthread.php?t=166504])

: ${CXXFLAGS=""}

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz subdir-objects no-define])
AM_SILENT_RULES([yes])

LT_INIT([win32-dll disable-static])

AC_PROG_CXX

AC_CANONICAL_HOST


X86="false"
PPC="false"
ARM="false"

AS_CASE(
        [$host_cpu],
        [i?86],   [BITS="32" X86="true"],
        [x86_64], [BITS="64" X86="true"],
        [powerpc*], [PPC="true"],
        [arm*], [ARM="true"] # Maybe doesn't work for all arm systems?
)

AS_CASE(
        [$host_os],
        [cygwin*|mingw*],
        [
         AS_IF(
               [test "x$BITS" = "x32"],
               [
                AC_SUBST([PLUGINLDFLAGS], ["-Wl,--kill-at"])
               ]
         )
        ]
)

AS_IF(
      [test "x$X86" = "xtrue"],
      [
       AC_SUBST([MFLAGS], ["-mfpmath=sse -msse2"])
      ]
)


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
