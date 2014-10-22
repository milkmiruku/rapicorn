#!/bin/bash
# mkrelease.sh: Copyright (C) 2010 Tim Janik
#
# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0

MYVERSION="mkrelease version 20120801"

# === initial setup ===
SCRIPT_NAME="$0"
die() {
  estatus=1
  case "$1" in
  [0-9]|[0-9][0-9]|[0-9][0-9][0-9]) estatus="$1" ; shift ;;
  esac
  echo "$SCRIPT_NAME: $*" >&2
  exit $estatus
}
unset R_REVISION
function match() {	# perform wildcard string match: match "*.*" unmatched1 foo.dat unmatched2...
  pattern="$1" ; searchtext="$2"
  case "$searchtext" in
    $pattern) return 0 ;; # $pattern must be unquoted to allow *?[]
  esac
  return 1
}


# === Usage ===
usage() {
  cat <<-EOF
	Usage: `basename $SCRIPT_NAME` <command> [options]
	Commands:
	  commit-stamps		list stamp files affected by a commit
	  contributors		extract contributor C strings from NEWS files
	  ChangeLog		generate ChangeLog from git history
	  news			list commits since last release tag
	  upload		check and upload release tarball
	Options:
	  -h, --help		usage help
	  -v, --version		issue version
	  -E <FILE:VAR>         revision variable to increment during "upload"
	                        (e.g. configure.ac:MICRO)
	  -B <A,B,C...>		ignored names for "contributors"
	  --body		include commit body for "news"
	  -C <NEWS>		file with ignored C strings for "contributors"
	  --first-parent	use git log with --first-parent
	  -o output		output filename (ChangeLog)
	  -R <revision>		revision range for "ChangeLog" generation
	                        last release revision for "news" (auto)
	  -T <disttarball>	name of distribution tarball (from Makefile)
	  -U <remoteurl>	remote release URL (e.g. example.com:distdir)
	  -V <releaseversion>	release version (from Makefile)
	  -X			expect no strings to be found for "contributors"
	Configuration (Makefile):
	  MKRELEASE_UPLOAD_URL	remote release URL
	  MKRELEASE_REVISION_VAR
				revision variable to increment during "upload"
EOF
  [ -z "$1" ] || exit $1
}

# === cmdline parsing ===
unset COMMAND
VERSION=_parse
TARBALL=_parse
BODY=false
REMOTE_URL=_parse
REVISIONVAR=_parse
CONTRBLACK=
CONTRCFILE=/dev/null
CONTREXIT=0
OUTPUT=ChangeLog
FIRST=
parse_options=1
while test $# -ne 0 -a $parse_options = 1; do
  case "$1" in
    -h|--help)	usage 0 ;;
    --first-parent) FIRST=--first-parent ;;
    -B)		CONTRBLACK="$2" ; shift ;;
    --body)	BODY=true ;;
    -C)		CONTRCFILE="$2" ; shift ;;
    -E)		REVISIONVAR="$2" ; shift ;;
    -o)		OUTPUT="$2" ; shift ;;
    -R)		R_REVISION="$2" ; shift ;;
    -T)		TARBALL="$2" ; shift ;;
    -U)		REMOTE_URL="$2" ; shift ;;
    -V)		VERSION="$2" ; shift ;;
    -X)		CONTREXIT=1 ;;
    -v|--version) echo "$MYVERSION" ; exit 0 ;;
    --)		parse_options=0 ;;
    *)		[ -z "$COMMAND" ] || usage 1
		COMMAND="$1"
		;;
  esac
  shift
done

# === commit-stamps ===
[ "$COMMAND" = "commit-stamps" ] && {
  # echo stamp files touched by commits
  [ -e "${GIT_DIR:-.git}" ] && \
    ls "${GIT_DIR:-.git}/`git symbolic-ref -q HEAD || echo HEAD`" \
       "${GIT_DIR:-.git}/packed-refs" \
       "${GIT_DIR:-.git}/HEAD" \
       2>/dev/null
  exit
}

# === ChangeLog ===
[ "$COMMAND" = "ChangeLog" ] && {
  TEMPF="`mktemp -t yyTMPlog.$$XXXXXX`" && touch $TEMPF || \
    die 9 "Failed to create temporary file"
  trap "rm -f $TEMPF" 0 HUP INT QUIT TRAP USR1 PIPE TERM
  # Generate ChangeLog with -prefixed records
  git log $FIRST --date=short --pretty='%ad  %an 	# %h%n%n%B%n' --abbrev=11 ${R_REVISION:-HEAD} \
  | {
    # Tab-indent ChangeLog, except for record start
    sed 's/^/	/; s/^	//; /^[ 	]*<unknown>$/d'
  } | {
    # Kill trailing whitespaces
    sed 's/[[:space:]]\+$//'
  } | {
    # Compress multiple newlines
    sed '/^\s*$/{ N; /^\s*\n\s*$/D }'
  } > $TEMPF
  # replace atomically
  mv $TEMPF "$OUTPUT"
  exit
}

# === contributors ===
[ "$COMMAND" = "contributors" ] && {
  TEMPF="`mktemp -t yyTMPcontribs.$$XXXXXX`" && touch $TEMPF || \
    die 9 "Failed to create temporary file"
  # find and extract contributor names from first NEWS section
  sed -n '/^[A-Za-z0-9]/{
	    # loop over content lines of first NEWS section
	    :1 n;
	    /^[A-Za-z0-9]/q;
	    /\[.*\]/{
	      # extract names, enclosed in brackets
	      s/\(^.*\[\|][^]]*$\)//g;
	      # separate list of names by newlines
	      s/[,;\/]/\n/g;
	      # remove excessive whitespaces
	      s/\(^\s\+\|\s\+$\)//g ; s/\s\s\+/ /g
	      p; };
	    b 1; }' < NEWS | sort | uniq > $TEMPF
  # list unknown contributor names as C strings
  EX=0
  while read NAME ; do
    case ",$CONTRBLACK," in (*",$NAME,"*) continue ;; esac
    grep -qFie \""$NAME"\" "$CONTRCFILE" || { echo "  \"$NAME\"," ; EX=$CONTREXIT ; }
  done < $TEMPF
  exit $EX
}

# === news ===
[ "$COMMAND" = "news" ] && {
  # list numeric tags
  TAGS=`git tag -l | grep '^[0-9.]*$'`
  # collect release tags in linear history
  if [ "${#R_REVISION[@]}" = 0 ] ; then # R_REVISION is unset
    XCOMMIT=	# prune commit, newest commit reachable from previous releases (merge base with HEAD)
    for t in $TAGS ; do
      # release tag?
      if git cat-file tag $t | sed '1,/^$/d' | head -n1 | grep -qi '\breleased\?\b' ; then
	TCOMMIT=`git rev-list -n1 $t`		# commit ID from tag
	MCOMMIT=`git merge-base HEAD $t`	# find history reachable from tag
        # update XCOMMIT if MCOMMIT is newer
	[ -n "$MCOMMIT" ] && {
	  [ -z "$XCOMMIT" ] || [ "`git merge-base $XCOMMIT $MCOMMIT`" = $XCOMMIT ] && XCOMMIT="$MCOMMIT"
	}
      fi
    done
  else
    XCOMMIT="$R_REVISION"
  fi
  [ -n "$XCOMMIT" ] && XCOMMIT="`git name-rev --tags --always --name-only $XCOMMIT`" # beautify
  [ -n "$XCOMMIT" ] && XCOMMIT="$XCOMMIT^!" # turn into exclude pattern
  # list news, excluding history reachable from previous releases
  echo "# git log $FIRST --reverse HEAD $XCOMMIT"
  if $BODY ; then
    git log $FIRST --date=short --pretty='%s    # %cd %an %h%d%n%b' --reverse HEAD $XCOMMIT | sed '/^$/d'
  else
    git log $FIRST --date=short --pretty='%s    # %cd %an %h%d' --reverse HEAD $XCOMMIT | cat
  fi
  exit
}

# === upload ===
[ "$COMMAND" = "upload" ] && {
  # functions
  check_makefile() {
    [ -r Makefile ] || die 7 "Failed to find and read Makefile"
  }
  parse_makefile_var() {
    VARNAME="$1"
    # find VARNAME, strip before '=', print and quit
    sed -ne "/^[ \t]*$VARNAME[ \t]*=/ { s/^[^=]*=[ \t]*//; p; q }" < Makefile
  }
  subst_makefile_vars() {
    sed -e "{	s/\$(VERSION)\|\${VERSION}/$VERSION/g ;
		s/\$(PACKAGE)\|\${PACKAGE}/$PACKAGE/g ;
		s/\$(distdir)\|${distdir}/$distdir/g ; }"
  }
  first_arg()	{ echo "$1" ; }
  msg_()  	{ printf "%-76s" "$@ " ; }
  msg()  	{ msg_ "  $@" ; }
  ok()   	{ echo "  OK" ; }
  skip()   	{ echo "SKIP" ; }
  fail() {
    echo "FAIL"
    while [ -n "$1" ] ; do
      printf "%s\n" "$1" ; shift
    done
    exit 1
  }
  skipop() {
    match "*:$1:*" ":$MKRELEASE_SKIP:" || return 1
    skip
    return 0
  }
  msg2() {
    M1="  $1 " ; shift
    M2="$@"
    L=`expr length "$M2"`
    I=$((80 - $L))
    printf "%-$I""s%s\\n" "$M1" "$M2"
  }
  # ensure VERSION, fallback to Makefile
  [ "$VERSION" = "_parse" ] && check_makefile && VERSION="`parse_makefile_var VERSION`"
  msg2 "Determine version..." "$VERSION"
  # extract REVISION from last numeric part in VERSION
  REVISION=`echo " $VERSION" | sed -n '/[0-9][ \t]*$/{ s/.*\b\([0-9]\+\)[ \t]*$/\1/ ; p ; q }'`
  [ -n "$REVISION" ] || { msg "Extracting revision from $VERSION..." ; fail ; }
  msg2 "Determine revision..." "$REVISION"
  # ensure TARBALL, fallback to Makefile
  [ "$TARBALL" = "_parse" ] && check_makefile && {
    # expand tarball from PACKAGE, VERSION, distdir and DIST_ARCHIVES
    PACKAGE="`parse_makefile_var PACKAGE`"
    distdir="`parse_makefile_var distdir | subst_makefile_vars`"
    DIST_ARCHIVES="`parse_makefile_var DIST_ARCHIVES | subst_makefile_vars`"
    TARBALL="`first_arg $DIST_ARCHIVES`"
    [ -z "$TARBALL" ] && die "Failed to determine release tarball"
  }
  msg2 "Determine tarball..." "$TARBALL"
  # read REMOTE_URL from Makefile.am
  [ "$REMOTE_URL" = _parse ] && check_makefile && REMOTE_URL="`parse_makefile_var MKRELEASE_UPLOAD_URL`"
  # ensure remote host from remote URL
  REMOTE_HOST=`printf "%s" "$REMOTE_URL" | sed -e 's/:.*//'`
  msg2 "Determine remote host..." "$REMOTE_HOST"
  [ -z "$REMOTE_URL" ] && die "remote release destination unkown, use -U or MKRELEASE_UPLOAD_URL"
  [ -z "$REMOTE_HOST" ] && die "Failed to determine remote host"
  # extract remote path, slash terminated
  REMOTE_PATH=`printf "%s" "$REMOTE_URL" | sed -ne '/:/ { s/[^:]*:// ; p ; q }'`
  case "$REMOTE_PATH" in *[^/]) REMOTE_PATH="$REMOTE_PATH/" ;; esac
  msg2 "Determine remote path..." "$REMOTE_PATH"
  # read REVISIONVAR from Makefile.am
  [ "$REVISIONVAR" = _parse ] && check_makefile && REVISIONVAR="`parse_makefile_var MKRELEASE_REVISION_VAR`"
  # extract file from REVISIONVAR
  REVISIONVAR_FILE=`printf "%s" "$REVISIONVAR" | sed -e 's/:.*//'`
  REVISIONVAR_NAME=`printf "%s" "$REVISIONVAR" | sed -ne '/:/ { s/[^:]*:// ; p ; q }'`
  [ -n "$REVISIONVAR" ] && {
    msg2 "Determine file for revision increments..." "$REVISIONVAR_FILE"
    [ -z "$REVISIONVAR_FILE" ] && die "Failed to extract file from: $REVISIONVAR"
    msg2 "Determine variable for revision increments..." "$REVISIONVAR_NAME"
    [ -z "$REVISIONVAR_NAME" ] && die "Failed to extract variable from: $REVISIONVAR"
  }
  # release checks
  msg "Checking for a clean $VERSION working tree..."
  skipop "clean" || {
    test 0 = `git diff HEAD | wc -l` && ok \
      || fail "note: use 'git diff HEAD' to view working tree changes"
  }
  msg "Checking untagged revision $VERSION..."
  RNAME=`! git rev-parse --verify -q "$VERSION"` && ok \
    || fail "note: a revision named '$VERSION' already exists: $RNAME" \
    "> `git log -n1 --oneline $RNAME`"
  msg "Checking for updated ChangeLog..."
  TST=true
  [ -e ChangeLog ] || TST=false
  for stamp in `"$SCRIPT_NAME" commit-stamps` ; do
    [ ChangeLog -nt "$stamp" ] || TST=false
  done
  $TST && ok || fail "note: ChangeLog outdated; see: $SCRIPT_NAME ChangeLog"
  msg "Checking for NEWS to cover $VERSION..."
  skipop "news" || {
    head -n2 NEWS | grep -q "$VERSION" && ok || \
      fail "note: NEWS fails to describe version $VERSION"
  }
  msg "Checking release tarball $TARBALL..."
  skipop "tarball" || {
    test -r "$TARBALL" && ok || fail "note: tarball unreadable"
  }
  msg "Checking tarball against ChangeLog age..."
  skipop "changelogage" || {
    test "$TARBALL" -nt ChangeLog && ok \
      || fail "note: ChangeLog appears to be newer; make distcheck"
  }
  msg "Checking tarball against NEWS age..."
  skipop "newsage" || {
    test "$TARBALL" -nt NEWS && ok \
      || fail "note: NEWS appears to be newer; make distcheck"
  }
  [ -n "$REVISIONVAR" ] && {
    msg "Checking revision variable to match version..."
    N=`sed -ne "/^$REVISIONVAR_NAME\s*=\s*[0-9]/ { s/^[^=]*=\s*\([0-9]\+\).*/\1/ ; p ; q }" $REVISIONVAR_FILE`
    [ -n "$N" -a "$N" = "$REVISION" ] && ok \
      || fail "note: mismatching revisions for $VERSION: '$REVISION' != '$N'"
    msg "Checking for git tracking of $REVISIONVAR_FILE..."
    git rev-list --max-count=1 HEAD -- "$REVISIONVAR_FILE" | grep -q '.' && ok \
      || fail "note: unexisting file in git HEAD: $REVISIONVAR_FILE"
  }
  msg "Checking for even revision in version $VERSION..."
  skipop "evenrev" || {
    test "$REVISION" = `echo "$REVISION / 2 * 2" | bc` && ok \
      || fail "note: refusing to release development version with odd revision: $REVISION"
  }
  msg "Checking master to be the current branch..."
  CBRANCH=`{ git symbolic-ref -q HEAD || git rev-parse HEAD ; }`
  test "$CBRANCH" = refs/heads/master && ok \
    || fail "note: expecting releases to be made from 'master' branch"
  msg "Checking HEAD to match upstream repository..."
  HBRANCH=`git symbolic-ref HEAD | sed s,^refs/heads/,,`
  HREMOTE=`git config --get "branch.$HBRANCH.remote"`
  H_MERGE=`git config --get "branch.$HBRANCH.merge"`
  skip || { # test -z "$HBRANCH" -o -z "$HREMOTE" -o -z "$H_MERGE" && skip || {
    RCOMMIT=`git ls-remote "$HREMOTE" "$H_MERGE" | sed 's/^\([[:alnum:]]\+\).*/\1/'`
    TCOMMIT=`git rev-list -n1 HEAD`
    test "$TCOMMIT" = "$RCOMMIT" && ok \
      || fail "note: mismatching HEAD and upstream revisions: $HREMOTE $H_MERGE" \
              "  $TCOMMIT != ${RCOMMIT:-<unknown-ref>}"
  }
  msg "Checking remote for unique release tarball..."
  ssh -x "$REMOTE_HOST" test ! -e "$REMOTE_PATH$TARBALL" && ok \
    || fail "note: file already exists: $REMOTE_HOST:$REMOTE_PATH$TARBALL"
  # planned steps
  msg2 "* Planned: tag HEAD as '$VERSION' release..."
  msg2 "* Planned: upload tarball $TARBALL..."
  [ -n "$REVISIONVAR" ] && \
    msg2 "* Planned: commit revision increment of $REVISIONVAR_FILE:$REVISIONVAR_NAME"
  read -ei n -p "--- Proceed as planned [y/n]? " ANS
  case "$ANS" in
    Y|YES|Yes|y|yes|1) msg "Confirmed planned procedures..." ; ok ;;
    *) die 1 "User cancellation..." ;;
  esac
  # tag release
  msg_ "* Tagging HEAD as '$VERSION' release..."
  git tag -m "Released $TARBALL" "$VERSION" && ok || fail
  git tag -n1 -l "$VERSION" | sed 's/^/  > /'
  # upload
  msg_ "* Uploading release tarball $TARBALL..."
  rsync -lpt --delay-updates "$TARBALL" "$REMOTE_HOST:$REMOTE_PATH" && ok \
    || fail "note: rsync transfer failed"
  RLS=$(ssh -x "$REMOTE_HOST" ls -l \`readlink -f "$REMOTE_PATH/$TARBALL"\`)
  msg2 ">" "$RLS"
  # bump version
  needs_head_push=false
  [ -n "$REVISIONVAR" ] && {
    N=$((1 + $REVISION))
    msg "Increment $REVISIONVAR_FILE:$REVISIONVAR_NAME to $N..."
    TEMPF="`mktemp -t mkrtempf.$$XXXXXX`" && touch $TEMPF || \
      die 9 "Failed to create temporary file"
    TEMPS="`mktemp -t mkrtemps.$$XXXXXX`" && touch $TEMPS || \
      die 9 "Failed to create temporary file"
    trap "rm -f $TEMPF" 0 HUP INT QUIT TRAP USR1 PIPE TERM
    sed "0,/^\($REVISIONVAR_NAME\s*=\s*\)[0-9]\+/s//\1$N/" < "$REVISIONVAR_FILE" > $TEMPF \
      && ok || fail
    cp "$REVISIONVAR_FILE" $TEMPS && touch $TEMPS -r "$REVISIONVAR_FILE" || \
      die 9 "Failed to record time stamp for: $REVISIONVAR_FILE"
    mv $TEMPF "$REVISIONVAR_FILE"
    git diff -U0 "$REVISIONVAR_FILE" | sed 's/^/  > /'
    git commit -v -m "$REVISIONVAR_FILE: revision increment of $REVISIONVAR_NAME to $N" \
      -- "$REVISIONVAR_FILE" | sed 's/^/  > /' || die "git commit failed"
    msg_ "* Comitted revision increment of $REVISIONVAR_NAME to $N" ; ok
    needs_head_push=true
    # checkout tag at VERSION
    msg_ "* Checkout $VERSION" "..."
    git checkout "$VERSION"
    # restore REVISIONVAR_FILE timestamp
    cmp $TEMPS "$REVISIONVAR_FILE" && touch "$REVISIONVAR_FILE" -r $TEMPS
    rm -f $TEMPS
  }
  # push notes
  $needs_head_push && \
    CHASH=`git rev-list -n1 "$VERSION"`
    msg2 "Note, push HEAD with:      # git push origin $CHASH:master"
    msg2 "Note, update 'devel' with: # git checkout devel && git merge --ff-only $CHASH"
  msg2 "Note, push tag with:       # git push origin '$VERSION'"
  msg2 "Done."
  exit
}

# === missing command ===
usage 1
