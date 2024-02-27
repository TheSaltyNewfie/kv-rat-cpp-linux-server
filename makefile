# Compiler to use
CC = g++

# Compiler flags
CFLAGS = -Wall -Wextra -std=c++23 -std=c23

# Linker flags
LDFLAGS = -lSDL2 -lSDL2_image -lGL -lc

# Executable name
EXEC = linux-server-kv-rat

IMGUI_DIR = external/imgui/

# Source files
SRC = main.cpp gui.cpp data.cpp
SRC += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp $(IMGUI_DIR)/imgui_stdlib.cpp
SRC += $(IMGUI_DIR)/imgui_impl_sdl2.cpp $(IMGUI_DIR)/imgui_impl_opengl3.cpp 
# Object files
OBJ = $(SRC:.cpp=.o)

LIBS += $(LINUX_GL_LIBS) -ldl `sdl2-config --libs`

CXXFLAGS += `sdl2-config --cflags`
CFLAGS = $(CXXFLAGS)

# Build rule for the executable
$(EXEC): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(EXEC)

# Rule for object files
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJ) $(EXEC)
