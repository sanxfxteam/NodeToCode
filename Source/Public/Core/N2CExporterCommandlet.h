// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "Commandlets/Commandlet.h"
#include "N2CTranslatorTypes.h"
#include "N2CTranslatorSettings.h"
#include "N2CExporterCommandlet.generated.h"

/**
 * @class UN2CExporterCommandlet
 * @brief Command line interface for NodeToCode Blueprint JSON export
 *
 * This commandlet provides command line access to the NodeToCode translation
 * functionality for CI/CD integration and automated workflows.
 *
 * Usage examples:
 * UnrealEditor.exe MyProject.uproject -run=N2CExporter -ExportAll
 * UnrealEditor.exe MyProject.uproject -run=N2CExporter -Path="/Game/Characters"
 * UnrealEditor.exe MyProject.uproject -run=N2CExporter -ExportAll -Strategy=MultipleFiles -OutputDir="Build/Translated"
 */
UCLASS()
class NODETOCODE_API UN2CExporterCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UN2CExporterCommandlet();

    // UCommandlet interface
    virtual int32 Main(const FString& Params) override;

private:
    /** Parse command line parameters */
    bool ParseCommandLineParams(const FString& Params, struct FN2CCommandLineOptions& OutOptions);

    /** Execute export all blueprints command */
    int32 ExecuteExportAll(const struct FN2CCommandLineOptions& Options);

    /** Execute export specific path command */
    int32 ExecuteExportPath(const struct FN2CCommandLineOptions& Options);

    /** Execute export single blueprint command */
    int32 ExecuteExportBlueprint(const struct FN2CCommandLineOptions& Options);

    /** Create settings from command line options */
    UN2CTranslatorSettings* CreateSettingsFromOptions(const struct FN2CCommandLineOptions& Options);

    /** Print usage information */
    void PrintUsage();

    /** Print export results */
    void PrintResults(const FN2CBatchResult& Result);

    /** Handle progress updates for command line output */
    void OnProgressUpdate(int32 Current, int32 Total);

    /** Handle blueprint processed events */
    void OnBlueprintProcessed(const FString& BlueprintName);

    /** Handle error events */
    void OnError(const FString& ErrorMessage);

    /** Validate command line options */
    bool ValidateOptions(const struct FN2CCommandLineOptions& Options, FString& OutErrorMessage);

    /** Initialize the translator system for commandlet use */
    bool InitializeTranslatorSystem();

    /** Cleanup translator system */
    void CleanupTranslatorSystem();

    /** Current progress tracking */
    int32 CurrentProgress = 0;
    int32 TotalProgress = 0;
    bool bVerboseOutput = false;
    bool bQuietMode = false;
};

/**
 * @struct FN2CCommandLineOptions
 * @brief Parsed command line options for the exporter commandlet
 */
USTRUCT()
struct FN2CCommandLineOptions
{
    GENERATED_BODY()

    /** Export mode */
    enum class EExportMode
    {
        Unknown,
        ExportAll,
        ExportPath,
        ExportBlueprint,
        Help
    };

    /** The export mode to execute */
    EExportMode Mode = EExportMode::Unknown;

    /** Specific path to export (for ExportPath mode) */
    FString Path;

    /** Specific blueprint to export (for ExportBlueprint mode) */
    FString BlueprintPath;

    /** Custom output directory */
    FString OutputDirectory;

    /** File organization strategy */
    EN2CFileStrategy FileStrategy = EN2CFileStrategy::SingleFilePerBlueprint;

    /** Graph count threshold for hybrid strategy */
    int32 GraphCountThreshold = 3;

    /** Include directories filter */
    TArray<FString> IncludeDirectories;

    /** Exclude directories filter */
    TArray<FString> ExcludeDirectories;

    /** Overwrite existing files */
    bool bOverwriteExisting = true;

    /** Create backup files */
    bool bCreateBackups = false;

    /** Use pretty-printed JSON */
    bool bPrettyPrintJson = true;

    /** Maximum concurrent processing */
    int32 MaxConcurrentProcessing = 4;

    /** Maximum nodes per blueprint */
    int32 MaxNodesPerBlueprint = 1000;

    /** Use incremental export */
    bool bIncrementalExport = false;

    /** Verbose output */
    bool bVerbose = false;

    /** Quiet mode (minimal output) */
    bool bQuiet = false;

    /** Exit code on any failures */
    bool bFailOnError = false;

    /** Show help */
    bool bShowHelp = false;

    FN2CCommandLineOptions()
    {
        IncludeDirectories.Add(TEXT("/Game/"));
    }
};