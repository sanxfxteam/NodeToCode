// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "N2CTranslatorTypes.h"
#include "N2CTranslatorSettings.generated.h"

/**
 * @class UN2CTranslatorSettings
 * @brief Configuration settings for automatic Blueprint JSON translation
 *
 * This class manages all configuration options for the automatic translation
 * feature including export triggers, file organization, and performance settings.
 */
UCLASS(config=EditorPerProjectUserSettings, meta=(DisplayName="Node to Code Translator"))
class NODETOCODE_API UN2CTranslatorSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UN2CTranslatorSettings();

    // UDeveloperSettings interface
    virtual FName GetCategoryName() const override;
    virtual FText GetSectionText() const override;

    /** Auto-export configuration */
    
    /** Enable automatic JSON export functionality */
    UPROPERTY(EditAnywhere, config, Category="Auto Export", meta=(ToolTip="Enable automatic JSON export functionality"))
    bool bAutoExportEnabled = false;

    /** When to trigger automatic export */
    UPROPERTY(EditAnywhere, config, Category="Auto Export", meta=(ToolTip="When to trigger automatic export"))
    EN2CExportTrigger ExportTrigger = EN2CExportTrigger::Manual;

    /** Show progress dialog during batch operations */
    UPROPERTY(EditAnywhere, config, Category="Auto Export", meta=(ToolTip="Show progress dialog during batch operations"))
    bool bShowProgressDialog = true;

    /** File organization */

    /** Base directory for exported JSON files */
    UPROPERTY(EditAnywhere, config, Category="File Organization", meta=(ToolTip="Base directory for exported JSON files"))
    FString ExportDirectory = TEXT("Translated");

    /** How to organize exported files */
    UPROPERTY(EditAnywhere, config, Category="File Organization", meta=(ToolTip="How to organize exported files"))
    EN2CFileStrategy FileStrategy = EN2CFileStrategy::SingleFilePerBlueprint;

    /** Graph count threshold for hybrid strategy */
    UPROPERTY(EditAnywhere, config, Category="File Organization", meta=(ToolTip="Graph count threshold for hybrid strategy", EditCondition="FileStrategy == EN2CFileStrategy::Hybrid"))
    int32 GraphCountThreshold = 3;

    /** Discovery filters */

    /** Directories to include in Blueprint discovery */
    UPROPERTY(EditAnywhere, config, Category="Discovery", meta=(ToolTip="Directories to include in Blueprint discovery"))
    TArray<FString> IncludeDirectories = {TEXT("/Game/")};

    /** Directories to exclude from Blueprint discovery */
    UPROPERTY(EditAnywhere, config, Category="Discovery", meta=(ToolTip="Directories to exclude from Blueprint discovery"))
    TArray<FString> ExcludeDirectories;

    /** File operations */

    /** Overwrite existing JSON files */
    UPROPERTY(EditAnywhere, config, Category="File Operations", meta=(ToolTip="Overwrite existing JSON files"))
    bool bOverwriteExisting = true;

    /** Create .bak files before overwriting */
    UPROPERTY(EditAnywhere, config, Category="File Operations", meta=(ToolTip="Create .bak files before overwriting"))
    bool bCreateBackups = false;

    /** Use pretty-printed JSON format */
    UPROPERTY(EditAnywhere, config, Category="File Operations", meta=(ToolTip="Use pretty-printed JSON format"))
    bool bPrettyPrintJson = true;

    /** Performance settings */

    /** Maximum number of Blueprints to process in parallel */
    UPROPERTY(EditAnywhere, config, Category="Performance", meta=(ToolTip="Maximum number of Blueprints to process in parallel", ClampMin=1, ClampMax=16))
    int32 MaxConcurrentProcessing = 4;

    /** Skip Blueprints larger than this size (in nodes) */
    UPROPERTY(EditAnywhere, config, Category="Performance", meta=(ToolTip="Skip Blueprints larger than this size (in nodes). 0 = no limit", ClampMin=0))
    int32 MaxNodesPerBlueprint = 1000;

    /** Use incremental export (only changed Blueprints) */
    UPROPERTY(EditAnywhere, config, Category="Performance", meta=(ToolTip="Only export Blueprints that have changed since last export"))
    bool bIncrementalExport = true;

    /** Utility functions */

    /** Check if a Blueprint path should be included based on filters */
    UFUNCTION(BlueprintCallable, Category="Translator Settings")
    bool ShouldIncludeBlueprint(const FString& BlueprintPath) const;

    /** Get the effective file strategy for a Blueprint with given graph count */
    UFUNCTION(BlueprintCallable, Category="Translator Settings")
    EN2CFileStrategy GetEffectiveFileStrategy(int32 GraphCount) const;

    /** Get the full export directory path */
    UFUNCTION(BlueprintCallable, Category="Translator Settings")
    FString GetFullExportPath() const;

    /** Validate settings and return any errors */
    UFUNCTION(BlueprintCallable, Category="Translator Settings")
    TArray<FString> ValidateSettings() const;

private:
    /** Internal helper to check if path matches any pattern in array */
    bool PathMatchesAnyPattern(const FString& Path, const TArray<FString>& Patterns) const;
};