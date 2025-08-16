# NodeToCode Command Line Interface

The NodeToCode plugin now supports command line batch processing through the `N2CExporter` commandlet. This enables automated Blueprint JSON export for CI/CD pipelines and batch processing workflows.

## Installation

The commandlet is automatically registered when the NodeToCode plugin is compiled and loaded in your Unreal Engine project.

## Basic Usage

```bash
UnrealEditor.exe <ProjectFile> -run=N2CExporter [mode] [options]
```

## Export Modes

### Export All Blueprints
Export all Blueprints in the project:
```bash
UnrealEditor.exe MyProject.uproject -run=N2CExporter -ExportAll
```

### Export Specific Directory
Export Blueprints in a specific directory:
```bash
UnrealEditor.exe MyProject.uproject -run=N2CExporter -Path="/Game/Characters"
```

### Export Single Blueprint
Export a single Blueprint:
```bash
UnrealEditor.exe MyProject.uproject -run=N2CExporter -Blueprint="/Game/Characters/PlayerCharacter"
```

## Configuration Options

### File Organization
```bash
# Single file per Blueprint (default)
-Strategy=SingleFile

# Multiple files per graph
-Strategy=MultipleFiles

# Hybrid strategy (single file for ≤3 graphs, multiple files for >3 graphs)
-Strategy=Hybrid -GraphThreshold=3
```

### Output Directory
```bash
# Custom output directory
-OutputDir="Build/Translated"

# Default: Content/Translated/
```

### Directory Filters
```bash
# Include specific directories (can be used multiple times)
-Include="/Game/Characters" -Include="/Game/Weapons"

# Exclude directories (can be used multiple times)
-Exclude="/Game/ThirdParty" -Exclude="/Game/Deprecated"
```

### File Operations
```bash
# Don't overwrite existing files
-NoOverwrite

# Create backup files before overwriting
-CreateBackups

# Use compact JSON format
-CompactJson

# Only export changed blueprints
-Incremental
```

### Performance Options
```bash
# Maximum concurrent processing threads (1-16)
-MaxConcurrent=4

# Skip blueprints larger than N nodes (0 = no limit)
-MaxNodes=1000
```

### Output Control
```bash
# Verbose output
-Verbose
# or
-v

# Quiet mode (minimal output)
-Quiet
# or
-q

# Exit with error code on any failures
-FailOnError
```

## Examples

### Basic CI/CD Export
```bash
# Export all blueprints to build directory
UnrealEditor.exe MyProject.uproject -run=N2CExporter -ExportAll -OutputDir="Build/Translated" -Quiet
```

### Development Workflow
```bash
# Export characters with verbose output and backups
UnrealEditor.exe MyProject.uproject -run=N2CExporter -Path="/Game/Characters" -Strategy=MultipleFiles -CreateBackups -Verbose
```

### Large Project Processing
```bash
# Export with performance optimizations
UnrealEditor.exe MyProject.uproject -run=N2CExporter -ExportAll -MaxConcurrent=8 -MaxNodes=500 -Incremental
```

### Custom Directory Structure
```bash
# Export only specific content with exclusions
UnrealEditor.exe MyProject.uproject -run=N2CExporter -ExportAll -Include="/Game/Core" -Include="/Game/UI" -Exclude="/Game/Core/Deprecated" -OutputDir="Export/Production"
```

## Exit Codes

- **0**: Success (all Blueprints exported successfully)
- **1**: Error (invalid parameters, initialization failure, or failures with `-FailOnError` flag)

## Integration with Build Systems

### GitHub Actions
```yaml
name: Export Blueprint JSON
on: [push, pull_request]

jobs:
  export-blueprints:
    runs-on: windows-latest
    steps:
    - uses: actions/checkout@v2
    
    - name: Setup Unreal Engine
      # Setup UE5 here
      
    - name: Export Blueprints
      run: |
        UnrealEditor.exe MyProject.uproject -run=N2CExporter -ExportAll -OutputDir="Artifacts/Blueprints" -FailOnError
        
    - name: Upload Artifacts
      uses: actions/upload-artifact@v2
      with:
        name: blueprint-json
        path: Artifacts/Blueprints/
```

### Jenkins Pipeline
```groovy
pipeline {
    agent any
    stages {
        stage('Export Blueprints') {
            steps {
                bat '''
                    UnrealEditor.exe MyProject.uproject -run=N2CExporter -ExportAll -Quiet -FailOnError
                '''
            }
        }
        stage('Archive') {
            steps {
                archiveArtifacts artifacts: 'Content/Translated/**/*.json', fingerprint: true
            }
        }
    }
}
```

### Batch Script
```batch
@echo off
setlocal

set PROJECT_FILE=MyProject.uproject
set UE_EDITOR="C:\UE\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe"
set OUTPUT_DIR=Build\Translated

echo Exporting Blueprint JSON files...
%UE_EDITOR% %PROJECT_FILE% -run=N2CExporter -ExportAll -OutputDir="%OUTPUT_DIR%" -CreateBackups

if %ERRORLEVEL% equ 0 (
    echo Export completed successfully
) else (
    echo Export failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
```

## Troubleshooting

### Common Issues

1. **Commandlet not found**: Ensure the NodeToCode plugin is enabled and compiled in your project
2. **Asset registry timeout**: Use `-Verbose` to see asset scanning progress
3. **Permission errors**: Run with administrator privileges if writing to protected directories
4. **Memory issues**: Reduce `-MaxConcurrent` value or use `-MaxNodes` to skip large Blueprints

### Debug Output
Use `-Verbose` flag to see detailed processing information:
```bash
UnrealEditor.exe MyProject.uproject -run=N2CExporter -ExportAll -Verbose
```

### Log Files
Check the Unreal Engine log files for detailed error information:
- `Saved/Logs/`
- Look for entries with `LogNodeToCode` or `LogTemp` categories

## Limitations

- Requires Unreal Engine Editor (cannot run in headless mode without editor)
- Asset registry must complete scanning before export begins
- Some Blueprint types may not be translatable (will be skipped with warnings)
- File system permissions must allow writing to output directory

## Performance Notes

- Export time scales with Blueprint complexity and count
- Parallel processing (`-MaxConcurrent`) can significantly improve performance
- Incremental export (`-Incremental`) only processes changed Blueprints
- Large Blueprints (>1000 nodes) may take several seconds each to process