// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#include "Core/N2CExporterCommandlet.h"
#include "Core/N2CTranslatorManager.h"
#include "Core/N2CBatchProcessor.h"
#include "Core/N2CBlueprintDiscovery.h"
#include "Utils/N2CLogger.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformFilemanager.h"

#define LOCTEXT_NAMESPACE "N2CExporterCommandlet"

UN2CExporterCommandlet::UN2CExporterCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
    ShowErrorCount = true;
    
    HelpDescription = TEXT("Exports Blueprint JSON files using NodeToCode translation system");
    HelpUsage = TEXT("UnrealEditor.exe <ProjectFile> -run=N2CExporter [options]");
}

int32 UN2CExporterCommandlet::Main(const FString& Params)
{
    UE_LOG(LogTemp, Log, TEXT("NodeToCode Exporter Commandlet started"));
    
    // Parse command line parameters
    FN2CCommandLineOptions Options;
    if (!ParseCommandLineParams(Params, Options))
    {
        PrintUsage();
        return 1;
    }

    // Handle help request
    if (Options.bShowHelp || Options.Mode == FN2CCommandLineOptions::EExportMode::Help)
    {
        PrintUsage();
        return 0;
    }

    // Set verbosity
    bVerboseOutput = Options.bVerbose;
    bQuietMode = Options.bQuiet;

    // Validate options
    FString ValidationError;
    if (!ValidateOptions(Options, ValidationError))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid options: %s"), *ValidationError);
        return 1;
    }

    // Initialize translator system
    if (!InitializeTranslatorSystem())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to initialize translator system"));
        return 1;
    }

    int32 ExitCode = 0;

    // Execute based on mode
    switch (Options.Mode)
    {
        case FN2CCommandLineOptions::EExportMode::ExportAll:
            ExitCode = ExecuteExportAll(Options);
            break;
            
        case FN2CCommandLineOptions::EExportMode::ExportPath:
            ExitCode = ExecuteExportPath(Options);
            break;
            
        case FN2CCommandLineOptions::EExportMode::ExportBlueprint:
            ExitCode = ExecuteExportBlueprint(Options);
            break;
            
        default:
            UE_LOG(LogTemp, Error, TEXT("Unknown export mode"));
            PrintUsage();
            ExitCode = 1;
            break;
    }

    // Cleanup
    CleanupTranslatorSystem();

    if (!bQuietMode)
    {
        UE_LOG(LogTemp, Log, TEXT("NodeToCode Exporter Commandlet completed with exit code: %d"), ExitCode);
    }

    return ExitCode;
}

bool UN2CExporterCommandlet::ParseCommandLineParams(const FString& Params, FN2CCommandLineOptions& OutOptions)
{
    TArray<FString> Tokens;
    TArray<FString> Switches;
    ParseCommandLine(*Params, Tokens, Switches);

    // Check for help request
    if (Switches.Contains(TEXT("help")) || Switches.Contains(TEXT("h")) || Switches.Contains(TEXT("?")))
    {
        OutOptions.bShowHelp = true;
        OutOptions.Mode = FN2CCommandLineOptions::EExportMode::Help;
        return true;
    }

    // Determine export mode
    if (Switches.Contains(TEXT("ExportAll")))
    {
        OutOptions.Mode = FN2CCommandLineOptions::EExportMode::ExportAll;
    }
    else if (Switches.Contains(TEXT("Path")))
    {
        OutOptions.Mode = FN2CCommandLineOptions::EExportMode::ExportPath;
        
        // Find path parameter
        for (const FString& Token : Tokens)
        {
            if (Token.StartsWith(TEXT("Path=")))
            {
                OutOptions.Path = Token.Mid(5);
                break;
            }
        }
        
        if (OutOptions.Path.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("Path mode requires -Path parameter"));
            return false;
        }
    }
    else if (Switches.Contains(TEXT("Blueprint")))
    {
        OutOptions.Mode = FN2CCommandLineOptions::EExportMode::ExportBlueprint;
        
        // Find blueprint parameter
        for (const FString& Token : Tokens)
        {
            if (Token.StartsWith(TEXT("Blueprint=")))
            {
                OutOptions.BlueprintPath = Token.Mid(10);
                break;
            }
        }
        
        if (OutOptions.BlueprintPath.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("Blueprint mode requires -Blueprint parameter"));
            return false;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No export mode specified. Use -ExportAll, -Path, or -Blueprint"));
        return false;
    }

    // Parse optional parameters
    for (const FString& Token : Tokens)
    {
        if (Token.StartsWith(TEXT("OutputDir=")))
        {
            OutOptions.OutputDirectory = Token.Mid(10);
        }
        else if (Token.StartsWith(TEXT("Strategy=")))
        {
            FString StrategyStr = Token.Mid(9);
            if (StrategyStr == TEXT("SingleFile"))
            {
                OutOptions.FileStrategy = EN2CFileStrategy::SingleFilePerBlueprint;
            }
            else if (StrategyStr == TEXT("MultipleFiles"))
            {
                OutOptions.FileStrategy = EN2CFileStrategy::MultipleFilesPerGraph;
            }
            else if (StrategyStr == TEXT("Hybrid"))
            {
                OutOptions.FileStrategy = EN2CFileStrategy::Hybrid;
            }
        }
        else if (Token.StartsWith(TEXT("GraphThreshold=")))
        {
            OutOptions.GraphCountThreshold = FCString::Atoi(*Token.Mid(15));
        }
        else if (Token.StartsWith(TEXT("MaxConcurrent=")))
        {
            OutOptions.MaxConcurrentProcessing = FCString::Atoi(*Token.Mid(14));
        }
        else if (Token.StartsWith(TEXT("MaxNodes=")))
        {
            OutOptions.MaxNodesPerBlueprint = FCString::Atoi(*Token.Mid(9));
        }
        else if (Token.StartsWith(TEXT("Include=")))
        {
            FString IncludeDir = Token.Mid(8);
            OutOptions.IncludeDirectories.AddUnique(IncludeDir);
        }
        else if (Token.StartsWith(TEXT("Exclude=")))
        {
            FString ExcludeDir = Token.Mid(8);
            OutOptions.ExcludeDirectories.AddUnique(ExcludeDir);
        }
    }

    // Parse switches
    OutOptions.bOverwriteExisting = !Switches.Contains(TEXT("NoOverwrite"));
    OutOptions.bCreateBackups = Switches.Contains(TEXT("CreateBackups"));
    OutOptions.bPrettyPrintJson = !Switches.Contains(TEXT("CompactJson"));
    OutOptions.bIncrementalExport = Switches.Contains(TEXT("Incremental"));
    OutOptions.bVerbose = Switches.Contains(TEXT("Verbose")) || Switches.Contains(TEXT("v"));
    OutOptions.bQuiet = Switches.Contains(TEXT("Quiet")) || Switches.Contains(TEXT("q"));
    OutOptions.bFailOnError = Switches.Contains(TEXT("FailOnError"));

    return true;
}

int32 UN2CExporterCommandlet::ExecuteExportAll(const FN2CCommandLineOptions& Options)
{
    if (bVerboseOutput)
    {
        UE_LOG(LogTemp, Log, TEXT("Executing export all blueprints"));
    }

    // Create settings from options
    UN2CTranslatorSettings* Settings = CreateSettingsFromOptions(Options);
    
    // Setup progress delegates
    FOnN2CProgressUpdate ProgressDelegate;
    FOnN2CBlueprintProcessed ProcessedDelegate;
    FOnN2CError ErrorDelegate;
    
    if (!bQuietMode)
    {
        ProgressDelegate.BindLambda([this](int32 Current, int32 Total) { OnProgressUpdate(Current, Total); });
        ProcessedDelegate.BindLambda([this](const FString& BlueprintName) { OnBlueprintProcessed(BlueprintName); });
        ErrorDelegate.BindLambda([this](const FString& ErrorMessage) { OnError(ErrorMessage); });
    }

    // Execute batch processing
    FN2CBatchResult Result = FN2CBatchProcessor::ProcessAllBlueprints(
        Settings, 
        ProgressDelegate, 
        ProcessedDelegate, 
        ErrorDelegate
    );

    // Print results
    if (!bQuietMode)
    {
        PrintResults(Result);
    }

    // Determine exit code
    if (Result.bSuccess)
    {
        return 0;
    }
    else if (Options.bFailOnError)
    {
        return 1;
    }
    else
    {
        // Partial success - return 0 but log warnings
        UE_LOG(LogTemp, Warning, TEXT("Export completed with %d failures"), Result.Failed);
        return 0;
    }
}

int32 UN2CExporterCommandlet::ExecuteExportPath(const FN2CCommandLineOptions& Options)
{
    if (bVerboseOutput)
    {
        UE_LOG(LogTemp, Log, TEXT("Executing export for path: %s"), *Options.Path);
    }

    // Create settings from options
    UN2CTranslatorSettings* Settings = CreateSettingsFromOptions(Options);
    
    // Override include directories to match specified path
    Settings->IncludeDirectories.Empty();
    Settings->IncludeDirectories.Add(Options.Path);

    // Setup progress delegates
    FOnN2CProgressUpdate ProgressDelegate;
    FOnN2CBlueprintProcessed ProcessedDelegate;
    FOnN2CError ErrorDelegate;
    
    if (!bQuietMode)
    {
        ProgressDelegate.BindLambda([this](int32 Current, int32 Total) { OnProgressUpdate(Current, Total); });
        ProcessedDelegate.BindLambda([this](const FString& BlueprintName) { OnBlueprintProcessed(BlueprintName); });
        ErrorDelegate.BindLambda([this](const FString& ErrorMessage) { OnError(ErrorMessage); });
    }

    // Execute batch processing for path
    FN2CBatchResult Result = FN2CBatchProcessor::ProcessBlueprintsInDirectory(
        Options.Path,
        Settings,
        ProgressDelegate,
        ProcessedDelegate,
        ErrorDelegate
    );

    // Print results
    if (!bQuietMode)
    {
        PrintResults(Result);
    }

    // Determine exit code
    if (Result.bSuccess)
    {
        return 0;
    }
    else if (Options.bFailOnError)
    {
        return 1;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Export completed with %d failures"), Result.Failed);
        return 0;
    }
}

int32 UN2CExporterCommandlet::ExecuteExportBlueprint(const FN2CCommandLineOptions& Options)
{
    if (bVerboseOutput)
    {
        UE_LOG(LogTemp, Log, TEXT("Executing export for blueprint: %s"), *Options.BlueprintPath);
    }

    // Load the specific blueprint
    FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    
    FAssetData AssetData = AssetRegistry.Get().GetAssetByObjectPath(FSoftObjectPath(Options.BlueprintPath));
    if (!AssetData.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Blueprint not found: %s"), *Options.BlueprintPath);
        return 1;
    }

    UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load blueprint: %s"), *Options.BlueprintPath);
        return 1;
    }

    // Create settings from options
    UN2CTranslatorSettings* Settings = CreateSettingsFromOptions(Options);

    // Export single blueprint
    FN2CExportResult Result = FN2CTranslatorManager::Get().ExportBlueprint(Blueprint, Settings);

    // Print results
    if (!bQuietMode)
    {
        if (Result.bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("Successfully exported: %s"), *Result.OutputPath);
            UE_LOG(LogTemp, Log, TEXT("Bytes written: %d"), Result.BytesWritten);
            UE_LOG(LogTemp, Log, TEXT("Processing time: %.2fs"), Result.ProcessingTime);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Export failed: %s"), *Result.ErrorMessage);
        }
    }

    return Result.bSuccess ? 0 : 1;
}

UN2CTranslatorSettings* UN2CExporterCommandlet::CreateSettingsFromOptions(const FN2CCommandLineOptions& Options)
{
    UN2CTranslatorSettings* Settings = NewObject<UN2CTranslatorSettings>();
    
    // Configure settings from options
    Settings->bAutoExportEnabled = true;
    Settings->ExportTrigger = EN2CExportTrigger::Manual;
    Settings->bShowProgressDialog = false; // No UI in commandlet
    
    if (!Options.OutputDirectory.IsEmpty())
    {
        Settings->ExportDirectory = Options.OutputDirectory;
    }
    
    Settings->FileStrategy = Options.FileStrategy;
    Settings->GraphCountThreshold = Options.GraphCountThreshold;
    Settings->IncludeDirectories = Options.IncludeDirectories;
    Settings->ExcludeDirectories = Options.ExcludeDirectories;
    Settings->bOverwriteExisting = Options.bOverwriteExisting;
    Settings->bCreateBackups = Options.bCreateBackups;
    Settings->bPrettyPrintJson = Options.bPrettyPrintJson;
    Settings->MaxConcurrentProcessing = Options.MaxConcurrentProcessing;
    Settings->MaxNodesPerBlueprint = Options.MaxNodesPerBlueprint;
    Settings->bIncrementalExport = Options.bIncrementalExport;

    return Settings;
}

void UN2CExporterCommandlet::PrintUsage()
{
    UE_LOG(LogTemp, Log, TEXT("NodeToCode Exporter Commandlet"));
    UE_LOG(LogTemp, Log, TEXT(""));
    UE_LOG(LogTemp, Log, TEXT("Usage:"));
    UE_LOG(LogTemp, Log, TEXT("  UnrealEditor.exe <ProjectFile> -run=N2CExporter [mode] [options]"));
    UE_LOG(LogTemp, Log, TEXT(""));
    UE_LOG(LogTemp, Log, TEXT("Modes:"));
    UE_LOG(LogTemp, Log, TEXT("  -ExportAll                    Export all blueprints in the project"));
    UE_LOG(LogTemp, Log, TEXT("  -Path=<path>                  Export blueprints in specific directory"));
    UE_LOG(LogTemp, Log, TEXT("  -Blueprint=<path>             Export single blueprint"));
    UE_LOG(LogTemp, Log, TEXT("  -help                         Show this help"));
    UE_LOG(LogTemp, Log, TEXT(""));
    UE_LOG(LogTemp, Log, TEXT("Options:"));
    UE_LOG(LogTemp, Log, TEXT("  -OutputDir=<dir>              Custom output directory (default: Translated)"));
    UE_LOG(LogTemp, Log, TEXT("  -Strategy=<strategy>          File strategy: SingleFile, MultipleFiles, Hybrid"));
    UE_LOG(LogTemp, Log, TEXT("  -GraphThreshold=<n>           Graph count threshold for hybrid strategy"));
    UE_LOG(LogTemp, Log, TEXT("  -MaxConcurrent=<n>            Maximum concurrent processing threads"));
    UE_LOG(LogTemp, Log, TEXT("  -MaxNodes=<n>                 Skip blueprints larger than N nodes"));
    UE_LOG(LogTemp, Log, TEXT("  -Include=<path>               Include directory (can be used multiple times)"));
    UE_LOG(LogTemp, Log, TEXT("  -Exclude=<path>               Exclude directory (can be used multiple times)"));
    UE_LOG(LogTemp, Log, TEXT("  -NoOverwrite                  Don't overwrite existing files"));
    UE_LOG(LogTemp, Log, TEXT("  -CreateBackups                Create .bak files before overwriting"));
    UE_LOG(LogTemp, Log, TEXT("  -CompactJson                  Use compact JSON format"));
    UE_LOG(LogTemp, Log, TEXT("  -Incremental                  Only export changed blueprints"));
    UE_LOG(LogTemp, Log, TEXT("  -Verbose (-v)                 Verbose output"));
    UE_LOG(LogTemp, Log, TEXT("  -Quiet (-q)                   Minimal output"));
    UE_LOG(LogTemp, Log, TEXT("  -FailOnError                  Exit with error code on any failures"));
    UE_LOG(LogTemp, Log, TEXT(""));
    UE_LOG(LogTemp, Log, TEXT("Examples:"));
    UE_LOG(LogTemp, Log, TEXT("  # Export all blueprints"));
    UE_LOG(LogTemp, Log, TEXT("  UnrealEditor.exe MyProject.uproject -run=N2CExporter -ExportAll"));
    UE_LOG(LogTemp, Log, TEXT(""));
    UE_LOG(LogTemp, Log, TEXT("  # Export characters directory with multiple files strategy"));
    UE_LOG(LogTemp, Log, TEXT("  UnrealEditor.exe MyProject.uproject -run=N2CExporter -Path=\"/Game/Characters\" -Strategy=MultipleFiles"));
    UE_LOG(LogTemp, Log, TEXT(""));
    UE_LOG(LogTemp, Log, TEXT("  # Export to build directory with custom settings"));
    UE_LOG(LogTemp, Log, TEXT("  UnrealEditor.exe MyProject.uproject -run=N2CExporter -ExportAll -OutputDir=\"Build/Translated\" -CreateBackups"));
}

void UN2CExporterCommandlet::PrintResults(const FN2CBatchResult& Result)
{
    UE_LOG(LogTemp, Log, TEXT(""));
    UE_LOG(LogTemp, Log, TEXT("Export Results:"));
    UE_LOG(LogTemp, Log, TEXT("  Total Blueprints: %d"), Result.TotalBlueprints);
    UE_LOG(LogTemp, Log, TEXT("  Successfully Exported: %d"), Result.ProcessedSuccessfully);
    UE_LOG(LogTemp, Log, TEXT("  Failed: %d"), Result.Failed);
    UE_LOG(LogTemp, Log, TEXT("  Total Processing Time: %.2fs"), Result.TotalProcessingTime);
    UE_LOG(LogTemp, Log, TEXT("  Total Bytes Written: %d"), Result.TotalBytesWritten);
    
    if (Result.Failed > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT(""));
        UE_LOG(LogTemp, Warning, TEXT("Errors:"));
        for (const FString& ErrorMessage : Result.ErrorMessages)
        {
            UE_LOG(LogTemp, Warning, TEXT("  %s"), *ErrorMessage);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT(""));
}

void UN2CExporterCommandlet::OnProgressUpdate(int32 Current, int32 Total)
{
    CurrentProgress = Current;
    TotalProgress = Total;
    
    if (bVerboseOutput)
    {
        float Percentage = Total > 0 ? (float)Current / (float)Total * 100.0f : 0.0f;
        UE_LOG(LogTemp, Log, TEXT("Progress: %d/%d (%.1f%%)"), Current, Total, Percentage);
    }
}

void UN2CExporterCommandlet::OnBlueprintProcessed(const FString& BlueprintName)
{
    if (bVerboseOutput)
    {
        UE_LOG(LogTemp, Log, TEXT("Exported: %s"), *BlueprintName);
    }
}

void UN2CExporterCommandlet::OnError(const FString& ErrorMessage)
{
    UE_LOG(LogTemp, Error, TEXT("Export Error: %s"), *ErrorMessage);
}

bool UN2CExporterCommandlet::ValidateOptions(const FN2CCommandLineOptions& Options, FString& OutErrorMessage)
{
    // Validate mode
    if (Options.Mode == FN2CCommandLineOptions::EExportMode::Unknown)
    {
        OutErrorMessage = TEXT("No export mode specified");
        return false;
    }

    // Validate path mode
    if (Options.Mode == FN2CCommandLineOptions::EExportMode::ExportPath && Options.Path.IsEmpty())
    {
        OutErrorMessage = TEXT("Path mode requires a valid path");
        return false;
    }

    // Validate blueprint mode
    if (Options.Mode == FN2CCommandLineOptions::EExportMode::ExportBlueprint && Options.BlueprintPath.IsEmpty())
    {
        OutErrorMessage = TEXT("Blueprint mode requires a valid blueprint path");
        return false;
    }

    // Validate numeric parameters
    if (Options.GraphCountThreshold < 1)
    {
        OutErrorMessage = TEXT("Graph count threshold must be >= 1");
        return false;
    }

    if (Options.MaxConcurrentProcessing < 1 || Options.MaxConcurrentProcessing > 16)
    {
        OutErrorMessage = TEXT("Max concurrent processing must be between 1 and 16");
        return false;
    }

    if (Options.MaxNodesPerBlueprint < 0)
    {
        OutErrorMessage = TEXT("Max nodes per blueprint must be >= 0");
        return false;
    }

    // Validate output directory
    if (!Options.OutputDirectory.IsEmpty())
    {
        // Check if path is valid (basic validation)
        if (Options.OutputDirectory.Contains(TEXT("*")) || Options.OutputDirectory.Contains(TEXT("?")))
        {
            OutErrorMessage = TEXT("Output directory contains invalid characters");
            return false;
        }
    }

    return true;
}

bool UN2CExporterCommandlet::InitializeTranslatorSystem()
{
    // Initialize the translator manager
    FN2CTranslatorManager& Manager = FN2CTranslatorManager::Get();
    if (!Manager.IsInitialized())
    {
        Manager.Initialize();
    }

    // Wait for asset registry to scan assets
    FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    if (!AssetRegistry.Get().IsLoadingAssets())
    {
        // Asset registry is ready
        return true;
    }

    if (bVerboseOutput)
    {
        UE_LOG(LogTemp, Log, TEXT("Waiting for asset registry to complete scanning..."));
    }

    // Wait for asset registry
    const double StartTime = FPlatformTime::Seconds();
    const double TimeoutSeconds = 60.0; // 1 minute timeout
    
    while (AssetRegistry.Get().IsLoadingAssets())
    {
        FPlatformProcess::Sleep(0.1f);
        
        if (FPlatformTime::Seconds() - StartTime > TimeoutSeconds)
        {
            UE_LOG(LogTemp, Error, TEXT("Timeout waiting for asset registry"));
            return false;
        }
    }

    if (bVerboseOutput)
    {
        UE_LOG(LogTemp, Log, TEXT("Asset registry scan completed"));
    }

    return true;
}

void UN2CExporterCommandlet::CleanupTranslatorSystem()
{
    // The translator manager will cleanup automatically when the module shuts down
    // No explicit cleanup needed for commandlet
}

#undef LOCTEXT_NAMESPACE