#ifndef CONFIG_H
#define CONFIG_H

#define BEOS
// #define LIBBSVG 
#define BUILD_TIME __DATE__
/* Version number of package */
#define VERSION "1.8.2"
/* architecture of build machine */
#define UNIX_ARCHITECTURE "BePC-Haiku"

/* config.h.  Generated by configure.  */
/* config.h.in.  Generated from configure.in by autoheader.  */


/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

/* Define to 1 if you have the <ctype.h> header file. */
#define HAVE_CTYPE_H 1

/* defined, if ncurses.h is present */
#define HAVE_CURSES_HEADER 1

/* Define to 1 if you have the `difftime' function. */
#define HAVE_DIFFTIME 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <float.h> header file. */
#define HAVE_FLOAT_H 1

/* Define to 1 if you have the `floor' function. */
#define HAVE_FLOOR 1

/* Define to 1 if you have the `fork' function. */
#define HAVE_FORK 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `ncurses' library (-lncurses). */
#define HAVE_LIBNCURSES 0

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <math.h> header file. */
#define HAVE_MATH_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* defined, if ncurses.h is present */
#define HAVE_NCURSES_HEADER 0

/* Define to 1 if you have the `pow' function. */
#define HAVE_POW 1

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the `sqrt' function. */
#define HAVE_SQRT 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
/* #undef HAVE_STDINT_H */

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the `strftime' function. */
#define HAVE_STRFTIME 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* defined, if strings.h is present */
#define HAVE_STRINGS_HEADER 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* defined, if string.h is present */
#define HAVE_STRING_HEADER 1

/* Define to 1 if you have the `strpbrk' function. */
#define HAVE_STRPBRK 1

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the `tmpnam' function. */
#define HAVE_TMPNAM 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `vfork' function. */
/* #undef HAVE_VFORK */

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef HAVE_VFORK_H */

/* Define to 1 if `fork' works. */
#define HAVE_WORKING_FORK 1

/* Define to 1 if `vfork' works. */
/* #undef HAVE_WORKING_VFORK */

/* Name of package */
#define PACKAGE "yabasic"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to the type of arg 1 for `select'. */
#define SELECT_TYPE_ARG1 int

/* Define to the type of args 2, 3 and 4 for `select'. */
#define SELECT_TYPE_ARG234 (fd_set *)

/* Define to the type of arg 5 for `select'. */
#define SELECT_TYPE_ARG5 (struct timeval *)

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
        STACK_DIRECTION > 0 => grows toward higher addresses
        STACK_DIRECTION < 0 => grows toward lower addresses
        STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1



/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define as `fork' if `vfork' does not work. */
#define vfork fork

#endif
