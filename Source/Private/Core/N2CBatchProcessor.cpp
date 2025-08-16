// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#include "Core/N2CBatchProcessor.h"
#include "Core/N2CFileExporter.h"
#include "Utils/N2CLogger.h"
#include "Misc/DateTime.h"

// Static member definitions
bool FN2CBatchProcessor::bIsBatchProcessingActive = false;
bool FN2CBatchProcessor::bCancelRequested = false;
float FN2CBatchProcessor::CurrentBatchProgress = 0.0f;
FString FN2CBatchProcessor::CurrentBatchStatus = TEXT("");
int32 FN2CBatchProcessor::CurrentBatchIndex = 0;
int32 FN2CBatchProcessor::CurrentBatchTotal = 0;

FN2CBatchResult FN2CBatchProcessor::ProcessAllBlueprints(
    const UN2CTranslatorSettings* Settings,
    FOnN2CProgressUpdate ProgressDelegate,
    FOnN2CBlueprintProcessed ProcessedDelegate,
    FOnN2CError ErrorDelegate)
{
    FN2CBatchResult Result;
    
    if (!Settings)
    {
        Result.ErrorMessages.Add(TEXT("Invalid settings provided"));
        if (ErrorDelegate.IsBound())
        {
            ErrorDelegate.Execute(TEXT("Invalid settings provided"));
        }
        return Result;
    }
    
    // Validate prerequisites
    FString ValidationError;
    if (!ValidateBatchPrerequisites(Settings, ValidationError))
    {
        Result.ErrorMessages.Add(ValidationError);
        if (ErrorDelegate.IsBound())
        {
            ErrorDelegate.Execute(ValidationError);
        }
        return Result;
    }
    
    // Discover Blueprints
    FN2CLogger::Get().Log(TEXT("Starting Blueprint discovery for batch processing"), EN2CLogSeverity::Info);
    
    const FN2CDiscoveryResult Discovery = FN2CBlueprintDiscovery::DiscoverAllBlueprints(Settings);
    
    if (Discovery.ValidBlueprints.Num() == 0)
    {
        const FString ErrorMsg = TEXT("No valid Blueprints found for export");
        Result.ErrorMessages.Add(ErrorMsg);
        FN2CLogger::Get().LogWarning(ErrorMsg);
        if (ErrorDelegate.IsBound())
        {
            ErrorDelegate.Execute(ErrorMsg);
        }
        return Result;
    }
    
    FN2CLogger::Get().Log(
        FString::Printf(TEXT("Discovered %d valid Blueprints for batch processing"), Discovery.ValidBlueprints.Num()),
        EN2CLogSeverity::Info
    );
    
    // Process the discovered Blueprints
    return ProcessBlueprintsInternal(Discovery.ValidBlueprints, Settings, ProgressDelegate, ProcessedDelegate, ErrorDelegate);
}

FN2CBatchResult FN2CBatchProcessor::ProcessBlueprintList(
    const TArray<UBlueprint*>& Blueprints,
    const UN2CTranslatorSettings* Settings,
    FOnN2CProgressUpdate ProgressDelegate,
    FOnN2CBlueprintProcessed ProcessedDelegate,
    FOnN2CError ErrorDelegate)
{
    FN2CBatchResult Result;
    
    if (!Settings)
    {
        Result.ErrorMessages.Add(TEXT("Invalid settings provided"));
        return Result;
    }
    
    if (Blueprints.Num() == 0)
    {
        FN2CLogger::Get().LogWarning(TEXT("Empty Blueprint list provided for batch processing"));
        return Result;
    }
    
    return ProcessBlueprintsInternal(Blueprints, Settings, ProgressDelegate, ProcessedDelegate, ErrorDelegate);
}

FN2CBatchResult FN2CBatchProcessor::ProcessBlueprintsInDirectory(
    const FString& DirectoryPath,
    const UN2CTranslatorSettings* Settings,
    FOnN2CProgressUpdate ProgressDelegate,
    FOnN2CBlueprintProcessed ProcessedDelegate,
    FOnN2CError ErrorDelegate)
{
    FN2CBatchResult Result;
    
    if (!Settings)
    {
        Result.ErrorMessages.Add(TEXT("Invalid settings provided"));
        return Result;
    }
    
    // Discover Blueprints in the specified directory
    const FN2CDiscoveryResult Discovery = FN2CBlueprintDiscovery::DiscoverBlueprintsInDirectory(DirectoryPath, Settings);
    
    if (Discovery.ValidBlueprints.Num() == 0)
    {
        const FString ErrorMsg = FString::Printf(TEXT("No valid Blueprints found in directory: %s"), *DirectoryPath);
        Result.ErrorMessages.Add(ErrorMsg);
        FN2CLogger::Get().LogWarning(ErrorMsg);
        return Result;
    }
    
    return ProcessBlueprintsInternal(Discovery.ValidBlueprints, Settings, ProgressDelegate, ProcessedDelegate, ErrorDelegate);
}

bool FN2CBatchProcessor::IsBatchProcessingActive()
{
    return bIsBatchProcessingActive;
}

void FN2CBatchProcessor::CancelBatchProcessing()
{
    if (bIsBatchProcessingActive)
    {
        bCancelRequested = true;
        FN2CLogger::Get().Log(TEXT("Batch processing cancellation requested"), EN2CLogSeverity::Info);
    }
}

float FN2CBatchProcessor::GetBatchProgress()
{
    return CurrentBatchProgress;
}

FString FN2CBatchProcessor::GetBatchStatusMessage()
{
    return CurrentBatchStatus;
}

FN2CBatchResult FN2CBatchProcessor::ProcessBlueprintsInternal(
    const TArray<UBlueprint*>& Blueprints,
    const UN2CTranslatorSettings* Settings,
    FOnN2CProgressUpdate ProgressDelegate,
    FOnN2CBlueprintProcessed ProcessedDelegate,
    FOnN2CError ErrorDelegate)
{
    FN2CBatchResult Result;
    
    // Set batch processing state
    bIsBatchProcessingActive = true;
    bCancelRequested = false;
    CurrentBatchProgress = 0.0f;
    CurrentBatchIndex = 0;
    CurrentBatchTotal = Blueprints.Num();
    
    const double StartTime = FPlatformTime::Seconds();
    
    Result.TotalBlueprints = Blueprints.Num();
    
    FN2CLogger::Get().Log(
        FString::Printf(TEXT("Starting batch processing of %d Blueprints"), Result.TotalBlueprints),
        EN2CLogSeverity::Info
    );
    
    // Process each Blueprint
    for (int32 Index = 0; Index < Blueprints.Num(); ++Index)
    {
        // Check for cancellation
        if (bCancelRequested)
        {
            Result.ErrorMessages.Add(TEXT("Batch processing was cancelled"));
            FN2CLogger::Get().Log(TEXT("Batch processing cancelled by user"), EN2CLogSeverity::Info);
            break;
        }
        
        const UBlueprint* Blueprint = Blueprints[Index];
        if (!Blueprint)
        {
            Result.Failed++;
            const FString ErrorMsg = FString::Printf(TEXT("Null Blueprint at index %d"), Index);
            HandleProcessingError(nullptr, ErrorMsg, ErrorDelegate, Result.ErrorMessages);
            continue;
        }
        
        CurrentBatchIndex = Index + 1;
        const FString BlueprintName = Blueprint->GetName();
        
        // Update progress
        UpdateProgress(CurrentBatchIndex, CurrentBatchTotal, BlueprintName, ProgressDelegate);
        
        // Process the Blueprint
        const FN2CExportResult ExportResult = ProcessSingleBlueprint(Blueprint, Settings);
        
        if (ExportResult.bSuccess)
        {
            Result.ProcessedSuccessfully++;
            Result.TotalBytesWritten += ExportResult.BytesWritten;
            
            if (ProcessedDelegate.IsBound())
            {
                ProcessedDelegate.Execute(BlueprintName);
            }
            
            FN2CLogger::Get().Log(
                FString::Printf(TEXT("Exported Blueprint: %s (%d/%d)"), 
                *BlueprintName, CurrentBatchIndex, CurrentBatchTotal),
                EN2CLogSeverity::Info
            );
        }
        else
        {
            Result.Failed++;
            HandleProcessingError(Blueprint, ExportResult.ErrorMessage, ErrorDelegate, Result.ErrorMessages);
        }
        
        // Update progress (current item completed)
        CurrentBatchProgress = static_cast<float>(CurrentBatchIndex) / static_cast<float>(CurrentBatchTotal);
    }
    
    // Finalize results
    Result.TotalProcessingTime = FPlatformTime::Seconds() - StartTime;
    Result.bSuccess = Result.Failed == 0 && !bCancelRequested;
    
    // Update final status
    CurrentBatchStatus = Result.bSuccess ? TEXT("Completed successfully") : TEXT("Completed with errors");
    CurrentBatchProgress = 1.0f;
    
    // Clean up
    CleanupBatchResources();
    bIsBatchProcessingActive = false;
    bCancelRequested = false;
    
    FN2CLogger::Get().Log(
        FString::Printf(TEXT("Batch processing completed: %d successful, %d failed, %.2f seconds total"),
        Result.ProcessedSuccessfully, Result.Failed, Result.TotalProcessingTime),
        Result.bSuccess ? EN2CLogSeverity::Info : EN2CLogSeverity::Warning
    );
    
    return Result;
}

FN2CExportResult FN2CBatchProcessor::ProcessSingleBlueprint(
    const UBlueprint* Blueprint,
    const UN2CTranslatorSettings* Settings)
{
    if (!Blueprint || !Settings)
    {
        FN2CExportResult Result;
        Result.ErrorMessage = TEXT("Invalid Blueprint or Settings");
        return Result;
    }
    
    try
    {
        return FN2CFileExporter::ExportBlueprintToFile(Blueprint, Settings);
    }
    catch (const std::exception& e)
    {
        FN2CExportResult Result;
        Result.ErrorMessage = FString::Printf(TEXT("Exception during export: %s"), ANSI_TO_TCHAR(e.what()));
        return Result;
    }
    catch (...)
    {
        FN2CExportResult Result;
        Result.ErrorMessage = TEXT("Unknown exception during export");
        return Result;
    }
}

void FN2CBatchProcessor::UpdateProgress(
    int32 CurrentIndex,
    int32 TotalCount,
    const FString& CurrentBlueprintName,
    FOnN2CProgressUpdate ProgressDelegate)
{
    CurrentBatchProgress = static_cast<float>(CurrentIndex) / static_cast<float>(TotalCount);
    CurrentBatchStatus = FString::Printf(TEXT("Processing: %s (%d/%d)"), *CurrentBlueprintName, CurrentIndex, TotalCount);
    
    if (ProgressDelegate.IsBound())
    {
        ProgressDelegate.Execute(CurrentIndex, TotalCount);
    }
}

void FN2CBatchProcessor::HandleProcessingError(
    const UBlueprint* Blueprint,
    const FString& ErrorMessage,
    FOnN2CError ErrorDelegate,
    TArray<FString>& ErrorMessages)
{
    const FString BlueprintName = Blueprint ? Blueprint->GetName() : TEXT("Unknown");
    const FString FullErrorMessage = FString::Printf(TEXT("Failed to export %s: %s"), *BlueprintName, *ErrorMessage);
    
    ErrorMessages.Add(FullErrorMessage);
    
    if (ErrorDelegate.IsBound())
    {
        ErrorDelegate.Execute(FullErrorMessage);
    }
    
    FN2CLogger::Get().LogError(FullErrorMessage);
}

bool FN2CBatchProcessor::ValidateBatchPrerequisites(const UN2CTranslatorSettings* Settings, FString& OutErrorMessage)
{
    if (!Settings)
    {
        OutErrorMessage = TEXT("Settings cannot be null");
        return false;
    }
    
    // Validate settings
    const TArray<FString> ValidationErrors = Settings->ValidateSettings();
    if (ValidationErrors.Num() > 0)
    {
        OutErrorMessage = FString::Printf(TEXT("Settings validation failed: %s"), *ValidationErrors[0]);
        return false;
    }
    
    // Check if another batch operation is running
    if (bIsBatchProcessingActive)
    {
        OutErrorMessage = TEXT("Another batch processing operation is already running");
        return false;
    }
    
    // Ensure export directory is writable
    const FString ExportPath = Settings->GetFullExportPath();
    if (!FN2CFileExporter::CreateDirectoryTree(ExportPath))
    {
        OutErrorMessage = FString::Printf(TEXT("Cannot create or access export directory: %s"), *ExportPath);
        return false;
    }
    
    return true;
}

float FN2CBatchProcessor::EstimateBatchProcessingTime(const TArray<UBlueprint*>& Blueprints)
{
    // Rough estimate: 0.5 seconds per Blueprint (varies by complexity)
    const float BaseTimePerBlueprint = 0.5f;
    return Blueprints.Num() * BaseTimePerBlueprint;
}

void FN2CBatchProcessor::CleanupBatchResources()
{
    // Force garbage collection to clean up loaded Blueprint assets
    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    
    // Reset progress tracking
    CurrentBatchProgress = 0.0f;
    CurrentBatchStatus = TEXT("");
    CurrentBatchIndex = 0;
    CurrentBatchTotal = 0;
}