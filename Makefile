# ==============================================================================
# Makefile: Electrical Fault Diagnosis & Protection Analysis System
# Course: Programming Fundamentals and Software Engineering
# ==============================================================================

CC       ?= gcc
CXX      ?= g++
CFLAGS   = -Wall -Wextra -Wpedantic -O2 -Iinclude
CXXFLAGS = -Wall -Wextra -Wpedantic -O2 -Iinclude -Itests

# Detect OS & Executable Extensions
ifeq ($(OS),Windows_NT)
    TARGET      = bin/fault_system.exe
    TEST_TARGET = bin/test_runner.exe
    RUN_CMD     = ./bin/fault_system.exe
    TEST_CMD    = ./bin/test_runner.exe
else
    TARGET      = bin/fault_system
    TEST_TARGET = bin/test_runner
    RUN_CMD     = ./bin/fault_system
    TEST_CMD    = ./bin/test_runner
endif

MKDIR = mkdir -p bin build data docs
RM    = rm -rf build bin

# Application Source & Object Files
SRCS = src/main.c \
       src/validation.c \
       src/fault_analyzer.c \
       src/protection_relay.c \
       src/fault_manager.c \
       src/file_handler.c

OBJS = build/main.o \
       build/validation.o \
       build/fault_analyzer.o \
       build/protection_relay.o \
       build/fault_manager.o \
       build/file_handler.o

# Unit Test Source & Object Files
TEST_SRCS = tests/test_main.cpp \
            tests/test_validation.cpp \
            tests/test_fault_analyzer.cpp \
            tests/test_protection_relay.cpp \
            tests/test_fault_manager.cpp

TEST_OBJS = build/test_main.o \
            build/test_validation.o \
            build/test_fault_analyzer.o \
            build/test_protection_relay.o \
            build/test_fault_manager.o

# Core modules linked into test suite (excluding main.c)
CORE_C_OBJS = build/validation.o \
              build/fault_analyzer.o \
              build/protection_relay.o \
              build/fault_manager.o \
              build/file_handler.o

.PHONY: all run test clean prep

all: prep $(TARGET)

prep:
	@$(MKDIR)

# Link Main Application
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Build C Objects
build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link Google Test Suite
$(TEST_TARGET): prep $(CORE_C_OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(CORE_C_OBJS) $(TEST_OBJS) -lm

# Build C++ Test Objects
build/%.o: tests/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	$(RUN_CMD)

test: $(TEST_TARGET)
	$(TEST_CMD)

clean:
	@$(RM)
	@echo "Clean completed."
