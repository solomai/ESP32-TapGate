#!/bin/bash
# Quick setup script for ESP32 MCU protobuf dependencies only
# Use this if you only need to work with ESP32 part

set -e

echo "ESP32 TapGate - Quick ESP32 Setup"
echo "Installing Python protobuf dependencies..."

# Detect Python and pip
if command -v python3 >/dev/null 2>&1; then
    PYTHON_CMD="python3"
    PIP_CMD="pip3"
elif command -v python >/dev/null 2>&1; then
    PYTHON_CMD="python"
    PIP_CMD="pip"
else
    echo "Error: Python 3 not found"
    exit 1
fi

# Install dependencies
$PIP_CMD install protobuf grpcio-tools

# Test nanopb
echo "Testing nanopb generator..."
cd "$(dirname "$0")/esp32_mcu/components/nanopb/generator"
$PYTHON_CMD nanopb_generator.py --version

echo "✓ ESP32 protobuf setup complete!"
echo "You can now build the ESP32 project."