# Files to compile that don't have a main() function
CFILES = student support structs

# Files to compile that do have a main() function
TARGETS = filesystem

# Let the programmer choose 32 or 64 bits, but default to 64
BITS ?= 64

# Directory names
ODIR := ./obj$(BITS)
output_folder := $(shell mkdir -p $(ODIR))

# Names of files that the compiler generates
EXEFILES  = $(patsubst %, $(ODIR)/%,   $(TARGETS))
OFILES    = $(patsubst %, $(ODIR)/%.o, $(CFILES))
EXEOFILES = $(patsubst %, $(ODIR)/%.o, $(TARGETS))
DEPS      = $(patsubst %, $(ODIR)/%.d, $(CFILES) $(TARGETS))

# Use gcc
CC = gcc
CFLAGS = -MMD -O2 -m$(BITS) -ggdb -Wall
LDFLAGS = -m$(BITS)

# Best to be safe...
.DEFAULT_GOAL = all
.PRECIOUS: $(OFILES) $(EXEOFILES)
.PHONY: all clean submit

# Goal is to build all executables and shared objects
all: $(EXEFILES)

# Rules for building object files
$(ODIR)/%.o: %.c
	@echo "[CC] $< --> $@"
	@$(CC) $< -o $@ -c $(CFLAGS)

# Rules for building executables
$(ODIR)/%: $(ODIR)/%.o $(OFILES)
	@echo "[LD] $< --> $@"
	@$(CC) $^ -o $@ $(LDFLAGS)

# clean by clobbering the build folder and deploy folder
clean:
	@echo Cleaning up...
	@rm -rf $(ODIR) $(DEPLOY) $(TURNIN)

# submit via script
submit:
	@echo "Submitting..."
	~jloew/CSE303/cse303-submit-p3.pl

# Include dependencies
-include $(DEPS)
