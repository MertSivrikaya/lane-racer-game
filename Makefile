# Detect OS
OS := $(shell uname -s)

# Compiler
CXX = gcc
CXXFLAGS = -Wall -Wextra -std=c++17 -g -O0 

# Source files
SRCS = main.c

# Object files directory
OBJ_DIR = obj

# Object files (with paths relative to OBJ_DIR)
OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS:.c=.o))

# Include directories
INCLUDES = -Iinclude

# OS-Specific configurations
ifeq ($(OS), Linux)
    LIBS = -L/usr/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    TARGET = lane_racer
else
    LIBS = -Llib -lraylib -lopengl32 -lgdi32 -lwinmm
    TARGET = lane_racer.exe
    CXXFLAGS += -static
endif

# Default rule
all: $(OBJ_DIR) $(TARGET)

# Create object files directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Compile source files into object files
$(OBJ_DIR)/%.o: %.c
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Link object files into the final executable
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LIBS)

# Clean up object files and executable
clean:
	-rm -rf obj/*.o $(TARGET)