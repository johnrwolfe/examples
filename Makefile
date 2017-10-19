#!/bin/make

GIT_VERSION := $(shell git describe --abbrev=8 --dirty --always --tags)
GITIGNORE := .gitignore

CXXFLAGS += $(DEPFLAGS)
CXXFLAGS += -g -Wall -DGIT_VERSION=\"$(GIT_VERSION)\"
CXXFLAGS += --std=c++14 -I include
LDFLAGS += -lboost_system -lboost_thread -lboost_date_time -lboost_program_options -lboost_chrono
LDFLAGS += -lpthread -lrt

ALL += xml-describe xml-mavlink-info xml-mavlink

.PHONY: all clean distclean gitignore
NOT_MISSING := all clean distclean gitignore

all : $(ALL)

distclean : clean
	rm -rf $(DEPDIR)



##########################################
## automatic target/dependency management
##
## note: this doesn't work properly with make < 4.0

# This macro creates a .o dependency for each $(1), adds
# $(obj_...) to the list of its dependencies, and adds $(lib_...) to
# LDFLAGS (with an additional -l for each value in $(lib_...))
#
# This lets us supply obj_... and lib_... only for programs that need them
#
# TODO: add pkg-config logic here to automatically assist with LDFLAGS and CXXFLAGS, i.e.:
#  $(1): CXXFLAGS += -g $(shell pkg-config --silence-errors --cflags $(__libs))
#  $(1): LDFLAGS += $(shell pkg-config --silence-errors --libs $(__libs))
#
# TODO: what if the target is "foo.o"? see also $(basename ...)
# ... and this example from the GNU Make docs:
#
#  $(filter %.o,$(files)): %.o: %.c
#  $(CC) -c $(CFLAGS) $< -o $@
#
define __build_target =
$(1) : $(1).o $$(obj_$(1)) $$(src_$(1):.cpp=.o)
 ifdef lib_$(1)
  __libs = $(strip $(lib_$(1)))
$(1): LDFLAGS += $$(lib_$(1):%=-l%)
  endif
$(1) :
	sed --in-place -e "\|\<$$^\>|h; \$$$${x;s|$$^||;{g;t};a\\" -e "$$^" -e "}" $(GITIGNORE)
	$$(CXX) $$(CXXFLAGS) -o $$@ $$^ -Wl,--start-group $$(LDFLAGS) -Wl,--end-group
endef

define __clean_target =
.PHONY : clean_$(1)
clean_$(1):
	rm -f $(1) $(foreach out,$(1),$(out).o $$(src_$(1):.cpp=.o) $$(obj_$(out)) $$(DEPDIR)/$(out).d $$(DEPDIR)/$(out).Td)
clean : clean_$(1)
endef

$(foreach out,$(filter-out %.json,$(ALL)),$(eval $(call __build_target,$(out))))
$(foreach out,$(ALL),$(eval $(call __clean_target,$(out))))

# if MAKECMDGOALS includes an unknown build target, attempt to build a rule for it
# if the target is clean_$(unknown), then clean it rather than building it
missing = $(strip $(filter-out $(ALL) %.json $(NOT_MISSING),$(MAKECMDGOALS)))
ifneq ("$(missing)","")
$(foreach miss,$(subst clean_,,$(missing)),$(eval $(call __build_target,$(miss))))
$(foreach miss,$(subst clean_,,$(missing)),$(eval $(call __clean_target,$(miss))))
endif

#####
## automated dependency generation

SRCS = $(wildcard *.cpp)

# creates a $(DEPDIR)/foo.d file for each foo.cpp
# the intermediate foo.Td helps us fail gracefully
DEPDIR = .d
$(shell mkdir -p $(DEPDIR) >/dev/null)

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

# overrides .cpp->.o to also build dependency file
%.o : %.cpp $(DEPDIR)/%.d
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	$(POSTCOMPILE)

# pattern rule with an empty recipe, so make won't
# fail if a .d file doesn't exist
$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

# if a file is listed in $(SRCS), we look for a dependency file in $(DEPDIR) with the same name
## the following line MUST be the LAST line in the Makefile!
-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS)))

##
#####


