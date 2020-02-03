# Target programs
programs := test_fs.x		\
	simple.x \
	test_read.x \

# File-system library
FSLIB := libfs
FSPATH := ../$(FSLIB)
libfs := $(FSPATH)/$(FSLIB).a

# Default rule
all: $(libfs) $(programs)

# Avoid builtin rules and variables
MAKEFLAGS += -rR

# Don't print the commands unless explicitely requested with `make V=1`
ifneq ($(V),1)
Q = @
V = 0
endif

# Current directory
CUR_PWD := $(shell pwd)

# Define compilation toolchain
CC	= gcc

# General gcc options
CFLAGS	:= -Wall -Werror
CFLAGS	+= -pipe
## Debug flag
ifneq ($(D),1)
CFLAGS	+= -O2
else
CFLAGS	+= -O0
CFLAGS	+= -g
endif

# Linker options
LDFLAGS := -L$(FSPATH) -lfs

# Include path
INCLUDE := -I$(FSPATH)

# Generate dependencies
DEPFLAGS = -MMD -MF $(@:.o=.d)

# Application objects to compile
objs := $(patsubst %.x,%.o,$(programs))

# Include dependencies
deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)

# Rule for libfs.a
$(libfs):
	@echo "MAKE	$@"
	$(Q)$(MAKE) V=$(V) D=$(D) -C $(FSPATH)

# Generic rule for linking final applications
%.x: %.o $(libfs)
	@echo "LD	$@"
	$(Q)$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Generic rule for compiling objects
%.o: %.c
	@echo "CC	$@"
	$(Q)$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $< $(DEPFLAGS)

# Cleaning rule
clean:
	@echo "CLEAN	$(CUR_PWD)"
	$(Q)$(MAKE) V=$(V) -C $(FSPATH) clean
	$(Q)rm -rf $(objs) $(deps) $(programs) README.html

# Keep object files around
.PRECIOUS: %.o
.PHONY: clean $(libfs)

