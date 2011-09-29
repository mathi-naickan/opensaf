#------------------------------------------------------------------------
#SUN MICROSYSTEMS, INC. THROUGH ITS SUN MICROSYSTEMS LABORATORIES
#DIVISION ("SUN") WILL LICENSE THIS SOFTWARE AND THE ACCOMPANYING
#DOCUMENTATION TO YOU (a "Licensee") ONLY ON YOUR ACCEPTANCE OF ALL 
#THE TERMS SET FORTH BELOW.
#
#Sun grants Licensee a non-exclusive, royalty-free right to download,
#install, compile, use, copy and distribute the Software, modify or
#otherwise create derivative works from the Software (each, a
#"Modification") and distribute any Modification in source code and/or
#binary code form to its customers with a license agreement containing
#these terms and noting that the Software has been modified. The
#Software is copyrighted by Sun and other third parties and Licensee
#shall retain and reproduce all copyright and other notices presently 
#on the Software. As between Sun and Licensee, Sun is the sole owner of
#all rights in and to the Software other than the limited rights
#granted to Licensee herein; Licensee will own its Modifications,
#expressly subject to Sun's continuing ownership of the
#Software. Licensee will, at its expense, defend and indemnify Sun and
#its licensors from and against any third party claims, including costs
#and reasonable attorneys' fees, and be wholly responsible for any
#liabilities arising out of or related to Licensee's development, use
#or distribution of the Software or Modifications. Any distribution of
#the Software and Modifications must comply with all applicable United
#States export control laws.
#
#THE SOFTWARE IS BEING PROVIDED TO LICENSEE "AS IS" AND ALL EXPRESS OR
#IMPLIED CONDITIONS AND WARRANTIES, INCLUDING ANY IMPLIED WARRANTY OF
#MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT,
#ARE DISCLAIMED. IN NO EVENT WILL SUN BE LIABLE HEREUNDER FOR ANY
#DIRECT DAMAGES OR ANY INDIRECT, PUNITIVE, SPECIAL, INCIDENTAL OR
#CONSEQUENTIAL DAMAGES OF ANY KIND.
#------------------------------------------------------------------------
#------------------------------------------------------------------------
# AC_JAVA_ANT
#
#  Figure out if ant is available and where
#
#  Arguments:
#     PATH
#
#  VARIABLES SET:
#     ANT
#
#------------------------------------------------------------------------

AC_DEFUN([AC_JAVA_ANT], [
   AC_ARG_WITH(ant,
      AC_HELP_STRING([--with-ant=DIR],[Use ant from DIR]),
      ANTPATH=$withval, ANTPATH=no)
   if test "$ANTPATH" = "no" ; then
      AC_JAVA_TOOLS_CHECK(ANT, ant)
   elif test ! -d "$ANTPATH"; then
      AC_MSG_ERROR([--with-ant=DIR option, must pass a valid DIR])
   else
      AC_JAVA_TOOLS_CHECK(ANT, ant, $ANTPATH/bin $ANTPATH)
   fi
])

#------------------------------------------------------------------------
# AC_JAVA_TOOLS_CHECK(VARIABLE, TOOL, PATH, NOERR)
#
# Helper function that will look for the given tool on the
# given PATH. If cross compiling and the tool can not
# be found on the PATH, then search for the same tool
# on the users PATH. If the tool still can not be found
# then give up with an error unless NOERR is 1.
#
# Arguments:
# 1. The variable name we pass to AC_PATH_PROG
# 2. The name of the tool
# 3. The path to search on
# 4. Pass 1 if you do not want any error generated
#------------------------------------------------------------------------

AC_DEFUN([AC_JAVA_TOOLS_CHECK], [
   if test "$cross_compiling" = "yes" ; then
      AC_PATH_PROG($1, $2)
   else
      AC_PATH_PROG($1, $2, , $3)
   fi

   # Check to see if $1 could not be found

   m4_ifval([$4],,[
   if test "x[$]$1" = "x" ; then
      AC_MSG_ERROR([Cannot find $2])
   fi
   ])
])

#------------------------------------------------------------------------
# AC_JAVA_WITH_JDK
#
#  Check to see if the --with-jdk command line option is given.
#  If it was, then set ac_java_with_jdk to the DIR argument.
#
# Arguments:
#  NONE
#
# VARIABLES SET:
#  ac_java_with_dir can be set to the directory where the jdk lives
#  ac_java_jvm_name can be set to "jdk"
#------------------------------------------------------------------------

AC_DEFUN([AC_JAVA_WITH_JDK], [
   AC_ARG_WITH(jdk,
      AC_HELP_STRING([--with-jdk=DIR],[use JDK from DIR]),
      ok=$withval, ok=no)
   if test "$ok" = "no" ; then
      NO=op
   elif test "$ok" = "yes" || test ! -d "$ok"; then
      AC_MSG_ERROR([--with-jdk=DIR option, must pass a valid DIR])
   elif test "$ok" != "no" ; then
      AC_MSG_RESULT([Use JDK path specified ($ok)])
      ac_java_jvm_dir=$ok
      ac_java_jvm_name=jdk
   fi
])

#------------------------------------------------------------------------
# AC_PROG_JAVAC
#
#  If JAVAC is not already defined, then search for "javac" on
#  the path.
#
# Arguments:
#  NONE
#
# VARIABLES SET:
#  JAVAC can be set to the path name of the java compiler
#  ac_java_jvm_dir can be set to the jvm's root directory
#------------------------------------------------------------------------

AC_DEFUN([AC_PROG_JAVAC], [
   if test "x$JAVAC" = "x" ; then
      AC_PATH_PROG(JAVAC, javac)
      if test "x$JAVAC" = "x" ; then
         AC_MSG_ERROR([javac not found on PATH ... did you try with --with-jdk=DIR])
      fi
   fi
   if test ! -f "$JAVAC" ; then
      AC_MSG_ERROR([javac '$JAVAC' does not exist.
      Perhaps Java is not installed or you passed a bad dir to a --with option.])
   fi

   # Check for installs which uses a symlink. If it is the case, try to resolve
   # JAVA_HOME from it
   if test -h "$JAVAC" -a "x$DONT_FOLLOW_SYMLINK" != "xyes"; then
      FOLLOW_SYMLINKS($JAVAC,"javac")
      JAVAC=$SYMLINK_FOLLOWED_TO
      TMP=`dirname $SYMLINK_FOLLOWED_TO`
      TMP=`dirname $TMP`
      ac_java_jvm_dir=$TMP
         echo "Java base directory (probably) available here : $ac_java_jvm_dir"
   fi

   # If we were searching for javac, then set ac_java_jvm_dir
   if test "x$ac_java_jvm_dir" = "x"; then
      TMP=`dirname $JAVAC`
      TMP=`dirname $TMP`
      ac_java_jvm_dir=$TMP
   fi
])

#------------------------------------------------------------------------
# FOLLOW_SYMLINKS <path>
#  Follows symbolic links on <path>,
#
# Arguments:
#  the file which want to resolve the real file
#  the real name of the file (for example, if we are looking for javac and
#  it is a symlink to /usr/bin/gcj, we want to stay in the java dir where
#  javac is found)
#
# VARIABLES SET:
#  SYMLINK_FOLLOWED_TO the "real" file
#------------------------------------------------------------------------

AC_DEFUN([FOLLOW_SYMLINKS],[
   # find the include directory relative to the executable
   _cur="$1"
   if test ! -z "$2"; then
      _fileNameWanted=$2
   else
      _fileNameWanted=$1
   fi
   while ls -ld "$_cur" 2>/dev/null | grep " -> " >/dev/null; do
      AC_MSG_CHECKING(Symlink for $_cur)

      _slink=`ls -ld "$_cur" | sed 's/.* -> //'`
      if test "$_fileNameWanted" != "`basename $_slink`"; then
         AC_MSG_RESULT(Filename changed... Keeping the one found before)
         break
      fi
      case "$_slink" in
         /*)
            _cur="$_slink";;
         # 'X' avoids triggering unwanted echo options.
         *)
            _cur=`echo "X$_cur" | sed -e 's/^X//' -e 's:[[^/]]*$::'`"$_slink";;
      esac
      AC_MSG_RESULT($_cur)
   done
   SYMLINK_FOLLOWED_TO="$_cur"
])

#------------------------------------------------------------------------
# AC_PROG_JAVADOC
#
#  If JAVADOC is not already defined, then search for "javadoc" on
#  the path.
#
# Arguments:
#  NONE
#
# VARIABLES SET:
#  JAVADOC can be set to the path name of the javadoc tool
#------------------------------------------------------------------------

AC_DEFUN([AC_PROG_JAVADOC], [
   if test "x$JAVADOC" = "x" ; then
      AC_PATH_PROG(JAVADOC, javadoc)
      if test "x$JAVADOC" = "x" ; then
         AC_MSG_ERROR([javadoc not found on PATH ... did you try with --with-jdk=DIR])
      fi
   fi
   if test ! -f "$JAVADOC" ; then
      AC_MSG_ERROR([javadoc '$JAVADOC' does not exist.
      Perhaps Java is not installed or you passed a bad dir to a --with option.])
   fi

   if test -h "$JAVADOC" -a "x$DONT_FOLLOW_SYMLINK" != "xyes"; then
      FOLLOW_SYMLINKS($JAVADOC,"javadoc")
      JAVADOC=$SYMLINK_FOLLOWED_TO
   fi
])

#------------------------------------------------------------------------
# AC_PROG_AM4J_API
#
#  If AM4J_API is not already defined, then search for "am4j-api-1.0.jar" in
#  the jdk lib path.
#
# Arguments:
#  NONE
#
# VARIABLES SET:
#  with_am4j_api_jar can be set to the path name of the AM4J API JAR or to no
#------------------------------------------------------------------------

AC_DEFUN([AC_PROG_AM4J_API], [
   if test "x$with_am4j_api_jar" = "xno" ; then
      AC_PATH_PROG(AM4J_API_JAR, am4j-api-1.0.jar, , $ac_java_vm_dir/lib)
      if test "x$AM4J_API_JAR" = "x" ; then
         AC_MSG_ERROR([am4j-api-1.0.jar not found on PATH ... did you try with --with-am4j-api-jar=FILE])
      fi
   fi

   AM4J_API=$with_am4j_api_jar

   if test ! -f $AM4J_API_JAR ; then
      AC_MSG_ERROR([am4j-api '$AM4J_API_JAR' does not exist.
      Perhaps AM4J API is not installed or you passed a bad file to a --with option.])
   fi

   if test -h "$AM4J_API_JAR" -a "x$DONT_FOLLOW_SYMLINK" != "xyes"; then
      FOLLOW_SYMLINKS($AM4J_API_JAR, "am4j-api-1.0.jar")
      AM4J_API_JAR=$SYMLINK_FOLLOWED_TO
   fi
])
