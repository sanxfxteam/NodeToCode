// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "N2CTranslatorTypes.h"
#include "N2CTranslatorSettings.h"
#include "N2CBlueprintDiscovery.h"

// Delegate declarations for batch processing events
DECLARE_DELEGATE_TwoParams(FOnN2CProgressUpdate, int32 /*Current*/, int32 /*Total*/);
DECLARE_DELEGATE_OneParam(FOnN2CBlueprintProcessed, const FString& /*BlueprintName*/);
DECLARE_DELEGATE_OneParam(FOnN2CError, const FString& /*ErrorMessage*/);
DECLARE_DELEGATE_OneParam(FOnN2CBatchComplete, const FN2CBatchResult& /*Result*/);

/**
 * @class FN2CBatchProcessor
 * @brief Handles batch processing of multiple Blueprints for export
 *
 * This class manages the batch export process, including progress tracking,
 * error handling, and parallel processing capabilities.
 */
class NODETOCODE_API FN2CBatchProcessor
{
public:
    /** Process all Blueprints in the project */
    static FN2CBatchResult ProcessAllBlueprints(
        const UN2CTranslatorSettings* Settings,
        FOnN2CProgressUpdate ProgressDelegate = FOnN2CProgressUpdate(),
        FOnN2CBlueprintProcessed ProcessedDelegate = FOnN2CBlueprintProcessed(),
        FOnN2CError ErrorDelegate = FOnN2CError()
    );

    /** Process a specific list of Blueprints */
    static FN2CBatchResult ProcessBlueprintList(
        const TArray<UBlueprint*>& Blueprints,
        const UN2CTranslatorSettings* Settings,
        FOnN2CProgressUpdate ProgressDelegate = FOnN2CProgressUpdate(),
        FOnN2CBlueprintProcessed ProcessedDelegate = FOnN2CBlueprintProcessed(),
        FOnN2CError ErrorDelegate = FOnN2CError()
    );

    /** Process Blueprints in a specific directory */
    static FN2CBatchResult ProcessBlueprintsInDirectory(
        const FString& DirectoryPath,
        const UN2CTranslatorSettings* Settings,
        FOnN2CProgressUpdate ProgressDelegate = FOnN2CProgressUpdate(),
        FOnN2CBlueprintProcessed ProcessedDelegate = FOnN2CBlueprintProcessed(),
        FOnN2CError ErrorDelegate = FOnN2CError()
    );

    /** Check if batch processing is currently running */
    static bool IsBatchProcessingActive();

    /** Cancel ongoing batch processing */
    static void CancelBatchProcessing();

    /** Get progress of current batch operation (0.0 to 1.0) */
    static float GetBatchProgress();

    /** Get current batch status message */
    static FString GetBatchStatusMessage();

private:
    /** Internal batch processing implementation */
    static FN2CBatchResult ProcessBlueprintsInternal(
        const TArray<UBlueprint*>& Blueprints,
        const UN2CTranslatorSettings* Settings,
        FOnN2CProgressUpdate ProgressDelegate,
        FOnN2CBlueprintProcessed ProcessedDelegate,
        FOnN2CError ErrorDelegate
    );

    /** Process a single Blueprint with error handling */
    static FN2CExportResult ProcessSingleBlueprint(
        const UBlueprint* Blueprint,
        const UN2CTranslatorSettings* Settings
    );

    /** Update progress and notify delegates */
    static void UpdateProgress(
        int32 CurrentIndex,
        int32 TotalCount,
        const FString& CurrentBlueprintName,
        FOnN2CProgressUpdate ProgressDelegate
    );

    /** Handle processing error */
    static void HandleProcessingError(
        const UBlueprint* Blueprint,
        const FString& ErrorMessage,
        FOnN2CError ErrorDelegate,
        TArray<FString>& ErrorMessages
    );

    /** Validate batch processing prerequisites */
    static bool ValidateBatchPrerequisites(const UN2CTranslatorSettings* Settings, FString& OutErrorMessage);

    /** Estimate total processing time */
    static float EstimateBatchProcessingTime(const TArray<UBlueprint*>& Blueprints);

    /** Clean up temporary resources */
    static void CleanupBatchResources();

    /** Static members for tracking batch state */
    static bool bIsBatchProcessingActive;
    static bool bCancelRequested;
    static float CurrentBatchProgress;
    static FString CurrentBatchStatus;
    static int32 CurrentBatchIndex;
    static int32 CurrentBatchTotal;
};