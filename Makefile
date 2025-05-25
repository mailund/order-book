# Define the lib directory and find all subdirectories in "c"
LIBDIR := c/lib
SUBDIRS := $(shell find c -mindepth 1 -maxdepth 1 -type d)
OTHER_SUBDIRS := $(filter-out $(LIBDIR), $(SUBDIRS))

.PHONY: all $(LIBDIR) $(OTHER_SUBDIRS)

# Build the lib directory first, then the other subdirectories.
all: $(LIBDIR) $(OTHER_SUBDIRS)

# Build the lib directory.
$(LIBDIR):
	$(MAKE) -C $@

# Build other subdirectories, ensuring lib is built first.
$(OTHER_SUBDIRS): | $(LIBDIR)
	$(MAKE) -C $@


.PHONY: clean
clean:
	@for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done