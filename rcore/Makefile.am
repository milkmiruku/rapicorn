# Rapicorn
include $(top_srcdir)/Makefile.decl

SUBDIRS	     = . tests

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir) -I.
DEFS        += @DEFINE__FILE_DIR__@ -DRAPICORN_CONVENIENCE -D__RAPICORN_BUILD__
AM_CXXFLAGS += $(RAPICORN_CORE_CFLAGS) $(SVG_DEPENDENCIES_CFLAGS)
GLIB_MKENUMS = glib-mkenums

rapicorn_public_headers = $(strip	\
	aida.hh		aidaprops.hh	\
	bindable.hh	resources.hh	formatter.hh	inout.hh	inifile.hh \
        platform.hh	debugtools.hh					utilities.hh  \
					math.hh				unicode.hh   \
        xmlnode.hh	markup.hh					testutils.hh \
			regex.hh  strings.hh	loop.hh		main.hh	memory.hh \
	quicktimer.hh	randomhash.hh	thread.hh	\
	$(rapicorn_impl_headers) \
	cpuasm.hh	cxxaux.hh	rcore.hh	visitor.hh	aidasignal.hh	\
)
rapicorn_cc_sources = $(sort	\
	aida.cc		aidaprops.cc	\
	bindable.cc	resources.cc	formatter.cc	inout.cc	inifile.cc \
        platform.cc	debugtools.cc					utilities.cc  \
					math.cc				unicode.cc   \
        xmlnode.cc	markup.cc					testutils.cc \
			regex.cc  strings.cc	loop.cc		main.cc	memory.cc \
	quicktimer.cc	randomhash.cc	svg.cc	thread.cc	\
)
rapicorn_impl_headers = $(strip 	\
	aidacxx.hh			\
	threadlib.hh			\
)
rapicorn_generated_headers = $(strip	\
	aidavariants.hh			\
	buildconfig.h			\
)
rapicorn_private_files = $(strip	\
	svg.hh				\
)

# === libraries ===
# librapicorn-core
noinst_LTLIBRARIES                      = librapicorncore-@MAJOR@.la
librapicorncore_@MAJOR@includedir       = $(includedir)/rapicorn-@MAJOR@/rcore
librapicorncore_@MAJOR@include_HEADERS  = $(rapicorn_public_headers) $(rapicorn_generated_headers)
librapicorncore_@MAJOR@_la_SOURCES      = $(rapicorn_cc_sources)
librapicorncore_@MAJOR@_la_DEPENDENCIES = ldscript.map buildid.lo
librapicorncore_@MAJOR@_la_LIBADD       = $(RAPICORN_CORE_LIBS) -lm buildid.lo
librapicorncore_@MAJOR@_la_LDFLAGS      = $(strip	\
	$(AM_LDFLAGS) -no-undefined				\
	-Wl,--version-script=$(srcdir)/ldscript.map		\
	-version-info $(LT_VERSION_INFO) 			\
) # set libtool version and export symbols for resolving
EXTRA_DIST += $(rapicorn_private_files) ldscript.map

# == Special Optimization Targets ==
OPTIMIZE_SOURCE_FILES = loop.cc randomhash.cc
AM_CXXFLAGS += $(patsubst %, @OPTIMIZE_FAST@, $(findstring $(<F), $(OPTIMIZE_SOURCE_FILES)))

# == Rapicorn Resources ==
@mk@ include $(top_srcdir)/res/Makefile.res	# define RES_FILE_LIST
zres.cc: $(RAPIDRESV) $(top_srcdir)/res/Makefile.res
	$(AM_V_GEN)
	$(Q) (cd $(top_srcdir)/res/ && $(abs_builddir)/$(RAPIDRESV) $(RES_FILE_LIST) ) > xgen-$@
	$(Q) mv xgen-$@ $@
CLEANFILES += xgen-zres.cc zres.cc
$(srcdir)/resources.cc: zres.cc

# === API content ===
api-content:
	cat $(sort $(filter-out buildconfig.h, $(rapicorn_public_headers) $(rapicorn_generated_headers)))
.PHONY: api-content

# === programs (not installed) ===
bin_PROGRAMS	= $(BIN_PROGS)
noinst_PROGRAMS	= $(ALLTESTS)
progs_ldadd	= librapicorncore-@MAJOR@.la $(RAPICORN_CORE_LIBS) -lm
BIN_PROGS	=

# == rapidres ==
RAPIDRESV             = rapidres # rapidres-@MAJOR@
BIN_PROGS            += $(RAPIDRESV)
rapidres_SOURCES      = rapidres.cc
rapidres_DEPENDENCIES = buildid.lo
rapidres_LDADD        = buildid.lo $(LIBZ) # $(progs_ldadd)

rapidres-installcheck:
	$(Q) cd . \
	&& echo '0123456789abcdef 0123456789abcdef c61fd6dd8e5a 0123456789abcdef c61fd6dd8e5a' > xtmp-empty.dat \
	; eval "$$TSTDIAGNOSE" "'Create rapidres sample data'"
	$(Q) $(bindir)/$(RAPIDRESV) xtmp-empty.dat > xtmp-empty.out \
	; eval "$$TSTDIAGNOSE" "'Run    rapidres'"
	$(Q) grep -q '"xtmp-empty.dat"' xtmp-empty.out \
	; eval "$$TSTDIAGNOSE" "'Verify rapidres output file name'"
	$(Q) test `grep _ENTRY xtmp-empty.out | grep -o '[0-9]*'` -gt 16 \
	; eval "$$TSTDIAGNOSE" "'Verify rapidres output length'"
	$(Q) test $$[`grep _DATA xtmp-empty.out | grep -o '[0-9 +-]*'`] -lt 50 \
	; eval "$$TSTDIAGNOSE" "'Verify rapidres output compression'"
	$(Q) rm -f xtmp-empty.dat xtmp-empty.out
installcheck-local: rapidres-installcheck
CLEANFILES += xtmp-empty.dat xtmp-empty.out

# === buildid.cc ===
BUILDID = $(shell test -x $(top_srcdir)/.git/ && (git log -n1 --pretty=format:Git-%h --abbrev=12 ; \
  C=`git diff HEAD --raw | wc -l ` ; test "$$C" -lt 1 || echo "+$$C" ) || sed -n "1,1{s/.*\# */ChangeLog-/g;p}" <$(top_srcdir)/ChangeLog )
AUTOCLEAN_BUILDIDCC := $(shell test \! -e buildid.cc || grep -qF '"$(BUILDID)"' buildid.cc 2>/dev/null || \
			   { rm -f buildid.cc ; echo "  CLEARING" buildid.cc >&2 ; })
buildid.cc: # buildid.lo is linked into libs/executables
	$(AM_V_GEN)
	$(Q) ( \
	  echo '#include "cxxaux.hh"' && \
	  echo 'namespace RapicornInternal { const char* buildid() { return "$(BUILDID)"; } }' \
	) >$@
CLEANFILES += buildid.cc
buildid:
	@echo "BUILDID      = $(BUILDID)"
	@echo "BUILDID_DEPS = $(BUILDID_DEPS)"
.PHONY: buildid

# === rapicornconfig.h ===
rapicornconfig.h: $(top_builddir)/config.status # Makefile
	$(AM_V_GEN)
	$(Q) cd . \
	&& echo "/* Generated data from $< (by make $@) */" 					 > xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- select programming environment --- */"					>> xgen-$(@F) \
	&& echo "#ifndef _GNU_SOURCE"								>> xgen-$(@F) \
	&& echo "#define _GNU_SOURCE"								>> xgen-$(@F) \
	&& echo "#endif"									>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* --- resource paths --- */"							>> xgen-$(@F) \
	&& echo	'#define RAPICORN_DOCDIR		"${docdir}"'				>> xgen-$(@F) \
	&& echo	'#define RAPICORN_LOCALEDIR     	"${pkglocaledir}"'			>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo ""										>> xgen-$(@F) \
	&& echo "/* Generated data ends here */" 						>> xgen-$(@F) \
	&& cp xgen-$(@F) $@ \
	&& rm -f xgen-$(@F)
CLEANFILES += xgen-rapicornconfig.h rapicornconfig.h
$(librapicorncore_@MAJOR@_la_OBJECTS): rapicornconfig.h
EXTRA_DIST += buildconfig.h.in

# === aidavariants.hh ===
XMANIFOLD = $(top_srcdir)/aidacc/xmanifold.py
$(srcdir)/*.cc: aidavariants.hh
aidavariants.hh: $(srcdir)/aidaproto.hh $(XMANIFOLD)
	$(AM_V_GEN)
	$(Q) cd . \
	&& echo "/* Aida Variants -- Generated from: $(^F) */"	 >xtmp-$(@F) \
	&& echo '# 1 "aidaproto.hh"'				>>xtmp-$(@F) \
	&& $(XMANIFOLD) $(srcdir)/aidaproto.hh 18		>>xtmp-$(@F) \
	&& cp xtmp-$(@F) $@ && rm -f xtmp-$(@F)
CLEANFILES += xtmp-aidavariants.hh aidavariants.hh xgen-aidas.sed
EXTRA_DIST += aidaproto.hh
# not using "$(abs_srcdir)/aidaproto.hh" to avoid hardcoded paths in distribution tarballs
