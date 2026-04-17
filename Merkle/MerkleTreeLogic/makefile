# Compiler settings
CXX = g++
# Added -pthread for multi-threading
CXXFLAGS = -std=c++11 -Wall -O3 -pthread

# Executable name
TARGET = app

# Source files (Using the optimized one)
SRCS = merkle.cpp sha.cpp

# Headers
HEADERS = sha.hpp

# --- TARGETS ---
all: clean $(TARGET) run

$(TARGET):
	@echo "--- COMPILING OPTIMIZED VERSION ---"
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

unit_tests: unit_tests.cpp sha.cpp sha.hpp
	@echo "--- COMPILING UNIT TESTS ---"
	$(CXX) $(CXXFLAGS) unit_tests.cpp sha.cpp -o unit_tests
	@echo "--- RUNNING UNIT TESTS ---"
	@./unit_tests

run:
	@echo "--- RUNNING ---"
	@./$(TARGET)

test: $(TARGET)
	@echo "--- RUNNING VALIDATION TESTS ---"
	@chmod +x final_test.sh
	@./final_test.sh

test-unit: unit_tests

clean:
	@echo "--- CLEANING ---"
	rm -f $(TARGET) unit_tests

.PHONY: all run test test-unit unit_tests clean