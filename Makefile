TARGET ?= linux

# Default target
ifeq ($(MAKECMDGOALS),linux)
  include config/Makefile.linux
endif

.PHONY: all linux arm_linux clean

# Build Commands
all:
	@echo "Please run 'make linux' or 'make arm_linux'"
pc:
	$(MAKE) -f config/Makefile.linux
tsan:
	$(MAKE) -f config/Makefile.linux VARIANT=tsan
arm:
	$(MAKE) -f config/Makefile.arm_linux

# Run commands
run_pc:
	@echo "Running Linux build..."
	$(MAKE) -f config/Makefile.linux run
run_tsan:
	@echo "Running Linux TSAN build..."
	$(MAKE) -f config/Makefile.linux run_tsan

# Clean all builds
clean:
	@echo "🧹 Cleaning all builds..."
	$(MAKE) -f config/Makefile.linux clean
	$(MAKE) -f config/Makefile.arm_linux clean
	@echo "✅ All builds cleaned."

# Target-specific clean commands
clean_pc:
	$(MAKE) -f config/Makefile.linux clean
clean_arm:
	$(MAKE) -f config/Makefile.arm_linux clean
clean_de1soc:
	$(MAKE) -f config/Makefile.de1soc clean
clean_flexpret:
	$(MAKE) -f config/Makefile.flexpret clean
