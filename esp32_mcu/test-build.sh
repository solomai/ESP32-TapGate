#!/bin/bash

# Simulate ESP-IDF build environment for testing
export IDF_PATH="/fake/esp-idf"

echo "Testing protobuf auto-generation..."

# Remove generated files
rm -f messages/messages.pb.h messages/messages.pb.c

# Test CMake configure phase
cd messages
cmake -P CMakeLists.txt

# Check if files were generated
if [ -f "messages.pb.h" ] && [ -f "messages.pb.c" ]; then
    echo "✓ Files generated successfully!"
    ls -la messages.pb.*
else
    echo "✗ Files not generated"
    echo "Trying manual script..."
    cd ..
    ./regenerate-protobuf.sh
fi