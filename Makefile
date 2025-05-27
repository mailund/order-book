BUILD ?= release
export BUILD

LIBDIR := c/lib
SUBDIRS := $(shell find c -mindepth 1 -maxdepth 1 -type d) rust
OTHER_SUBDIRS := $(filter-out $(LIBDIR), $(SUBDIRS))

.PHONY: all $(LIBDIR) $(OTHER_SUBDIRS)

# Build everything
all: $(LIBDIR) $(OTHER_SUBDIRS)

# Build the lib directory 
$(LIBDIR):
	@echo "🔧 Building $(LIBDIR)..."
	@$(MAKE) -C $@

# Build all other subdirectories, ensuring lib is first
$(OTHER_SUBDIRS): | $(LIBDIR)
	@echo "🔧 Building $@..."
	@$(MAKE) -C $@

.PHONY: clean
clean:
	@for dir in $(SUBDIRS); do \
	  echo "🧹 Cleaning $$dir..."; \
	  $(MAKE) -C $$dir clean; \
	done
