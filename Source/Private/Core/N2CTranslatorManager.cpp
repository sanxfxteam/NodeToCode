// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#include "Core/N2CTranslatorManager.h"
#include "Core/N2CFileExporter.h"
#include "Core/N2CBlueprintDiscovery.h"
#include "Utils/N2CLogger.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Engine/ObjectLibrary.h"
#include "Async/Async.h"
#include "UObject/SavePackage.h"
#include "UObject/Package.h"
#include "UObject/ObjectSaveContext.h"

FN2CTranslatorManager& FN2CTranslatorManager::Get()
{
    static FN2CTranslatorManager Instance;
    return Instance;
}

FN2CTranslatorManager::FN2CTranslatorManager()
{
    // Constructor - singleton instance
}

FN2CTranslatorManager::~FN2CTranslatorManager()
{
    Shutdown();
}

void FN2CTranslatorManager::Initialize()
{
    if (bIsInitialized)
    {
        FN2CLogger::Get().LogWarning(TEXT("TranslatorManager already initialized"));
        return;
    }

    FN2CLogger::Get().Log(TEXT("Initializing NodeToCode Translator Manager"), EN2CLogSeverity::Info);

    // Get settings
    CachedSettings = GetDefault<UN2CTranslatorSettings>();
    if (!CachedSettings)
    {
        FN2CLogger::Get().LogError(TEXT("Failed to get TranslatorSettings"));
        return;
    }

    // Validate settings
    const TArray<FString> ValidationErrors = CachedSettings->ValidateSettings();
    if (ValidationErrors.Num() > 0)
    {
        FN2CLogger::Get().LogWarning(FString::Printf(TEXT("TranslatorSettings validation issues: %d"), ValidationErrors.Num()));
        for (const FString& Error : ValidationErrors)
        {
            FN2CLogger::Get().LogWarning(Error);
        }
    }

    // Setup event bindings
    OnBatchComplete.BindLambda([this](const FN2CBatchResult& Result) { HandleBatchComplete(Result); });

    // Setup auto-export triggers if enabled
    if (CachedSettings->bAutoExportEnabled)
    {
        SetupAutoExportTriggers();
    }

    // Reset statistics
    ResetExportStatistics();

    bIsInitialized = true;
    FN2CLogger::Get().Log(TEXT("TranslatorManager initialized successfully"), EN2CLogSeverity::Info);
}

void FN2CTranslatorManager::Shutdown()
{
    if (!bIsInitialized)
    {
        return;
    }

    FN2CLogger::Get().Log(TEXT("Shutting down TranslatorManager"), EN2CLogSeverity::Info);

    // Cancel any active batch processing
    if (IsBatchProcessingActive())
    {
        CancelBatchProcessing();
    }

    // Cleanup auto-export triggers
    CleanupAutoExportTriggers();

    // Clear event bindings
    OnBatchComplete.Unbind();

    bIsInitialized = false;
    CachedSettings = nullptr;

    FN2CLogger::Get().Log(TEXT("TranslatorManager shutdown complete"), EN2CLogSeverity::Info);
}

FN2CBatchResult FN2CTranslatorManager::ExportAllBlueprints(const UN2CTranslatorSettings* Settings)
{
    if (!bIsInitialized)
    {
        FN2CBatchResult Result;
        Result.ErrorMessages.Add(TEXT("TranslatorManager not initialized"));
        return Result;
    }

    const UN2CTranslatorSettings* EffectiveSettings = GetEffectiveSettings(Settings);
    
    FN2CLogger::Get().Log(TEXT("Starting export of all Blueprints"), EN2CLogSeverity::Info);

    FN2CBatchResult Result = FN2CBatchProcessor::ProcessAllBlueprints(
        EffectiveSettings,
        OnProgressUpdate,
        OnBlueprintProcessed,
        OnError
    );

    HandleBatchComplete(Result);
    return Result;
}

FN2CExportResult FN2CTranslatorManager::ExportBlueprint(const UBlueprint* Blueprint, const UN2CTranslatorSettings* Settings)
{
    if (!bIsInitialized)
    {
        FN2CExportResult Result;
        Result.ErrorMessage = TEXT("TranslatorManager not initialized");
        return Result;
    }

    if (!Blueprint)
    {
        FN2CExportResult Result;
        Result.ErrorMessage = TEXT("Blueprint cannot be null");
        return Result;
    }

    const UN2CTranslatorSettings* EffectiveSettings = GetEffectiveSettings(Settings);

    FN2CLogger::Get().Log(
        FString::Printf(TEXT("Exporting single Blueprint: %s"), *Blueprint->GetName()),
        EN2CLogSeverity::Info
    );

    FN2CExportResult Result = FN2CFileExporter::ExportBlueprintToFile(Blueprint, EffectiveSettings);
    
    UpdateExportStatistics(Result);
    return Result;
}

FN2CBatchResult FN2CTranslatorManager::ExportBlueprintsInDirectory(const FString& DirectoryPath, const UN2CTranslatorSettings* Settings)
{
    if (!bIsInitialized)
    {
        FN2CBatchResult Result;
        Result.ErrorMessages.Add(TEXT("TranslatorManager not initialized"));
        return Result;
    }

    const UN2CTranslatorSettings* EffectiveSettings = GetEffectiveSettings(Settings);

    FN2CLogger::Get().Log(
        FString::Printf(TEXT("Starting export of Blueprints in directory: %s"), *DirectoryPath),
        EN2CLogSeverity::Info
    );

    FN2CBatchResult Result = FN2CBatchProcessor::ProcessBlueprintsInDirectory(
        DirectoryPath,
        EffectiveSettings,
        OnProgressUpdate,
        OnBlueprintProcessed,
        OnError
    );

    HandleBatchComplete(Result);
    return Result;
}

const UN2CTranslatorSettings* FN2CTranslatorManager::GetSettings() const
{
    return CachedSettings;
}

void FN2CTranslatorManager::UpdateSettings(const UN2CTranslatorSettings* NewSettings)
{
    if (!NewSettings)
    {
        FN2CLogger::Get().LogWarning(TEXT("Cannot update TranslatorManager with null settings"));
        return;
    }

    // Validate new settings
    const TArray<FString> ValidationErrors = NewSettings->ValidateSettings();
    if (ValidationErrors.Num() > 0)
    {
        FN2CLogger::Get().LogWarning(TEXT("New settings have validation errors, keeping current settings"));
        return;
    }

    CachedSettings = NewSettings;

    // Update auto-export triggers if needed
    CleanupAutoExportTriggers();
    if (CachedSettings->bAutoExportEnabled)
    {
        SetupAutoExportTriggers();
    }

    FN2CLogger::Get().Log(TEXT("TranslatorManager settings updated"), EN2CLogSeverity::Info);
}

bool FN2CTranslatorManager::IsBatchProcessingActive() const
{
    return FN2CBatchProcessor::IsBatchProcessingActive();
}

void FN2CTranslatorManager::CancelBatchProcessing()
{
    if (IsBatchProcessingActive())
    {
        FN2CBatchProcessor::CancelBatchProcessing();
        FN2CLogger::Get().Log(TEXT("Batch processing cancellation requested"), EN2CLogSeverity::Info);
    }
}

float FN2CTranslatorManager::GetBatchProgress() const
{
    return FN2CBatchProcessor::GetBatchProgress();
}

FString FN2CTranslatorManager::GetBatchStatusMessage() const
{
    return FN2CBatchProcessor::GetBatchStatusMessage();
}

bool FN2CTranslatorManager::ValidateBlueprintForExport(const UBlueprint* Blueprint, FString* OutErrorMessage) const
{
    return FN2CBlueprintDiscovery::ValidateBlueprintForTranslation(Blueprint, OutErrorMessage);
}

float FN2CTranslatorManager::EstimateExportTime(const TArray<UBlueprint*>& Blueprints) const
{
    // Simple estimation: 0.5 seconds per Blueprint
    return Blueprints.Num() * 0.5f;
}

void FN2CTranslatorManager::ResetExportStatistics()
{
    ExportStats = FExportStatistics();
    FN2CLogger::Get().Log(TEXT("Export statistics reset"), EN2CLogSeverity::Debug);
}

const UN2CTranslatorSettings* FN2CTranslatorManager::GetEffectiveSettings(const UN2CTranslatorSettings* ProvidedSettings) const
{
    if (ProvidedSettings)
    {
        return ProvidedSettings;
    }
    
    if (CachedSettings)
    {
        return CachedSettings;
    }
    
    // Fallback to default settings
    return GetDefault<UN2CTranslatorSettings>();
}

void FN2CTranslatorManager::HandleBatchComplete(const FN2CBatchResult& Result)
{
    UpdateExportStatistics(Result);

    const FString StatusMessage = Result.bSuccess ? 
        FString::Printf(TEXT("Batch export completed successfully: %d Blueprints exported"), Result.ProcessedSuccessfully) :
        FString::Printf(TEXT("Batch export completed with errors: %d succeeded, %d failed"), Result.ProcessedSuccessfully, Result.Failed);

    FN2CLogger::Get().Log(StatusMessage, Result.bSuccess ? EN2CLogSeverity::Info : EN2CLogSeverity::Warning);
}

void FN2CTranslatorManager::UpdateExportStatistics(const FN2CBatchResult& Result)
{
    ExportStats.TotalExported += Result.ProcessedSuccessfully;
    ExportStats.TotalFailed += Result.Failed;
    ExportStats.TotalTime += Result.TotalProcessingTime;
    ExportStats.TotalBytesWritten += Result.TotalBytesWritten;
    ExportStats.LastExportTime = FDateTime::Now();
}

void FN2CTranslatorManager::UpdateExportStatistics(const FN2CExportResult& Result)
{
    if (Result.bSuccess)
    {
        ExportStats.TotalExported++;
        ExportStats.TotalBytesWritten += Result.BytesWritten;
    }
    else
    {
        ExportStats.TotalFailed++;
    }
    
    ExportStats.TotalTime += Result.ProcessingTime;
    ExportStats.LastExportTime = FDateTime::Now();
}

void FN2CTranslatorManager::SetupAutoExportTriggers()
{
    if (!CachedSettings)
    {
        return;
    }

    FN2CLogger::Get().Log(TEXT("Setting up auto-export triggers"), EN2CLogSeverity::Debug);

    // Always setup asset deletion monitoring for JSON cleanup
    if (FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry"))
    {
        AssetDeletedHandle = AssetRegistryModule->Get().OnAssetRemoved().AddRaw(this, &FN2CTranslatorManager::OnAssetDeleted);
        FN2CLogger::Get().Log(TEXT("Asset deletion monitoring enabled for JSON cleanup"), EN2CLogSeverity::Info);
    }

    switch (CachedSettings->ExportTrigger)
    {
        case EN2CExportTrigger::OnStartup:
            if (GEditor)
            {
                EditorStartupHandle = FEditorDelegates::OnEditorInitialized.AddStatic(&FN2CTranslatorManager::StaticOnEditorStartup);
            }
            break;

        case EN2CExportTrigger::OnAssetSave:
            if (GEditor)
            {
                // Use UPackage saved delegate to detect when Blueprints are saved
                AssetSavedHandle = UPackage::PackageSavedWithContextEvent.AddRaw(this, &FN2CTranslatorManager::OnPackageSaved);
                FN2CLogger::Get().Log(TEXT("Auto-export on Blueprint save enabled"), EN2CLogSeverity::Info);
            }
            break;

        case EN2CExportTrigger::Watch:
            // TODO: Implement file system watching
            FN2CLogger::Get().LogWarning(TEXT("Watch mode not yet implemented"));
            break;

        case EN2CExportTrigger::OnBuild:
            // TODO: Implement build system integration
            FN2CLogger::Get().LogWarning(TEXT("Build trigger not yet implemented"));
            break;

        default:
            break;
    }
}

void FN2CTranslatorManager::CleanupAutoExportTriggers()
{
    if (AssetSavedHandle.IsValid())
    {
        UPackage::PackageSavedWithContextEvent.Remove(AssetSavedHandle);
        AssetSavedHandle.Reset();
    }

    if (AssetDeletedHandle.IsValid())
    {
        if (FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry"))
        {
            AssetRegistryModule->Get().OnAssetRemoved().Remove(AssetDeletedHandle);
        }
        AssetDeletedHandle.Reset();
    }

    if (EditorStartupHandle.IsValid())
    {
        FEditorDelegates::OnEditorInitialized.Remove(EditorStartupHandle);
        EditorStartupHandle.Reset();
    }

    FN2CLogger::Get().Log(TEXT("Auto-export triggers cleaned up"), EN2CLogSeverity::Debug);
}

void FN2CTranslatorManager::OnPackageSaved(const FString& PackageFilename, UPackage* Package, FObjectPostSaveContext ObjectSaveContext)
{
    if (!CachedSettings || !Package)
    {
        return;
    }

    FN2CLogger::Get().Log(
        FString::Printf(TEXT("Package saved: %s"), *PackageFilename),
        EN2CLogSeverity::Debug
    );

    // Find Blueprint assets in the saved package
    TArray<UObject*> ObjectsInPackage;
    GetObjectsWithOuter(Package, ObjectsInPackage, false);

    for (UObject* Object : ObjectsInPackage)
    {
        UBlueprint* Blueprint = Cast<UBlueprint>(Object);
        if (!Blueprint)
        {
            continue;
        }

        // Check if this Blueprint should be exported
        const FString AssetPath = Blueprint->GetPathName();
        if (!CachedSettings->ShouldIncludeBlueprint(AssetPath))
        {
            FN2CLogger::Get().Log(
                FString::Printf(TEXT("Blueprint excluded by filters: %s"), *AssetPath),
                EN2CLogSeverity::Debug
            );
            continue;
        }

        FN2CLogger::Get().Log(
            FString::Printf(TEXT("Auto-export triggered for saved Blueprint: %s"), *Blueprint->GetName()),
            EN2CLogSeverity::Info
        );

        // Export the Blueprint in background (non-blocking)
        AsyncTask(ENamedThreads::GameThread, [this, Blueprint]()
        {
            ExportBlueprint(Blueprint, CachedSettings);
        });
    }
}

void FN2CTranslatorManager::OnAssetSaved(const FAssetData& AssetData)
{
    if (!CachedSettings)
    {
        return;
    }

    // Check if the asset is a Blueprint
    if (AssetData.AssetClassPath != UBlueprint::StaticClass()->GetClassPathName())
    {
        return;
    }

    // Check if this Blueprint should be exported
    const FString AssetPath = AssetData.PackageName.ToString();
    if (!CachedSettings->ShouldIncludeBlueprint(AssetPath))
    {
        return;
    }

    // Load the Blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
    if (!Blueprint)
    {
        return;
    }

    FN2CLogger::Get().Log(
        FString::Printf(TEXT("Auto-export triggered for saved Blueprint: %s"), *Blueprint->GetName()),
        EN2CLogSeverity::Info
    );

    // Export the Blueprint
    ExportBlueprint(Blueprint, CachedSettings);
}

void FN2CTranslatorManager::StaticOnEditorStartup(double)
{
    FN2CTranslatorManager::Get().OnEditorStartup();
}

void FN2CTranslatorManager::OnEditorStartup()
{
    if (!CachedSettings)
    {
        return;
    }

    FN2CLogger::Get().Log(TEXT("Auto-export triggered on editor startup"), EN2CLogSeverity::Info);

    // Export all Blueprints on startup
    ExportAllBlueprints(CachedSettings);
}

void FN2CTranslatorManager::OnAssetDeleted(const FAssetData& AssetData)
{
    if (!CachedSettings)
    {
        return;
    }

    // Check if the deleted asset is a Blueprint
    if (AssetData.AssetClassPath != UBlueprint::StaticClass()->GetClassPathName())
    {
        return;
    }

    const FString AssetPath = AssetData.PackageName.ToString();
    
    FN2CLogger::Get().Log(
        FString::Printf(TEXT("Blueprint deleted, cleaning up JSON files: %s"), *AssetPath),
        EN2CLogSeverity::Info
    );

    // Delete associated JSON files
    if (FN2CFileExporter::DeleteBlueprintJsonFiles(AssetPath, CachedSettings))
    {
        FN2CLogger::Get().Log(
            FString::Printf(TEXT("Successfully cleaned up JSON files for deleted Blueprint: %s"), *AssetPath),
            EN2CLogSeverity::Info
        );
    }
    else
    {
        FN2CLogger::Get().LogWarning(
            FString::Printf(TEXT("Failed to clean up some JSON files for deleted Blueprint: %s"), *AssetPath)
        );
    }
}