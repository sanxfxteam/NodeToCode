// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "N2CTranslatorTypes.generated.h"

/**
 * @enum EN2CExportTrigger
 * @brief Defines when automatic Blueprint export should be triggered
 */
UENUM(BlueprintType)
enum class EN2CExportTrigger : uint8
{
    /** Manual trigger only */
    Manual          UMETA(DisplayName="Manual Only"),
    
    /** Export on editor startup */
    OnStartup       UMETA(DisplayName="On Editor Startup"),
    
    /** Export when Blueprint assets are saved */
    OnAssetSave     UMETA(DisplayName="On Blueprint Save"),
    
    /** Export on project build */
    OnBuild         UMETA(DisplayName="On Project Build"),
    
    /** Continuous monitoring */
    Watch           UMETA(DisplayName="Watch Mode")
};

/**
 * @enum EN2CFileStrategy
 * @brief Defines how exported JSON files should be organized
 */
UENUM(BlueprintType)
enum class EN2CFileStrategy : uint8
{
    /** One JSON file per Blueprint containing all graphs */
    SingleFilePerBlueprint      UMETA(DisplayName="Single File Per Blueprint"),
    
    /** Separate JSON file for each graph */
    MultipleFilesPerGraph       UMETA(DisplayName="Multiple Files Per Graph"),
    
    /** Hybrid based on graph count threshold */
    Hybrid                      UMETA(DisplayName="Hybrid Strategy"),
    
    /** User-defined custom logic */
    Custom                      UMETA(DisplayName="Custom")
};

/**
 * @struct FN2CExportResult
 * @brief Result structure for individual Blueprint export operations
 */
USTRUCT(BlueprintType)
struct FN2CExportResult
{
    GENERATED_BODY()

    /** Whether the export was successful */
    UPROPERTY(BlueprintReadOnly, Category="Export Result")
    bool bSuccess = false;

    /** Path to the exported file(s) */
    UPROPERTY(BlueprintReadOnly, Category="Export Result")
    FString OutputPath;

    /** Error message if export failed */
    UPROPERTY(BlueprintReadOnly, Category="Export Result")
    FString ErrorMessage;

    /** Number of bytes written */
    UPROPERTY(BlueprintReadOnly, Category="Export Result")
    int32 BytesWritten = 0;

    /** Processing time in seconds */
    UPROPERTY(BlueprintReadOnly, Category="Export Result")
    float ProcessingTime = 0.0f;

    FN2CExportResult() = default;
};

/**
 * @struct FN2CBatchResult
 * @brief Result structure for batch Blueprint export operations
 */
USTRUCT(BlueprintType)
struct FN2CBatchResult
{
    GENERATED_BODY()

    /** Whether the batch operation was successful */
    UPROPERTY(BlueprintReadOnly, Category="Batch Result")
    bool bSuccess = false;

    /** Total number of Blueprints processed */
    UPROPERTY(BlueprintReadOnly, Category="Batch Result")
    int32 TotalBlueprints = 0;

    /** Number of Blueprints processed successfully */
    UPROPERTY(BlueprintReadOnly, Category="Batch Result")
    int32 ProcessedSuccessfully = 0;

    /** Number of failed Blueprints */
    UPROPERTY(BlueprintReadOnly, Category="Batch Result")
    int32 Failed = 0;

    /** Error messages from failed exports */
    UPROPERTY(BlueprintReadOnly, Category="Batch Result")
    TArray<FString> ErrorMessages;

    /** Total processing time in seconds */
    UPROPERTY(BlueprintReadOnly, Category="Batch Result")
    float TotalProcessingTime = 0.0f;

    /** Total bytes written across all files */
    UPROPERTY(BlueprintReadOnly, Category="Batch Result")
    int32 TotalBytesWritten = 0;

    FN2CBatchResult() = default;
};