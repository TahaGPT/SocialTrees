#!/bin/bash

# Compile and Run All Tests for Merkle Tree System
# Includes: CLI Interface, Unit Tests, Integration Tests, Performance Tests

set -e  # Exit on error

echo "========================================"
echo "  MERKLE TREE TEST COMPILATION"
echo "========================================"
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Navigate to backend
cd "$(dirname "$0")/backend"

echo -e "${YELLOW}Compiling CLI Interface...${NC}"
g++ -std=c++11 -O3 -o cli_interface cli_simple.cpp
echo -e "${GREEN}✓ CLI interface compiled${NC}\n"

echo -e "${YELLOW}Compiling Test Suite...${NC}"
g++ -std=c++11 -O3 -o test_suite comprehensive_test_suite.cpp merkle_api.cpp sha.cpp -lpthread
echo -e "${GREEN}✓ Test suite compiled${NC}\n"

echo "========================================"
echo "  RUNNING COMPREHENSIVE TEST SUITE"
echo "========================================"
echo ""

# Run test suite
./test_suite
TEST_EXIT_CODE=$?

echo ""
echo "========================================"
echo "  TEST SUITE COMPLETE"
echo "========================================"
echo ""

if [ $TEST_EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✓ ALL TESTS PASSED${NC}"
    echo ""
    echo "To run the CLI interface:"
    echo "  cd backend && ./cli_interface"
else
    echo -e "${RED}✗ SOME TESTS FAILED${NC}"
    exit 1
fi

echo ""
echo "Available executables:"
echo "  - ./backend/test_suite          # Run all tests"
echo "  - ./backend/cli_interface       # Interactive CLI"
echo "  - ./backend/api_server          # REST API server"
echo ""

exit $TEST_EXIT_CODE
