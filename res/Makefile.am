# Rapicorn - experimental UI toolkit
include $(top_srcdir)/Makefile.decl
noinst_DATA =

# == Rebuild Makefile.res ==
-include Makefile.res	# define RES_FILE_LIST
streq = $(and $(findstring $(1),$(2)),$(findstring $(2),$(1)))
NEW_RES_PATHS := $(sort $(filter-out Makefile%, $(wildcard * */* */*/*)))
NEW_RES_DIRS  := $(patsubst %/., %, $(wildcard $(addsuffix /., $(NEW_RES_PATHS))))
NEW_RES_FILES := $(filter-out $(NEW_RES_DIRS), $(NEW_RES_PATHS))
Makefile.res: @INGIT@ $(if $(call streq, $(NEW_RES_FILES), $(RES_FILE_LIST)),, Makefile.tmp) $(NEW_RES_FILES)
	$(AM_V_GEN)
	$(Q) echo "RES_FILE_LIST = $(NEW_RES_FILES)" | \
	  sed 's/\(.\{61\}[^ ]\+\) /\1 \\\n\t/g' > Makefile.tmp
	$(Q) mv Makefile.tmp $@
Makefile.tmp: # MAKE(1) only remakes+rereads Makefiles with prerequisites, see (Remaking Makefiles)
	$(Q) touch $@
noinst_DATA += Makefile.res

# == Ship Resource Files ==
EXTRA_DIST += $(RES_FILE_LIST) Makefile.res
