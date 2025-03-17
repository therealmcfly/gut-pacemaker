CC = gcc
CFLAGS = -Wall -g -O2 -I$(INCDIR)

TARGET = pacemaker
SRCDIR = src
INCDIR = inc
OBJDIR = obj

# Find all .c files in src/
SRCS = $(wildcard $(SRCDIR)/*.c)

# Convert src/file.c → obj/file.o
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

# Compile .c to .o (assumes obj/ already exists)
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	
# Link the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Clean build files (Windows & Unix compatible)
clean:
	@if exist $(OBJDIR) (rmdir /S /Q $(OBJDIR))
	@if exist $(TARGET) (del /F /Q $(TARGET))

.PHONY: clean
