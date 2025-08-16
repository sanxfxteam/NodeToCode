// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "AssetRegistry/AssetData.h"
#include "N2CTranslatorTypes.h"
#include "N2CTranslatorSettings.h"
#include "N2CBatchProcessor.h"

/**
 * @class FN2CTranslatorManager
 * @brief Main interface for automatic Blueprint JSON translation functionality
 *
 * This singleton class provides the primary interface for all automatic translation
 * operations, including batch processing, settings management, and event handling.
 */
class NODETOCODE_API FN2CTranslatorManager
{
public:
    /** Get singleton instance */
    static FN2CTranslatorManager& Get();

    /** Initialize the translator system */
    void Initialize();

    /** Shutdown and cleanup */
    void Shutdown();

    /** Check if the translator system is initialized */
    bool IsInitialized() const { return bIsInitialized; }

    /** Export all Blueprints in the project */
    FN2CBatchResult ExportAllBlueprints(const UN2CTranslatorSettings* Settings = nullptr);

    /** Export a single Blueprint */
    FN2CExportResult ExportBlueprint(const UBlueprint* Blueprint, const UN2CTranslatorSettings* Settings = nullptr);

    /** Export Blueprints in a specific directory */
    FN2CBatchResult ExportBlueprintsInDirectory(const FString& DirectoryPath, const UN2CTranslatorSettings* Settings = nullptr);

    /** Get current translator settings */
    const UN2CTranslatorSettings* GetSettings() const;

    /** Update translator settings */
    void UpdateSettings(const UN2CTranslatorSettings* NewSettings);

    /** Check if batch processing is currently active */
    bool IsBatchProcessingActive() const;

    /** Cancel ongoing batch processing */
    void CancelBatchProcessing();

    /** Get progress of current batch operation (0.0 to 1.0) */
    float GetBatchProgress() const;

    /** Get current batch status message */
    FString GetBatchStatusMessage() const;

    /** Event delegates for batch processing */
    FOnN2CProgressUpdate OnProgressUpdate;
    FOnN2CBlueprintProcessed OnBlueprintProcessed;
    FOnN2CError OnError;
    FOnN2CBatchComplete OnBatchComplete;

    /** Utility functions */

    /** Validate a Blueprint for export compatibility */
    bool ValidateBlueprintForExport(const UBlueprint* Blueprint, FString* OutErrorMessage = nullptr) const;

    /** Get estimated export time for a list of Blueprints */
    float EstimateExportTime(const TArray<UBlueprint*>& Blueprints) const;

    /** Get export statistics */
    struct FExportStatistics
    {
        int32 TotalExported = 0;
        int32 TotalFailed = 0;
        float TotalTime = 0.0f;
        int64 TotalBytesWritten = 0;
        FDateTime LastExportTime;
    };
    
    FExportStatistics GetExportStatistics() const { return ExportStats; }

    /** Reset export statistics */
    void ResetExportStatistics();

private:
    /** Constructor (private for singleton) */
    FN2CTranslatorManager();

    /** Destructor */
    ~FN2CTranslatorManager();

    /** Get default settings if none provided */
    const UN2CTranslatorSettings* GetEffectiveSettings(const UN2CTranslatorSettings* ProvidedSettings) const;

    /** Handle batch completion */
    void HandleBatchComplete(const FN2CBatchResult& Result);

    /** Update export statistics */
    void UpdateExportStatistics(const FN2CBatchResult& Result);
    void UpdateExportStatistics(const FN2CExportResult& Result);

    /** Setup auto-export triggers based on settings */
    void SetupAutoExportTriggers();

    /** Cleanup auto-export triggers */
    void CleanupAutoExportTriggers();

    /** Handle asset saved event for auto-export */
    void OnAssetSaved(const FAssetData& AssetData);

    /** Static wrapper for editor startup delegate */
    static void StaticOnEditorStartup(double);
    
    /** Handle editor startup for auto-export */
    void OnEditorStartup();

    /** Private members */
    bool bIsInitialized = false;
    FExportStatistics ExportStats;
    
    /** Settings instance */
    UPROPERTY()
    const UN2CTranslatorSettings* CachedSettings = nullptr;

    /** Delegates for auto-export triggers */
    FDelegateHandle AssetSavedHandle;
    FDelegateHandle EditorStartupHandle;

    /** Prevent copy construction and assignment */
    FN2CTranslatorManager(const FN2CTranslatorManager&) = delete;
    FN2CTranslatorManager& operator=(const FN2CTranslatorManager&) = delete;
};