// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#include "Core/N2CBlueprintDiscovery.h"
#include "Utils/N2CLogger.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Misc/DateTime.h"

FN2CDiscoveryResult FN2CBlueprintDiscovery::DiscoverAllBlueprints(const UN2CTranslatorSettings* Settings)
{
    FN2CDiscoveryResult Result;
    
    if (!Settings)
    {
        Result.ErrorMessages.Add(TEXT("Invalid settings provided"));
        return Result;
    }
    
    const double StartTime = FPlatformTime::Seconds();
    
    // Get Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Find all Blueprint assets
    TArray<FAssetData> BlueprintAssets;
    AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets);
    
    Result.TotalFound = BlueprintAssets.Num();
    
    FN2CLogger::Get().Log(
        FString::Printf(TEXT("Found %d Blueprint assets in project"), Result.TotalFound),
        EN2CLogSeverity::Info
    );
    
    // Process each Blueprint asset
    for (const FAssetData& AssetData : BlueprintAssets)
    {
        const FString AssetPath = AssetData.PackageName.ToString();
        
        // Apply include/exclude filters
        if (!ShouldIncludeBlueprint(AssetPath, Settings))
        {
            Result.SkippedPaths.Add(AssetPath);
            FN2CLogger::Get().Log(
                FString::Printf(TEXT("Skipped Blueprint (filtered): %s"), *AssetPath),
                EN2CLogSeverity::Debug
            );
            continue;
        }
        
        // Load and validate Blueprint
        FString LoadErrorMessage;
        UBlueprint* Blueprint = LoadBlueprintSafely(AssetData, &LoadErrorMessage);
        
        if (!Blueprint)
        {
            const FString ErrorMsg = FString::Printf(TEXT("Failed to load Blueprint %s: %s"), *AssetPath, *LoadErrorMessage);
            Result.ErrorMessages.Add(ErrorMsg);
            FN2CLogger::Get().LogWarning(ErrorMsg);
            continue;
        }
        
        // Validate Blueprint for translation
        FString ValidationErrorMessage;
        if (!ValidateBlueprintForTranslation(Blueprint, &ValidationErrorMessage))
        {
            const FString ErrorMsg = FString::Printf(TEXT("Blueprint validation failed for %s: %s"), *AssetPath, *ValidationErrorMessage);
            Result.ErrorMessages.Add(ErrorMsg);
            FN2CLogger::Get().LogWarning(ErrorMsg);
            continue;
        }
        
        // Check complexity limits
        if (Settings->MaxNodesPerBlueprint > 0)
        {
            const int32 NodeCount = EstimateBlueprintComplexity(Blueprint);
            if (NodeCount > Settings->MaxNodesPerBlueprint)
            {
                const FString ErrorMsg = FString::Printf(TEXT("Blueprint %s exceeds node limit (%d > %d)"), 
                    *AssetPath, NodeCount, Settings->MaxNodesPerBlueprint);
                Result.ErrorMessages.Add(ErrorMsg);
                FN2CLogger::Get().LogWarning(ErrorMsg);
                continue;
            }
        }
        
        // Blueprint passed all checks
        Result.ValidBlueprints.Add(Blueprint);
        Result.TotalProcessed++;
        
        FN2CLogger::Get().Log(
            FString::Printf(TEXT("Validated Blueprint: %s"), *AssetPath),
            EN2CLogSeverity::Debug
        );
    }
    
    Result.DiscoveryTime = FPlatformTime::Seconds() - StartTime;
    
    FN2CLogger::Get().Log(
        FString::Printf(TEXT("Blueprint discovery completed: %d valid, %d skipped, %d errors (%.2f seconds)"),
        Result.TotalProcessed, Result.SkippedPaths.Num(), Result.ErrorMessages.Num(), Result.DiscoveryTime),
        EN2CLogSeverity::Info
    );
    
    return Result;
}

FN2CDiscoveryResult FN2CBlueprintDiscovery::DiscoverBlueprintsInDirectory(const FString& DirectoryPath, const UN2CTranslatorSettings* Settings)
{
    FN2CDiscoveryResult Result;
    
    if (!Settings)
    {
        Result.ErrorMessages.Add(TEXT("Invalid settings provided"));
        return Result;
    }
    
    const double StartTime = FPlatformTime::Seconds();
    
    // Get Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Find Blueprint assets in the specified directory
    TArray<FAssetData> BlueprintAssets;
    AssetRegistry.GetAssetsByPath(FName(*DirectoryPath), BlueprintAssets, true); // true = recursive
    
    // Filter to only Blueprint assets
    BlueprintAssets.RemoveAll([](const FAssetData& AssetData)
    {
        return AssetData.AssetClassPath != UBlueprint::StaticClass()->GetClassPathName();
    });
    
    Result.TotalFound = BlueprintAssets.Num();
    
    FN2CLogger::Get().Log(
        FString::Printf(TEXT("Found %d Blueprint assets in directory: %s"), Result.TotalFound, *DirectoryPath),
        EN2CLogSeverity::Info
    );
    
    // Process each Blueprint (similar to DiscoverAllBlueprints but with directory filter)
    for (const FAssetData& AssetData : BlueprintAssets)
    {
        const FString AssetPath = AssetData.PackageName.ToString();
        
        // Apply include/exclude filters
        if (!ShouldIncludeBlueprint(AssetPath, Settings))
        {
            Result.SkippedPaths.Add(AssetPath);
            continue;
        }
        
        // Load and validate Blueprint
        FString LoadErrorMessage;
        UBlueprint* Blueprint = LoadBlueprintSafely(AssetData, &LoadErrorMessage);
        
        if (!Blueprint)
        {
            Result.ErrorMessages.Add(FString::Printf(TEXT("Failed to load Blueprint %s: %s"), *AssetPath, *LoadErrorMessage));
            continue;
        }
        
        FString ValidationErrorMessage;
        if (!ValidateBlueprintForTranslation(Blueprint, &ValidationErrorMessage))
        {
            Result.ErrorMessages.Add(FString::Printf(TEXT("Blueprint validation failed for %s: %s"), *AssetPath, *ValidationErrorMessage));
            continue;
        }
        
        Result.ValidBlueprints.Add(Blueprint);
        Result.TotalProcessed++;
    }
    
    Result.DiscoveryTime = FPlatformTime::Seconds() - StartTime;
    
    return Result;
}

bool FN2CBlueprintDiscovery::ValidateBlueprintForTranslation(const UBlueprint* Blueprint, FString* OutErrorMessage)
{
    if (!Blueprint)
    {
        if (OutErrorMessage)
        {
            *OutErrorMessage = TEXT("Blueprint is null");
        }
        return false;
    }
    
    // Check if Blueprint has a generated class
    if (!Blueprint->GeneratedClass)
    {
        if (OutErrorMessage)
        {
            *OutErrorMessage = TEXT("Blueprint has no generated class");
        }
        return false;
    }
    
    // Skip abstract classes
    if (Blueprint->GeneratedClass->HasAnyClassFlags(CLASS_Abstract))
    {
        if (OutErrorMessage)
        {
            *OutErrorMessage = TEXT("Blueprint is abstract");
        }
        return false;
    }
    
    // Check if Blueprint is supported type
    if (!IsSupportedBlueprintClass(Blueprint))
    {
        if (OutErrorMessage)
        {
            *OutErrorMessage = TEXT("Unsupported Blueprint class type");
        }
        return false;
    }
    
    // Check for graphs
    const bool bHasGraphs = Blueprint->UbergraphPages.Num() > 0 || 
                           Blueprint->FunctionGraphs.Num() > 0 || 
                           Blueprint->MacroGraphs.Num() > 0;
    
    if (!bHasGraphs)
    {
        if (OutErrorMessage)
        {
            *OutErrorMessage = TEXT("Blueprint has no graphs to translate");
        }
        return false;
    }
    
    // Check if Blueprint is compiled
    if (Blueprint->Status == BS_Error)
    {
        if (OutErrorMessage)
        {
            *OutErrorMessage = TEXT("Blueprint has compilation errors");
        }
        return false;
    }
    
    return true;
}

bool FN2CBlueprintDiscovery::ShouldIncludeBlueprint(const FString& AssetPath, const UN2CTranslatorSettings* Settings)
{
    if (!Settings)
    {
        return false;
    }
    
    // Check if it's user-created content
    if (!IsUserCreatedBlueprint(AssetPath))
    {
        return false;
    }
    
    // Apply the same logic as the settings class
    return Settings->ShouldIncludeBlueprint(AssetPath);
}

bool FN2CBlueprintDiscovery::GetBlueprintMetadata(const FString& AssetPath, FString& OutBlueprintType, FString& OutBlueprintClass)
{
    // Get Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    // Find the asset
    FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FName(*AssetPath));
    if (!AssetData.IsValid())
    {
        return false;
    }
    
    // Extract metadata from asset registry tags
    FString BlueprintType;
    if (AssetData.GetTagValue(FBlueprintTags::BlueprintType, BlueprintType))
    {
        OutBlueprintType = BlueprintType;
    }
    
    FString ParentClass;
    if (AssetData.GetTagValue(FBlueprintTags::ParentClassPath, ParentClass))
    {
        OutBlueprintClass = ParentClass;
    }
    
    return !OutBlueprintType.IsEmpty() || !OutBlueprintClass.IsEmpty();
}

int32 FN2CBlueprintDiscovery::EstimateBlueprintComplexity(const UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return 0;
    }
    
    return CountBlueprintNodes(Blueprint);
}

bool FN2CBlueprintDiscovery::IsBlueprintModifiedSince(const UBlueprint* Blueprint, const FDateTime& CompareTime)
{
    if (!Blueprint)
    {
        return false;
    }
    
    // Get the package file path
    const FString PackageFilePath = FPackageName::LongPackageNameToFilename(
        Blueprint->GetOutermost()->GetName(), 
        FPackageName::GetAssetPackageExtension()
    );
    
    // Get file modification time
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    const FDateTime FileTime = PlatformFile.GetTimeStamp(*PackageFilePath);
    
    return FileTime > CompareTime;
}

UBlueprint* FN2CBlueprintDiscovery::LoadBlueprintSafely(const FAssetData& AssetData, FString* OutErrorMessage)
{
    try
    {
        UObject* Asset = AssetData.GetAsset();
        if (!Asset)
        {
            if (OutErrorMessage)
            {
                *OutErrorMessage = TEXT("Failed to load asset");
            }
            return nullptr;
        }
        
        UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
        if (!Blueprint)
        {
            if (OutErrorMessage)
            {
                *OutErrorMessage = TEXT("Asset is not a Blueprint");
            }
            return nullptr;
        }
        
        return Blueprint;
    }
    catch (const std::exception& e)
    {
        if (OutErrorMessage)
        {
            *OutErrorMessage = FString::Printf(TEXT("Exception during load: %s"), ANSI_TO_TCHAR(e.what()));
        }
        return nullptr;
    }
    catch (...)
    {
        if (OutErrorMessage)
        {
            *OutErrorMessage = TEXT("Unknown exception during load");
        }
        return nullptr;
    }
}

bool FN2CBlueprintDiscovery::IsUserCreatedBlueprint(const FString& AssetPath)
{
    // User content is typically in /Game/ paths
    if (AssetPath.StartsWith(TEXT("/Game/")))
    {
        return true;
    }
    
    // Also check for /Content/ paths (alternative content root)
    if (AssetPath.Contains(TEXT("/Content/")))
    {
        return true;
    }
    
    // Exclude engine and plugin content
    if (AssetPath.StartsWith(TEXT("/Engine/")) ||
        AssetPath.StartsWith(TEXT("/Script/")) ||
        AssetPath.StartsWith(TEXT("/Temp/")))
    {
        return false;
    }
    
    return false;
}

int32 FN2CBlueprintDiscovery::CountBlueprintNodes(const UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return 0;
    }
    
    int32 TotalNodes = 0;
    
    // Count nodes in all graph types
    auto CountNodesInGraphs = [&TotalNodes](const TArray<UEdGraph*>& Graphs)
    {
        for (const UEdGraph* Graph : Graphs)
        {
            if (Graph)
            {
                TotalNodes += Graph->Nodes.Num();
            }
        }
    };
    
    CountNodesInGraphs(Blueprint->UbergraphPages);
    CountNodesInGraphs(Blueprint->FunctionGraphs);
    CountNodesInGraphs(Blueprint->MacroGraphs);
    
    return TotalNodes;
}

bool FN2CBlueprintDiscovery::IsSupportedBlueprintClass(const UBlueprint* Blueprint)
{
    if (!Blueprint || !Blueprint->GeneratedClass)
    {
        return false;
    }
    
    // For now, support most Blueprint types
    // This can be expanded to filter specific unsupported types
    
    // Skip interface Blueprints (they have no implementation)
    if (Blueprint->BlueprintType == BPTYPE_Interface)
    {
        return false;
    }
    
    // Skip const Blueprints if they have no meaningful logic
    if (Blueprint->BlueprintType == BPTYPE_Const && 
        Blueprint->FunctionGraphs.Num() == 0 && 
        Blueprint->UbergraphPages.Num() == 0)
    {
        return false;
    }
    
    return true;
}