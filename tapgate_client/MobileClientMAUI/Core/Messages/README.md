# Protobuf Integration Optimization

## ✅ Optimization Complete

The project now uses MSBuild-integrated protobuf artifact generation instead of separate bat/sh scripts.

## What Was Changed

### ❌ Removed
- `regenerate-protobuf.bat` - no longer needed
- `regenerate-protobuf.sh` - no longer needed

### ✅ Added to TapGate.csproj
- Automatic protobuf file generation in `Core/Messages/` during build
- `RegenerateProtobuf` target for manual regeneration  
- `CleanProtobufFiles` target for cleaning generated files
- Automatic check for regeneration necessity

## How It Works

### Automatic Generation
- Occurs during `dotnet build` if `Core/Messages/Messages.pb.cs` file doesn't exist
- Uses protoc from `Grpc.Tools` NuGet package
- Generates files directly into `Core/Messages/` folder with `.pb.cs` extension

### Manual Regeneration
```bash
# Full regeneration (deletes and recreates)
dotnet msbuild -target:RegenerateProtobuf

# Or via build
dotnet build -target:RegenerateProtobuf
```

### Cleanup
```bash
# Removes generated files
dotnet clean
```

## Advantages of New Approach

1. **MSBuild Integration**: Everything happens within standard .NET build process
2. **Automation**: No need to remember to run separate scripts
3. **Cross-platform**: Works on all platforms without additional scripts
4. **Reliability**: Uses standard .NET mechanisms for dependency tracking
5. **Simplicity**: Fewer files to maintain

## Configuration

Configured in `TapGate.csproj`:
- `ProtoRoot` = `../../proto` - folder with .proto files
- `ProtobufOutputPath` = `Core/Messages` - folder for C# files  
- Uses protoc from `grpc.tools/2.67.0` package with cross-platform path detection:
  - **Windows**: `%USERPROFILE%\.nuget\packages\grpc.tools\2.67.0\tools\windows_x64\protoc.exe`
  - **Linux**: `$HOME/.nuget/packages/grpc.tools/2.67.0/tools/linux_x64/protoc`
  - **macOS**: `$HOME/.nuget/packages/grpc.tools/2.67.0/tools/macosx_x64/protoc`
  - **Fallback**: System `protoc` if package not found

## Troubleshooting

1. Ensure `Grpc.Tools` NuGet package is installed
2. Verify that `proto/messages.proto` file exists
3. Run `dotnet restore` to download protoc
4. Check that `Core/Messages/Messages.pb.cs` is generated with proper `.pb.cs` extension
5. For debugging use `dotnet msbuild -verbosity:detailed`