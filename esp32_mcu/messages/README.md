# Messages Component

This component contains pre-generated nanopb protobuf files for ESP32 communication.

## Files

- `messages.pb.h` - Generated header file with message definitions
- `messages.pb.c` - Generated source file with message descriptors  
- `CMakeLists.txt` - Component build configuration

## Generated Files Included

The protobuf files are **pre-generated and included** in the repository for immediate building. 
You can build the ESP32 project right away without any additional steps.

## Regenerating Files (Optional)

If you modify the proto files (`../../proto/messages.proto` or `../../proto/messages.options`), 
you can regenerate the nanopb files **manually**:

### Linux/macOS:
```bash
cd esp32_mcu
./regenerate-protobuf.sh
```

### Windows:
```cmd
cd esp32_mcu
regenerate-protobuf.bat
```

## Build Process

1. **Normal build**: Just run `idf.py build` - files are already included
2. **After proto changes**: Run regenerate script first, then build
3. **Clean build**: If issues occur, try `idf.py clean` then `idf.py build`

## Dependencies

This component depends on the nanopb component which provides:
- `pb.h` - Main nanopb header
- `pb_encode.h` - Encoding functions  
- `pb_decode.h` - Decoding functions

## Technical Notes

- Uses official nanopb protoc plugin approach
- Files generated with nanopb 0.4.9.1
- Compatible with ESP-IDF 5.5
- No automatic regeneration during build to avoid toolchain dependencies