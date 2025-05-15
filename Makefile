CC = gcc
# CFLAGS = -Wall -g -O2 -I$(INCDIR)
CFLAGS = -Wall -O3 -I$(INCDIR) -pthread

TARGET = pacemaker.out
SRCDIR = src
INCDIR = inc
OBJDIR = obj

# Find all .c files in src/
SRCS = $(wildcard $(SRCDIR)/*.c)

# Convert src/file.c → obj/file.o
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

# # Compile .c to .o
# # Compile .c to .o (assumes obj/ already exists)
# $(OBJDIR)/%.o: $(SRCDIR)/%.c
# 	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/%.o: $(SRCDIR)/%.c
ifeq ($(OS),Windows_NT)
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
else
	@mkdir -p $(OBJDIR)
endif
	$(CC) $(CFLAGS) -c $< -o $@
	
	
# Link the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Clean build files (Windows & Unix compatible)
clean:
ifeq ($(OS),Windows_NT)
	@if exist $(OBJDIR) (rmdir /S /Q $(OBJDIR))
	@if exist $(TARGET).exe (del /F /Q $(TARGET).exe)
else
	rm -rf $(OBJDIR) $(TARGET)
endif

.PHONY: clean
