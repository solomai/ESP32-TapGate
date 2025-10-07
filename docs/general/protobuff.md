# ESP32 TapGate - Protobuf Configuration

## Project Structure

This project is configured for automatic protobuf file generation using nanopb.

### Components

- `components/nanopb/` - nanopb library component for protobuf
- `messages/` - component with automatically generated protobuf structures (at same level as components/)
- `proto/` - source proto files (located in project root)

### Protobuf File Generation

Protobuf files are pre-generated and stored in the repository. When you modify `.proto` files, you need to regenerate them manually.

#### Source Files:
- `proto/messages.proto` - message definitions
- `proto/messages.options` - nanopb generator settings

#### Generated Files:
- `messages/messages.pb.h` - header file with structures
- `messages/messages.pb.c` - encoding/decoding implementation

#### Regenerating Protobuf Files

When you modify `proto/messages.proto` or `proto/messages.options`, regenerate the files:

**Windows:**
```cmd
cd esp32_mcu
regenerate-protobuf.bat
```

**Unix/Linux/macOS:**
```bash
cd esp32_mcu
./regenerate-protobuf.sh
```

**Manual regeneration:**
```bash
cd esp32_mcu/components/nanopb/generator
python3 nanopb_generator.py -D ../../messages -f ../../../proto/messages.options -I ../../../proto ../../../proto/messages.proto
```

### Building the Project

#### For ESP32 (requires ESP-IDF):
```bash
cd esp32_mcu
idf.py build
```

#### For host tests:
```bash
cd esp32_mcu
TAPGATE_BUILD_MODE=host cmake -S . -B build_host
cmake --build build_host
```

### Protobuf Testing

The `main.c` file includes a `test_protobuf_messages()` function that demonstrates:
- Creating a message
- Encoding to binary format
- Decoding back to structure
- Outputting results to log

### nanopb Configuration

In the `proto/messages.options` file:
- Maximum payload string size set to 256 characters
- Static types enabled for memory optimization

### Adding New Messages

1. Edit `proto/messages.proto`
2. Update `proto/messages.options` if needed
3. Regenerate protobuf files using the regeneration script
4. Build the project
5. Include `messages.pb.h` in your files
6. Use `pb_encode()` and `pb_decode()` to work with messages

### Dependency Setup

#### Automatic Setup (Recommended)

Run the setup script from the project root to automatically install all dependencies:

**Windows:**
```cmd
setup-dev-env.bat
```

**Unix/Linux/macOS:**
```bash
./setup-dev-env.sh
```

#### Quick ESP32-only Setup

If you only need ESP32 protobuf dependencies:

**Windows:**
```cmd
cd esp32_mcu
quick-setup.bat
```

**Unix/Linux/macOS:**
```bash
cd esp32_mcu
./quick-setup.sh
```

#### Manual Setup

Ensure Python 3 and required packages are installed:
```bash
pip install protobuf grpcio-tools
```