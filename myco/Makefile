# Myco Language Interpreter Makefile
# Version 1.6.0 - Modular Architecture

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -O2 -Iinclude
LDFLAGS = -lm

# Source directories
SRC_DIR = src
CORE_DIR = $(SRC_DIR)/core
DATA_STRUCTURES_DIR = $(SRC_DIR)/data_structures
LIBRARIES_DIR = $(SRC_DIR)/libraries
SYSTEM_DIR = $(SRC_DIR)/system
PACKAGE_MANAGER_DIR = $(SRC_DIR)/package_manager

# Core language features
CORE_SOURCES = $(CORE_DIR)/eval.c

# Data structures
DATA_SOURCES = $(DATA_STRUCTURES_DIR)/arrays.c \
               $(DATA_STRUCTURES_DIR)/objects.c \
               $(DATA_STRUCTURES_DIR)/sets.c \
               $(DATA_STRUCTURES_DIR)/collections.c

# Libraries
LIB_SOURCES = $(LIBRARIES_DIR)/math_lib.c \
              $(LIBRARIES_DIR)/utility_lib.c \
              $(LIBRARIES_DIR)/file_io_lib.c \
              $(LIBRARIES_DIR)/path_lib.c \
              $(LIBRARIES_DIR)/env_lib.c \
              $(LIBRARIES_DIR)/args_lib.c \
              $(LIBRARIES_DIR)/process_lib.c \
              $(LIBRARIES_DIR)/text_lib.c \
              $(LIBRARIES_DIR)/debug_lib.c \
              $(LIBRARIES_DIR)/types_lib.c \
              $(LIBRARIES_DIR)/polish_lib.c \
              $(LIBRARIES_DIR)/test_lib.c \
              $(LIBRARIES_DIR)/data_lib.c

# System functionality
SYSTEM_SOURCES = $(SYSTEM_DIR)/lexer.c \
                 $(SYSTEM_DIR)/parser.c \
                 $(SYSTEM_DIR)/codegen.c \
                 $(SYSTEM_DIR)/memory_tracker.c \
                 $(SYSTEM_DIR)/loop_manager.c

# Package manager (NEW)
PACKAGE_SOURCES = $(PACKAGE_MANAGER_DIR)/package_core.c \
                  $(PACKAGE_MANAGER_DIR)/dependency.c \
                  $(PACKAGE_MANAGER_DIR)/registry.c \
                  $(PACKAGE_MANAGER_DIR)/installer.c

# Main source
MAIN_SOURCE = $(SRC_DIR)/main.c

# All sources
SOURCES = $(MAIN_SOURCE) \
          $(CORE_SOURCES) \
          $(DATA_SOURCES) \
          $(LIB_SOURCES) \
          $(SYSTEM_SOURCES) \
          $(PACKAGE_SOURCES)

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Target executable
TARGET = myco

# Default target
all: $(TARGET)

# Build the interpreter
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✅ Myco interpreter built successfully!"

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "🧹 Build artifacts cleaned!"

# Install (optional)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/
	@echo "📦 Myco installed to /usr/local/bin/"

# Uninstall (optional)
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "🗑️  Myco uninstalled from /usr/local/bin/"

# Run tests
test: $(TARGET)
	@echo "🧪 Running Myco tests..."
	./$(TARGET) tests/unit_test.myco

# Development mode (with debug flags)
dev: CFLAGS += -DDEBUG -g3
dev: $(TARGET)

# Release mode (optimized)
release: CFLAGS += -O3 -DNDEBUG
release: $(TARGET)

# Show source structure
structure:
	@echo "📁 Myco Source Structure:"
	@echo "├── $(CORE_DIR)/          # Core language features"
	@echo "├── $(DATA_STRUCTURES_DIR)/ # Data type implementations"
	@echo "├── $(LIBRARIES_DIR)/      # Built-in libraries"
	@echo "├── $(SYSTEM_DIR)/         # System functionality"
	@echo "├── $(PACKAGE_MANAGER_DIR)/ # Package management"
	@echo "└── $(MAIN_SOURCE)        # Main entry point"

# Show build info
info:
	@echo "🔍 Myco Build Information:"
	@echo "Compiler: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Libraries: $(LDFLAGS)"
	@echo "Source files: $(words $(SOURCES))"
	@echo "Target: $(TARGET)"

# Help
help:
	@echo "🚀 Myco Makefile Help:"
	@echo "make          - Build the interpreter"
	@echo "make clean    - Clean build artifacts"
	@echo "make test     - Run unit tests"
	@echo "make dev      - Build with debug flags"
	@echo "make release  - Build optimized release"
	@echo "make install  - Install to system (requires sudo)"
	@echo "make uninstall- Remove from system (requires sudo)"
	@echo "make structure- Show source structure"
	@echo "make info     - Show build information"
	@echo "make help     - Show this help"

# Phony targets
.PHONY: all clean install uninstall test dev release structure info help 