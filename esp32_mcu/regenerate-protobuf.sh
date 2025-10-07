#!/bin/bash
# Script to regenerate protobuf files for ESP32 project
# Run this script whenever proto files change

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Regenerating protobuf files..."

# Navigate to nanopb generator directory
cd "$SCRIPT_DIR/components/nanopb/generator"

# Check if Python and nanopb are available
if ! command -v python3 >/dev/null 2>&1; then
    echo "Error: Python 3 not found"
    exit 1
fi

if ! python3 nanopb_generator.py --version >/dev/null 2>&1; then
    echo "Error: nanopb generator not working"
    echo "Run setup script first: ../../quick-setup.sh"
    exit 1
fi

# Generate protobuf files
echo "Generating messages.pb.h and messages.pb.c..."
python3 nanopb_generator.py \
    -D "$SCRIPT_DIR/messages" \
    -f "$PROJECT_ROOT/proto/messages.options" \
    -I "$PROJECT_ROOT/proto" \
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