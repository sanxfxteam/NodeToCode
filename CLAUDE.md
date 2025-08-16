# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Node to Code** is an Unreal Engine 5 plugin that translates Blueprint graphs into readable code in multiple languages (C++, C#, JavaScript, Python, Swift, Pseudocode). The plugin uses LLM providers (OpenAI, Anthropic, Google, DeepSeek, Ollama) to perform the translation after converting Blueprint structures into an efficient JSON format.

## Build System & Development Commands

This is an Unreal Engine plugin project that follows UE's module system:

- **Build File**: `Source/NodeToCode.Build.cs` defines the module dependencies
- **Plugin Descriptor**: `NodeToCode.uplugin` contains plugin metadata and module configuration
- **Module Type**: Editor plugin (loads at PostEngineInit phase)
- **Platforms**: Windows 64-bit and macOS

### Dependencies
The plugin depends on core UE modules including BlueprintGraph, GraphEditor, HTTP, JSON, and UMG for UI components.

### Compiling the Plugin

To compile the NodeToCode plugin from the command line:

**Method 1: Using RunUAT BuildPlugin (Recommended)**
```bash
# Build and package the plugin for distribution
"C:\UE\UE_5.5_AS_2\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="C:\Projects\Testbeds\NodeToCode\NodeToCode.uplugin" -Package="C:\Projects\Testbeds\NodeToCode\Package" -CreateSubFolder
```

**Method 2: Using UnrealBuildTool directly**
```bash
# From the project root directory
"C:\UE\UE_5.5_AS_2\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -ModuleFiles="Source\NodeToCode.Build.cs" -Platform=Win64 -Configuration=Development
```

**Build Requirements:**
- Visual Studio 2022 with C++ development tools
- Windows 10/11 SDK
- .NET SDK 8.0.300 (bundled with UE5)
- Unreal Engine 5.5 installed at `C:\UE\UE_5.5_AS_2\`

**Build Output:**
- Compiled binaries: `Binaries/Win64/UnrealEditor-NodeToCode.dll`
- Generated headers: `Intermediate/Build/Win64/UnrealEditor/Inc/NodeToCode/UHT/`
- Log files: `%LOCALAPPDATA%\UnrealBuildTool\Log.txt`

## Code Architecture

### Core Translation Pipeline
The plugin follows a multi-stage pipeline to convert Blueprints to code:

1. **Collection** (`N2CNodeCollector`): Extracts nodes from Blueprint Editor
2. **Translation** (`N2CNodeTranslator`): Converts UE nodes to structured N2C format
3. **Serialization** (`N2CSerializer`): Converts to optimized JSON
4. **LLM Processing**: Sends JSON to LLM provider with language-specific prompts
5. **Display** (`N2CCodeEditorWidget`): Shows results in integrated editor

### Key Components

**Core System** (`Source/Public/Core/`):
- `N2CEditorIntegration`: Blueprint Editor toolbar integration
- `N2CNodeCollector`: Extracts nodes from Blueprint graphs with depth control
- `N2CNodeTranslator`: Main conversion engine from UE nodes to N2C structures
- `N2CSerializer`: JSON serialization with 60-90% token reduction vs UE format

**LLM Integration** (`Source/Public/LLM/`):
- `N2CLLMProviderRegistry`: Factory system for different LLM providers
- `N2CBaseLLMService`: Base class for all LLM integrations
- Provider-specific implementations for OpenAI, Anthropic, Google, DeepSeek, Ollama
- `N2CSystemPromptManager`: Manages language-specific prompt templates

**Node Processing** (`Source/Public/Utils/Processors/`):
- `N2CNodeProcessorFactory`: Creates appropriate processors for different node types
- Specialized processors for arrays, casts, delegates, events, function calls, etc.
- `N2CBaseNodeProcessor`: Common interface for all node type handlers

**Code Editor** (`Source/Public/Code Editor/`):
- `N2CCodeEditorWidget`: Integrated editor with syntax highlighting
- Language-specific syntax definitions (C++, C#, JavaScript, Python, Swift)
- `N2CRichTextSyntaxHighlighter`: Handles syntax coloring and formatting

### Data Models

**Blueprint Representation** (`Source/Public/Models/`):
- `FN2CBlueprint`: Root structure containing metadata and graphs
- `FN2CNode`: Individual node with pins, properties, and type information
- `FN2CPin`: Pin connections with type and flow information
- Custom structs and enums are captured and included in translations

### System Architecture Patterns

- **Factory Pattern**: Used extensively for LLM providers and node processors
- **Singleton Pattern**: Core managers like `N2CEditorIntegration` and `N2CNodeTranslator`
- **Registry Pattern**: `N2CLLMProviderRegistry` for dynamic provider registration
- **Visitor Pattern**: Node processors handle different node types polymorphically

## Language-Specific Prompts

The plugin includes sophisticated prompt templates in `Content/Prompting/` that guide LLMs in producing idiomatic code for each target language. These prompts include detailed specifications for the N2C JSON format and language-specific translation rules.

## Integration Points

- **Blueprint Editor**: Adds toolbar buttons for translation and JSON export
- **UE Asset System**: Integrates with Blueprint asset editors
- **HTTP Module**: For cloud LLM provider communication
- **JSON Module**: For efficient Blueprint serialization

## Code Conventions

- All classes use `N2C` prefix for namespacing
- Follows UE coding standards with Pascal case for public methods
- Extensive use of UE reflection system (UCLASS, USTRUCT, UENUM macros)
- Comments follow Doxygen format with @brief, @param, @return tags
- Copyright headers on all source files: "Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved."