import os

src_files = ""
compiler_flags = "-Wall -Wextra -std=c++23 -std=c23"
linker_flags = "-lSDL2 -lSDL2_image -lGL -lc"
name = "linux-server-kv-rat"

src_dir = "/src/"
imgui_dir = "/external/imgui/"

def clean():
    pass