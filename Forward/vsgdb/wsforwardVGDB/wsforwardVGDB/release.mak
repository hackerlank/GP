#Generated by VisualGDB (http://visualgdb.com)
#DO NOT EDIT THIS FILE MANUALLY UNLESS YOU ABSOLUTELY NEED TO
#USE VISUALGDB PROJECT PROPERTIES DIALOG INSTEAD

BINARYDIR := Release

#Toolchain
CC := gcc
CXX := g++
LD := $(CXX)
AR := ar
OBJCOPY := objcopy

#Additional flags
PREPROCESSOR_MACROS := NDEBUG=1 RELEASE=1
INCLUDE_DIRS := ./Log
LIBRARY_DIRS := 
LIBRARY_NAMES := 
ADDITIONAL_LINKER_INPUTS := 
MACOS_FRAMEWORKS := 
LINUX_PACKAGES := 

CFLAGS := -ggdb -gdwarf-2 -ffunction-sections -O3 -std=c++0x
CXXFLAGS := -ggdb -gdwarf-2 -ffunction-sections -O3 -std=c++0x
ASFLAGS := 
LDFLAGS := -Wl,-Bstatic -static-libstdc++ -static-libgcc -gc-sections, -Wl,-Bdynamic -lpthread
COMMONFLAGS := 
LINKER_SCRIPT := 

START_GROUP := -Wl,--start-group
END_GROUP := -Wl,--end-group

#Additional options detected from testing the toolchain
IS_LINUX_PROJECT := 1