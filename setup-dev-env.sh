#!/bin/bash
# ESP32 TapGate - Development Environment Setup Script (Unix/Linux/macOS)
# This script sets up all dependencies required for building the project

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== ESP32 TapGate - Development Environment Setup ===${NC}"
echo ""

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to print status
print_status() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓ $2${NC}"
    else
        echo -e "${RED}✗ $2${NC}"
    fi
}

# Function to print info
print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

# Function to print warning
print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

echo "Checking system requirements..."
echo ""

# Check Python 3
print_info "Checking Python 3..."
if command_exists python3; then
    PYTHON_VERSION=$(python3 --version 2>&1 | cut -d' ' -f2)
    print_status 0 "Python 3 found: $PYTHON_VERSION"
    PYTHON_CMD="python3"
elif command_exists python; then
    PYTHON_VERSION=$(python --version 2>&1 | cut -d' ' -f2)
    if [[ $PYTHON_VERSION == 3.* ]]; then
        print_status 0 "Python 3 found: $PYTHON_VERSION"
        PYTHON_CMD="python"
    else
        print_status 1 "Python 3 required, found Python $PYTHON_VERSION"
        echo "Please install Python 3.7 or later"
        exit 1
    fi
else
    print_status 1 "Python 3 not found"
    echo "Please install Python 3.7 or later"
    exit 1
fi

# Check pip
print_info "Checking pip..."
if command_exists pip3; then
    print_status 0 "pip3 found"
    PIP_CMD="pip3"
elif command_exists pip; then
    print_status 0 "pip found"
    PIP_CMD="pip"
else
    print_status 1 "pip not found"
    echo "Please install pip"
    exit 1
fi

# Check Git
print_info "Checking Git..."
if command_exists git; then
    GIT_VERSION=$(git --version | cut -d' ' -f3)
    print_status 0 "Git found: $GIT_VERSION"
else
    print_status 1 "Git not found"
    echo "Please install Git"
    exit 1
fi

# Check CMake
print_info "Checking CMake..."
if command_exists cmake; then
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    print_status 0 "CMake found: $CMAKE_VERSION"
else
    print_warning "CMake not found - required for building"
    echo "Please install CMake 3.20 or later"
fi

echo ""
echo "Installing Python dependencies..."

# Install protobuf dependencies
print_info "Installing protobuf and grpcio-tools..."
$PIP_CMD install protobuf grpcio-tools
if [ $? -eq 0 ]; then
    print_status 0 "Python protobuf dependencies installed"
else
    print_status 1 "Failed to install Python dependencies"
    exit 1
fi

# Test nanopb generator
echo ""
print_info "Testing nanopb generator..."
cd esp32_mcu/components/nanopb/generator
if $PYTHON_CMD nanopb_generator.py --version >/dev/null 2>&1; then
    NANOPB_VERSION=$($PYTHON_CMD nanopb_generator.py --version 2>&1)
    print_status 0 "nanopb generator working: $NANOPB_VERSION"
else
    print_status 1 "nanopb generator test failed"
    exit 1
fi
cd - >/dev/null

# Check ESP-IDF (optional)
echo ""
print_info "Checking ESP-IDF environment..."
if [ -n "$IDF_PATH" ] && [ -f "$IDF_PATH/tools/cmake/project.cmake" ]; then
    print_status 0 "ESP-IDF found at: $IDF_PATH"
else
    print_warning "ESP-IDF not found or not configured"
    echo "For ESP32 development, please install ESP-IDF and run:"
    echo ". \$HOME/esp/esp-idf/export.sh"
    echo "Or follow: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/"
fi

# Test host build
echo ""
print_info "Testing host build..."
cd esp32_mcu
if TAPGATE_BUILD_MODE=host cmake -S . -B build_setup_test >/dev/null 2>&1; then
    if cmake --build build_setup_test >/dev/null 2>&1; then
        print_status 0 "Host build test successful"
        rm -rf build_setup_test
    else
        print_status 1 "Host build test failed"
        rm -rf build_setup_test
        exit 1
    fi
else
    print_status 1 "Host build configuration failed"
    rm -rf build_setup_test 2>/dev/null
    exit 1
fi
cd - >/dev/null

echo ""
echo -e "${GREEN}=== Setup completed successfully! ===${NC}"
echo ""
echo "Next steps:"
echo "1. For ESP32 development: Install and configure ESP-IDF"
echo "2. Build ESP32 project: cd esp32_mcu && idf.py build"
echo "3. Build host tests: cd esp32_mcu && TAPGATE_BUILD_MODE=host cmake -S . -B build_host && cmake --build build_host"
echo "4. For MAUI client: Open tapgate_client/TapGateClient.sln in Visual Studio"
echo ""