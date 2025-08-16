// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "N2CTranslatorSettings.h"
#include "N2CBlueprintDiscovery.generated.h"

/**
 * @struct FN2CDiscoveryResult
 * @brief Result structure for Blueprint discovery operations
 */
USTRUCT(BlueprintType)
struct FN2CDiscoveryResult
{
    GENERATED_BODY()

    /** Blueprints that passed validation and can be exported */
    UPROPERTY(BlueprintReadOnly, Category="Discovery Result")
    TArray<UBlueprint*> ValidBlueprints;

    /** Asset paths that were skipped due to filters */
    UPROPERTY(BlueprintReadOnly, Category="Discovery Result")
    TArray<FString> SkippedPaths;

    /** Error messages from failed validations */
    UPROPERTY(BlueprintReadOnly, Category="Discovery Result")
    TArray<FString> ErrorMessages;

    /** Total number of Blueprint assets found */
    UPROPERTY(BlueprintReadOnly, Category="Discovery Result")
    int32 TotalFound = 0;

    /** Number that passed validation */
    UPROPERTY(BlueprintReadOnly, Category="Discovery Result")
    int32 TotalProcessed = 0;

    /** Discovery time in seconds */
    UPROPERTY(BlueprintReadOnly, Category="Discovery Result")
    float DiscoveryTime = 0.0f;

    FN2CDiscoveryResult() = default;
};

/**
 * @class FN2CBlueprintDiscovery
 * @brief Handles discovery and validation of Blueprint assets for translation
 *
 * This class provides functionality to scan the project for Blueprint assets,
 * apply filtering rules, and validate them for translation compatibility.
 */
class NODETOCODE_API FN2CBlueprintDiscovery
{
public:
    /** Discover all Blueprints in the project according to settings */
    static FN2CDiscoveryResult DiscoverAllBlueprints(const UN2CTranslatorSettings* Settings);

    /** Discover Blueprints in a specific directory */
    static FN2CDiscoveryResult DiscoverBlueprintsInDirectory(const FString& DirectoryPath, const UN2CTranslatorSettings* Settings);

    /** Validate a single Blueprint for translation compatibility */
    static bool ValidateBlueprintForTranslation(const UBlueprint* Blueprint, FString* OutErrorMessage = nullptr);

    /** Check if a Blueprint should be included based on filter settings */
    static bool ShouldIncludeBlueprint(const FString& AssetPath, const UN2CTranslatorSettings* Settings);

    /** Get metadata about a Blueprint without loading it */
    static bool GetBlueprintMetadata(const FString& AssetPath, FString& OutBlueprintType, FString& OutBlueprintClass);

    /** Estimate the complexity of a Blueprint (node count approximation) */
    static int32 EstimateBlueprintComplexity(const UBlueprint* Blueprint);

    /** Check if a Blueprint has been modified since a given time */
    static bool IsBlueprintModifiedSince(const UBlueprint* Blueprint, const FDateTime& CompareTime);

private:
    /** Load Blueprint from asset data safely */
    static UBlueprint* LoadBlueprintSafely(const struct FAssetData& AssetData, FString* OutErrorMessage = nullptr);

    /** Check if Blueprint is a user-created Blueprint (not engine/plugin) */
    static bool IsUserCreatedBlueprint(const FString& AssetPath);

    /** Count total nodes in a Blueprint approximately */
    static int32 CountBlueprintNodes(const UBlueprint* Blueprint);

    /** Check if Blueprint class is supported for translation */
    static bool IsSupportedBlueprintClass(const UBlueprint* Blueprint);
};