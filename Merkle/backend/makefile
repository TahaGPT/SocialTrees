# Compiler settings
CXX = g++
# Added -pthread for multi-threading
CXXFLAGS = -std=c++11 -Wall -O3 -pthread

# Executable names
TARGET = app
API_SERVER = api_server

# Source files (Using the optimized one)
SRCS = merkle.cpp sha.cpp

# Headers
HEADERS = sha.hpp json.hpp httplib.h merkle_api.hpp

# --- TARGETS ---
all: clean $(TARGET) run

$(TARGET):
	@echo "--- COMPILING OPTIMIZED VERSION ---"
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

# Build API Server
api: clean-api
	@echo "--- COMPILING API SERVER ---"
	$(CXX) $(CXXFLAGS) -DBUILD_API_SERVER -c sha.cpp -o sha.o
	$(CXX) $(CXXFLAGS) -DBUILD_API_SERVER api_server.cpp merkle_api.cpp sha.o -o $(API_SERVER)
	@echo "--- API SERVER COMPILED ---"

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
	rm -f $(TARGET) unit_tests *.o

clean-api:
	@echo "--- CLEANING API ---"
	rm -f $(API_SERVER) *.o

.PHONY: all run test test-unit unit_tests clean clean-api api