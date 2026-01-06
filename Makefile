CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I include

SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build

# Source files
DEBOUNCER_SRC = $(SRC_DIR)/height_debouncer.cpp
TEST_SRC = $(TEST_DIR)/test_height_debouncer.cpp

# Targets
TEST_BIN = test_height_debouncer

.PHONY: all test clean

all: test

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(DEBOUNCER_SRC) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(TEST_BIN)
	rm -rf $(BUILD_DIR)
