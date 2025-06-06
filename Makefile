# Default compiler
CC = gcc

SRCDIR = src
INCDIR = inc
OBJDIR_PC = obj/pc
OBJDIR_TSAN = obj/tsan
OBJDIR_DE1SOC = de1soc/obj

# Build settings
CFLAGS_PC = -Wall -g -O0 -I$(INCDIR) -pthread -fstack-usage
CFLAGS_TSAN = -Wall -g -O1 -fsanitize=thread -I$(INCDIR) -pthread
CFLAGS_DE1SOC = -Wall -O2 -std=gnu99 -I$(INCDIR) -march=armv7-a -mtune=cortex-a9 -mfloat-abi=hard -mfpu=neon -fstack-usage

# Source files
SRCS = $(wildcard $(SRCDIR)/*.c)

# Object files per target
OBJS_PC = $(patsubst $(SRCDIR)/%.c, $(OBJDIR_PC)/%.o, $(SRCS))
OBJS_TSAN = $(patsubst $(SRCDIR)/%.c, $(OBJDIR_TSAN)/%.o, $(SRCS))
OBJS_DE1SOC = $(patsubst $(SRCDIR)/%.c, $(OBJDIR_DE1SOC)/%.o, $(SRCS))

# Executable file names
TARGET_PC = p.o
TARGET_TSAN = p_tsan.o
TARGET_DE1SOC = de1soc/p_de1soc.o

# All target: builds all variants
all: pc tsan de1soc

# PC build
pc: CFLAGS = $(CFLAGS_PC)
pc: $(TARGET_PC)

# TSAN build
tsan: CFLAGS = $(CFLAGS_TSAN)
tsan: $(TARGET_TSAN)

# DE1-SoC build
de1soc: CC = $(HOME)/toolchains/gcc-linaro-4.9-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
de1soc: CFLAGS = $(CFLAGS_DE1SOC)
SYSROOT = $(HOME)/toolchains/de1soc_rootfs
de1soc: $(TARGET_DE1SOC)

# Compile source to object files per target
$(OBJDIR_PC)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR_PC)
	$(CC) $(CFLAGS_PC) -c $< -o $@

$(OBJDIR_TSAN)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR_TSAN)
	$(CC) $(CFLAGS_TSAN) -c $< -o $@

$(OBJDIR_DE1SOC)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR_DE1SOC)
	$(CC) $(CFLAGS_DE1SOC) --sysroot=$(SYSROOT) -c $< -o $@

# Link targets
$(TARGET_PC): $(OBJS_PC)
	$(CC) $(CFLAGS_PC) $^ -o $@

$(TARGET_TSAN): $(OBJS_TSAN)
	$(CC) $(CFLAGS_TSAN) $^ -o $@

$(TARGET_DE1SOC): $(OBJS_DE1SOC)
	@mkdir -p de1soc
	$(CC) --sysroot=$(SYSROOT) $(CFLAGS_DE1SOC) -static $^ -lpthread -o $@

# Clean build files
clean:
	rm -rf $(OBJDIR_PC) $(OBJDIR_TSAN) de1soc $(TARGET_PC) $(TARGET_TSAN)

clean-pc:
	rm -rf $(OBJDIR_PC) $(TARGET_PC)

clean-tsan:
	rm -rf $(OBJDIR_TSAN) $(TARGET_TSAN)

clean-de1soc:
	rm -rf de1soc

.PHONY: all clean clean-pc clean-tsan clean-de1soc pc tsan de1soc
