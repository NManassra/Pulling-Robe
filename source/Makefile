# the compiler to use
CC = gcc

# compiler flags:
# -Wall enables all warnings
# -g adds debug info for tools like gdb
CFLAGS = -Wall -g

# linker flags:
# -lGL, -lGLU, -lglut link to OpenGL libraries
# -lm links the math library
LDFLAGS = -lGL -lGLU -lglut -lm

# source files are in the 'src' directory
SRC_DIR = src

# header files are in the 'include' directory
INC_DIR = include

# the name of the final program (output executable)
TARGET = rope_pulling_game_sim

# get all .c files from the source directory
SRCS = $(wildcard $(SRC_DIR)/*.c)

# default target: builds the program
all: $(TARGET)

# how to build the target
# uses gcc with flags and includes all source files
$(TARGET):
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $(TARGET) $(SRCS) $(LDFLAGS)

# clean target: deletes the built program (used to start fresh)
clean:
	rm -f $(TARGET)
