#!/bin/bash
# Script to regenerate protobuf files for ESP32 project
# Run this script whenever proto files change

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Regenerating protobuf files..."

# Navigate to nanopb generator directory
cd "$SCRIPT_DIR/components/nanopb/generator"

# Make tools executable
chmod +x protoc protoc-gen-nanopb

# Check if protoc works
if ! ./protoc --version >/dev/null 2>&1; then
    echo "Error: protoc not working"
    exit 1
fi

# Generate protobuf files using the official nanopb way
echo "Generating messages.pb.h and messages.pb.c..."
./protoc \
    --plugin=protoc-gen-nanopb=./protoc-gen-nanopb \
    --nanopb_out="$SCRIPT_DIR/messages" \
    --nanopb_opt=-f"$PROJECT_ROOT/proto/messages.options" \
    -I"$PROJECT_ROOT/proto" \
    "$PROJECT_ROOT/proto/messages.proto"

if [ $? -eq 0 ]; then
    echo "✓ Protobuf files regenerated successfully!"
    echo "Files updated:"
    echo "  - messages/messages.pb.h"
    echo "  - messages/messages.pb.c"
else
    echo "✗ Failed to regenerate protobuf files"
    exit 1
fi

echo "You can now build the project with: idf.py build"