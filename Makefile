CC = gcc
SRCDIR = src
INCDIR = inc
OBJDIR = obj

# Default build settings
# CFLAGS = -Wall -g -O2 -I$(INCDIR)
CFLAGS = -Wall -g -O0 -I$(INCDIR) -pthread
TARGET = pacemaker.out

# TSan build settings
TSAN_FLAGS = -Wall -g -O1 -fsanitize=thread -I$(INCDIR) -pthread
TSAN_TARGET = pacemaker_tsan.out

# Source and object files
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

# Default target
all: $(TARGET)

# Create object directory and compile .c to .o
$(OBJDIR)/%.o: $(SRCDIR)/%.c
ifeq ($(OS),Windows_NT)
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
else
	@mkdir -p $(OBJDIR)
endif
	$(CC) $(CFLAGS) -c $< -o $@
	
	
# Link the final executable
# $(TARGET): $(OBJS)
# 	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Link the TSan target
$(TSAN_TARGET): $(OBJS)
	$(CC) $(TSAN_FLAGS) -o $@ $(OBJS)

# Clean build files (Windows & Unix compatible)
clean:
ifeq ($(OS),Windows_NT)
	@if exist $(OBJDIR) (rmdir /S /Q $(OBJDIR))
	@if exist $(TARGET).exe (del /F /Q $(TARGET).exe)
	@if exist $(TSAN_TARGET).exe (del /F /Q $(TSAN_TARGET).exe)
else
	rm -rf $(OBJDIR) $(TARGET) $(TSAN_TARGET)
endif

# .PHONY: clean

# ThreadSanitizer build
tsan: clean $(TSAN_TARGET)

.PHONY: clean tsan all