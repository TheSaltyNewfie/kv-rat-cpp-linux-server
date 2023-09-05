# Compiler to use
CC = g++

# Compiler flags
CFLAGS = -Wall -Wextra

# Linker flags
LDFLAGS = -lSDL2 -lSDL2_image

# Executable name
EXEC = linux-server-kv-rat

# Source files
SRC = main.cpp

# Object files
OBJ = $(SRC:.cpp=.o)

# Build rule for the executable
$(EXEC): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(EXEC)

# Rule for object files
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJ) $(EXEC)
