// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#include "Core/N2CTranslatorSettings.h"
#include "Utils/N2CLogger.h"

UN2CTranslatorSettings::UN2CTranslatorSettings()
{
    CategoryName = TEXT("Plugins");
}

FName UN2CTranslatorSettings::GetCategoryName() const
{
    return TEXT("Plugins");
}

FText UN2CTranslatorSettings::GetSectionText() const
{
    return NSLOCTEXT("NodeToCode", "TranslatorSettingsSection", "Node to Code Translator");
}

bool UN2CTranslatorSettings::ShouldIncludeBlueprint(const FString& BlueprintPath) const
{
    // Check include directories first
    bool bIncluded = false;
    for (const FString& IncludeDir : IncludeDirectories)
    {
        if (BlueprintPath.StartsWith(IncludeDir))
        {
            bIncluded = true;
            break;
        }
    }
    
    if (!bIncluded)
    {
        return false;
    }
    
    // Check exclude directories
    for (const FString& ExcludeDir : ExcludeDirectories)
    {
        if (BlueprintPath.StartsWith(ExcludeDir))
        {
            return false;
        }
    }
    
    return true;
}

EN2CFileStrategy UN2CTranslatorSettings::GetEffectiveFileStrategy(int32 GraphCount) const
{
    switch (FileStrategy)
    {
        case EN2CFileStrategy::Hybrid:
            return GraphCount > GraphCountThreshold ? 
                EN2CFileStrategy::MultipleFilesPerGraph : 
                EN2CFileStrategy::SingleFilePerBlueprint;
                
        case EN2CFileStrategy::Custom:
            // For now, fall back to single file for custom strategy
            // This can be extended with plugin system later
            return EN2CFileStrategy::SingleFilePerBlueprint;
            
        default:
            return FileStrategy;
    }
}

FString UN2CTranslatorSettings::GetFullExportPath() const
{
    const FString ProjectDir = FPaths::ProjectDir();
    return FPaths::Combine(ProjectDir, ExportDirectory);
}

TArray<FString> UN2CTranslatorSettings::ValidateSettings() const
{
    TArray<FString> Errors;
    
    // Validate export directory
    if (ExportDirectory.IsEmpty())
    {
        Errors.Add(TEXT("Export directory cannot be empty"));
    }
    else if (ExportDirectory.Contains(TEXT("..")))
    {
        Errors.Add(TEXT("Export directory cannot contain relative path components (..)"));
    }
    
    // Validate include directories
    if (IncludeDirectories.Num() == 0)
    {
        Errors.Add(TEXT("At least one include directory must be specified"));
    }
    
    for (const FString& IncludeDir : IncludeDirectories)
    {
        if (IncludeDir.IsEmpty())
        {
            Errors.Add(TEXT("Include directories cannot be empty"));
            break;
        }
    }
    
    // Validate performance settings
    if (MaxConcurrentProcessing < 1 || MaxConcurrentProcessing > 16)
    {
        Errors.Add(TEXT("Max concurrent processing must be between 1 and 16"));
    }
    
    if (MaxNodesPerBlueprint < 0)
    {
        Errors.Add(TEXT("Max nodes per Blueprint cannot be negative"));
    }
    
    // Validate graph threshold for hybrid strategy
    if (FileStrategy == EN2CFileStrategy::Hybrid && GraphCountThreshold < 1)
    {
        Errors.Add(TEXT("Graph count threshold must be at least 1 for hybrid strategy"));
    }
    
    // Log validation results
    if (Errors.Num() > 0)
    {
        FN2CLogger::Get().LogWarning(FString::Printf(TEXT("Translator settings validation found %d errors"), Errors.Num()));
        for (const FString& Error : Errors)
        {
            FN2CLogger::Get().LogWarning(Error);
        }
    }
    
    return Errors;
}

bool UN2CTranslatorSettings::PathMatchesAnyPattern(const FString& Path, const TArray<FString>& Patterns) const
{
    for (const FString& Pattern : Patterns)
    {
        if (Path.StartsWith(Pattern))
        {
            return true;
        }
    }
    return false;
}