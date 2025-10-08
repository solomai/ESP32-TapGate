#!/bin/bash
# Script to regenerate C# protobuf files for MAUI project using NuGet protoc
# Run this script whenever proto files change

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
PROTO_DIR="$PROJECT_ROOT/proto"
OUTPUT_DIR="$SCRIPT_DIR/Core/Messages"

echo "Regenerating C# protobuf files using NuGet protoc..."
echo "Script directory: $SCRIPT_DIR"
echo "Project root: $PROJECT_ROOT"
echo "Proto directory: $PROTO_DIR"

# Check if proto files exist
if [ ! -f "$PROTO_DIR/messages.proto" ]; then
    echo "Error: Proto file not found: $PROTO_DIR/messages.proto"
    exit 1
fi

# Find protoc in NuGet packages (should be already available from previous restore)
PROTOC_PATH=""
NUGET_BASE="$HOME/.nuget/packages"

# Look for Grpc.Tools package
for version_dir in "$NUGET_BASE/grpc.tools"/*; do
    if [ -d "$version_dir" ]; then
        if [ -f "$version_dir/tools/linux_x64/protoc" ]; then
            PROTOC_PATH="$version_dir/tools/linux_x64/protoc"
            break
        elif [ -f "$version_dir/tools/macosx_x64/protoc" ]; then
            PROTOC_PATH="$version_dir/tools/macosx_x64/protoc"
            break
        fi
    fi
done

if [ -z "$PROTOC_PATH" ]; then
    echo "Error: protoc not found in NuGet packages"
    echo "Try running: dotnet restore (may fail on Linux but protoc will be available)"
    exit 1
fi

echo "Found NuGet protoc at: $PROTOC_PATH"
chmod +x "$PROTOC_PATH"

# Test protoc
if ! "$PROTOC_PATH" --version >/dev/null 2>&1; then
    echo "Error: protoc not working: $PROTOC_PATH"
    exit 1
fi

# Create output directory and clean old files
mkdir -p "$OUTPUT_DIR"
echo "Cleaning old generated files..."
rm -f "$OUTPUT_DIR"/*.cs "$OUTPUT_DIR"/*.g.cs 2>/dev/null || true

# Generate C# files from proto using NuGet protoc
echo "Generating C# classes from messages.proto..."
"$PROTOC_PATH" \
    --proto_path="$PROTO_DIR" \
    --csharp_out="$OUTPUT_DIR" \
    --csharp_opt=file_extension=.g.cs \
    "$PROTO_DIR/messages.proto"

if [ $? -eq 0 ]; then
    echo "[OK] C# protobuf files generated successfully!"
    echo "Files generated in: $OUTPUT_DIR"
    ls -la "$OUTPUT_DIR"/*.g.cs 2>/dev/null || ls -la "$OUTPUT_DIR"/*.cs 2>/dev/null || echo "No generated files found"
    
    echo ""
    echo "Note: MSBuild will automatically generate these files during build."
    echo "Manual generation is only needed for development/debugging."
else
    echo "[ERROR] Failed to generate C# protobuf files"
    exit 1
fi