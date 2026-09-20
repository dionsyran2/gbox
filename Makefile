# Detect the Host OS
ifeq ($(OS), Windows_NT)
    HOST_OS := Windows
else
    HOST_OS := $(shell uname -s)
endif

TARGET_OS ?= $(HOST_OS)
CXXFLAGS = -Wall -std=c++17 -O3
INCLUDES = -Iinclude -Ivendor/imgui -Ivendor/imgui/backends -Ivendor/nfd/include

BINDIR = bin
OBJDIR = lib
SRCDIR = src
VENDORDIR = vendor

ifeq ($(HOST_OS), Windows)
    RM = del /Q /F
    FIXPATH = $(subst /,\,$1)
    MKDIR_OBJ = if not exist "$(subst /,\,$(patsubst %/,%,$(dir $@)))" mkdir "$(subst /,\,$(patsubst %/,%,$(dir $@)))"
else
    RM = rm -f
    FIXPATH = $1
    MKDIR_OBJ = mkdir -p $(dir $@)
endif

ifeq ($(TARGET_OS), Windows)
    TARGET_EXT = .exe
    
    LDFLAGS = -lglfw3 -lgdi32 -lopengl32 -luser32 \
              -lole32 -luuid -lshell32

    ifeq ($(HOST_OS), Linux)
        CXX ?= x86_64-w64-mingw32-g++
    else
        CXX ?= g++
    endif
else
    TARGET_EXT =
    
    LDFLAGS = -lglfw -lGL -lm -lpthread -ldl $(shell pkg-config --libs gtk+-3.0)

    CXXFLAGS += $(shell pkg-config --cflags gtk+-3.0)

    ifeq ($(HOST_OS), Windows)
        CXX ?= wsl g++
    endif
endif



rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC_FILES = $(call rwildcard,$(SRCDIR),*.cpp)
VENDOR_FILES = $(call rwildcard,$(VENDORDIR),*.cpp)

ifeq ($(TARGET_OS), Windows)
    VENDOR_FILES := $(filter-out %nfd_gtk.cpp %nfd_portal.cpp %nfd_cocoa.m, $(VENDOR_FILES))
else
    VENDOR_FILES := $(filter-out %nfd_win.cpp %nfd_portal.cpp %nfd_cocoa.m, $(VENDOR_FILES))
endif


SRCS = $(SRC_FILES) $(VENDOR_FILES)

OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC_FILES)) \
       $(patsubst $(VENDORDIR)/%.cpp, $(OBJDIR)/$(VENDORDIR)/%.o, $(VENDOR_FILES))


TARGET = $(BINDIR)/gbox$(TARGET_EXT)

.PHONY: all clean dirs

all: dirs $(TARGET)

dirs:
ifeq ($(HOST_OS), Windows)
	@if not exist "$(call FIXPATH,$(BINDIR))" mkdir "$(call FIXPATH,$(BINDIR))"
else
	@mkdir -p $(BINDIR)
endif

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@$(MKDIR_OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/$(VENDORDIR)/%.o: $(VENDORDIR)/%.cpp
	@$(MKDIR_OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) $(call FIXPATH,$(OBJS))