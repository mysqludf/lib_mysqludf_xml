#####
#
# SYNOPSIS
#
#   AX_XML2()
#
# DESCRIPTION
#
#   This macro provides tests of availability of XML2 library and headers.
#
#   This macro calls:
#
#     AC_SUBST(XML2_LIBDIR)
#     AC_SUBST(XML2_LIBS)
#     AC_SUBST(XML2_INCLUDEDIR)
#
#####

AC_DEFUN([AX_XML2], [
  AC_MSG_CHECKING(for libxml2)
  AC_ARG_WITH(libxml2,
    [[  --with-libxml2[=PATH]   use libxml2 @<:@default=yes@:>@, optionally specify
                          prefix or path to xml2Conf.sh]],
    [withxml2="$withval"],
    [withxml2="yes"]
  )
  
  if test -z "$withxml2" -o "$withxml2" = "yes"; then
    for i in /usr /usr/local ; do
      if test -f "$i/lib/xml2Conf.sh" ; then
        XML2_CONFIG="$i/lib/xml2Conf.sh"
      elif test -f "$i/lib/libxml2.so"; then
        XML2_DIR="$i"
      fi
    done
    if test "XML2_CONFIG" = "" -a "$XML2_DIR" = ""; then
      AC_MSG_ERROR(["could not find libxml2"])
    fi
  elif test "$withxml2" = "no"; then
    AC_MSG_ERROR(["can not build with libxml2 disabled"])
  else
    if test `basename "$withxml2"` = "xml2Conf.sh"; then
      if test -f "$withxml2"; then
        XML2_CONFIG="$withxml2"
      else
        AC_MSG_ERROR(["could not find libxml2 config: $withxml2"])
      fi
    else
      if test -f "$withxml2/lib/libxml2.so"; then
        XML2_DIR="$withxml2"
      else
        AC_MSG_ERROR(["could not find libxml2: $withxml2/lib/libxml2.so"])
      fi
    fi
  fi

  if test "$XML2_CONFIG" != ""; then
    . "$XML2_CONFIG"
    AC_MSG_RESULT([$XML2_CONFIG])

  else
    XML2_LIBDIR="-L$XML2_DIR/lib"
    XML2_LIBS="-lxml2"
      
    for i in include include/libxml2; do
      if test -f "$XML2_DIR/$i/libxml"; then
        XML2_INCLUDEDIR="-I$XML2_DIR/$i"
      fi
    done 
    if test "$XML2_INCLUDEDIR" = ""; then
      AC_MSG_ERROR(["could not find headers for libxml2: $XML2_DIR/include/libxml or $XML2_DIR/include/libxml2/libxml"])
    fi

    AC_MSG_RESULT([$XML2_DIR])
  fi
    
  AC_DEFINE([XML2_ENABLED], [1], [Enables libxml2])
  
  AC_SUBST(XML2_LIBDIR)
  AC_SUBST(XML2_LIBS)
  AC_SUBST(XML2_INCLUDEDIR)
])
