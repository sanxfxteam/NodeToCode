// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#include "Core/N2CFileExporter.h"
#include "Core/N2CNodeCollector.h"
#include "Core/N2CNodeTranslator.h"
#include "Core/N2CSerializer.h"
#include "Utils/N2CLogger.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"

FN2CExportResult FN2CFileExporter::ExportBlueprintToFile(const UBlueprint* Blueprint, const UN2CTranslatorSettings* Settings)
{
    FN2CExportResult Result;
    
    if (!Blueprint || !Settings)
    {
        Result.ErrorMessage = TEXT("Invalid Blueprint or Settings");
        return Result;
    }
    
    const double StartTime = FPlatformTime::Seconds();
    
    // Translate Blueprint to N2C format
    FN2CBlueprint N2CBlueprint;
    if (!TranslateBlueprint(Blueprint, N2CBlueprint))
    {
        Result.ErrorMessage = TEXT("Failed to translate Blueprint");
        return Result;
    }
    
    // Determine export strategy based on settings and graph count
    const EN2CFileStrategy EffectiveStrategy = Settings->GetEffectiveFileStrategy(N2CBlueprint.Graphs.Num());
    
    // Export using appropriate strategy
    switch (EffectiveStrategy)
    {
        case EN2CFileStrategy::SingleFilePerBlueprint:
            Result = ExportSingleFile(Blueprint, N2CBlueprint, Settings);
            break;
            
        case EN2CFileStrategy::MultipleFilesPerGraph:
            Result = ExportMultipleFiles(Blueprint, N2CBlueprint, Settings);
            break;
            
        case EN2CFileStrategy::Hybrid:
            Result = ExportHybrid(Blueprint, N2CBlueprint, Settings);
            break;
            
        default:
            Result.ErrorMessage = TEXT("Unsupported file strategy");
            return Result;
    }
    
    Result.ProcessingTime = FPlatformTime::Seconds() - StartTime;
    
    if (Result.bSuccess)
    {
        FN2CLogger::Get().Log(
            FString::Printf(TEXT("Successfully exported Blueprint '%s' to '%s' in %.2f seconds"), 
            *Blueprint->GetName(), *Result.OutputPath, Result.ProcessingTime),
            EN2CLogSeverity::Info
        );
    }
    else
    {
        FN2CLogger::Get().LogError(
            FString::Printf(TEXT("Failed to export Blueprint '%s': %s"), 
            *Blueprint->GetName(), *Result.ErrorMessage)
        );
    }
    
    return Result;
}

FN2CExportResult FN2CFileExporter::ExportSingleFile(const UBlueprint* Blueprint, const FN2CBlueprint& N2CBlueprint, const UN2CTranslatorSettings* Settings)
{
    FN2CExportResult Result;
    
    // Generate file path
    const FString OutputPath = GenerateExportPath(Blueprint, nullptr, Settings);
    
    // Check if we should update this file
    if (!ShouldUpdateFile(OutputPath, Blueprint, Settings->bIncrementalExport))
    {
        Result.bSuccess = true;
        Result.OutputPath = OutputPath;
        FN2CLogger::Get().Log(
            FString::Printf(TEXT("Skipping unchanged Blueprint: %s"), *Blueprint->GetName()),
            EN2CLogSeverity::Debug
        );
        return Result;
    }
    
    // Ensure directory exists
    if (!CreateDirectoryTree(FPaths::GetPath(OutputPath)))
    {
        Result.ErrorMessage = FString::Printf(TEXT("Failed to create directory: %s"), *FPaths::GetPath(OutputPath));
        return Result;
    }
    
    // Serialize to JSON
    FN2CSerializer::SetPrettyPrint(Settings->bPrettyPrintJson);
    const FString JsonContent = FN2CSerializer::ToJson(N2CBlueprint);
    
    if (JsonContent.IsEmpty())
    {
        Result.ErrorMessage = TEXT("JSON serialization failed");
        return Result;
    }
    
    // Write to file
    if (!WriteJsonToFile(OutputPath, JsonContent, Settings->bCreateBackups))
    {
        Result.ErrorMessage = FString::Printf(TEXT("Failed to write file: %s"), *OutputPath);
        return Result;
    }
    
    Result.bSuccess = true;
    Result.OutputPath = OutputPath;
    Result.BytesWritten = JsonContent.Len();
    
    return Result;
}

FN2CExportResult FN2CFileExporter::ExportMultipleFiles(const UBlueprint* Blueprint, const FN2CBlueprint& N2CBlueprint, const UN2CTranslatorSettings* Settings)
{
    FN2CExportResult Result;
    
    int32 TotalBytesWritten = 0;
    TArray<FString> ExportedFiles;
    
    // Export each graph as separate file
    for (const FN2CGraph& Graph : N2CBlueprint.Graphs)
    {
        // Create single-graph Blueprint structure
        FN2CBlueprint SingleGraphBlueprint = N2CBlueprint;
        SingleGraphBlueprint.Graphs = {Graph};
        
        // Generate file path for this graph
        const FString OutputPath = GenerateExportPath(Blueprint, &Graph, Settings);
        
        // Check if we should update this file
        if (!ShouldUpdateFile(OutputPath, Blueprint, Settings->bIncrementalExport))
        {
            FN2CLogger::Get().Log(
                FString::Printf(TEXT("Skipping unchanged graph: %s::%s"), *Blueprint->GetName(), *Graph.Name),
                EN2CLogSeverity::Debug
            );
            continue;
        }
        
        // Ensure directory exists
        if (!CreateDirectoryTree(FPaths::GetPath(OutputPath)))
        {
            Result.ErrorMessage = FString::Printf(TEXT("Failed to create directory: %s"), *FPaths::GetPath(OutputPath));
            return Result;
        }
        
        // Serialize and write
        FN2CSerializer::SetPrettyPrint(Settings->bPrettyPrintJson);
        const FString JsonContent = FN2CSerializer::ToJson(SingleGraphBlueprint);
        
        if (JsonContent.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(TEXT("JSON serialization failed for graph: %s"), *Graph.Name);
            return Result;
        }
        
        if (!WriteJsonToFile(OutputPath, JsonContent, Settings->bCreateBackups))
        {
            Result.ErrorMessage = FString::Printf(TEXT("Failed to write file: %s"), *OutputPath);
            return Result;
        }
        
        TotalBytesWritten += JsonContent.Len();
        ExportedFiles.Add(OutputPath);
        
        FN2CLogger::Get().Log(
            FString::Printf(TEXT("Exported graph '%s' to '%s'"), *Graph.Name, *OutputPath),
            EN2CLogSeverity::Debug
        );
    }
    
    Result.bSuccess = true;
    Result.OutputPath = FString::Join(ExportedFiles, TEXT("; "));
    Result.BytesWritten = TotalBytesWritten;
    
    return Result;
}

FN2CExportResult FN2CFileExporter::ExportHybrid(const UBlueprint* Blueprint, const FN2CBlueprint& N2CBlueprint, const UN2CTranslatorSettings* Settings)
{
    // Hybrid strategy: use single file if graphs <= threshold, multiple files otherwise
    const bool bUseSingleFile = N2CBlueprint.Graphs.Num() <= Settings->GraphCountThreshold;
    
    if (bUseSingleFile)
    {
        return ExportSingleFile(Blueprint, N2CBlueprint, Settings);
    }
    else
    {
        return ExportMultipleFiles(Blueprint, N2CBlueprint, Settings);
    }
}

FString FN2CFileExporter::GenerateExportPath(const UBlueprint* Blueprint, const FN2CGraph* Graph, const UN2CTranslatorSettings* Settings)
{
    // Get Blueprint's package path
    FString BlueprintPath = Blueprint->GetPathName();
    
    // Convert from "/Game/MyFolder/MyBlueprint" to "MyFolder/MyBlueprint"
    BlueprintPath = BlueprintPath.Replace(TEXT("/Game/"), TEXT(""));
    
    // Build export directory path
    const FString ExportDir = FPaths::Combine(Settings->GetFullExportPath(), FPaths::GetPath(BlueprintPath));
    
    // Generate filename
    FString FileName = FPaths::GetBaseFilename(BlueprintPath);
    if (Graph != nullptr)
    {
        // Clean graph name for filename (remove invalid characters)
        FString CleanGraphName = Graph->Name;
        CleanGraphName = CleanGraphName.Replace(TEXT(" "), TEXT("_"));
        CleanGraphName = CleanGraphName.Replace(TEXT("<"), TEXT("_"));
        CleanGraphName = CleanGraphName.Replace(TEXT(">"), TEXT("_"));
        CleanGraphName = CleanGraphName.Replace(TEXT(":"), TEXT("_"));
        CleanGraphName = CleanGraphName.Replace(TEXT("\""), TEXT("_"));
        CleanGraphName = CleanGraphName.Replace(TEXT("/"), TEXT("_"));
        CleanGraphName = CleanGraphName.Replace(TEXT("\\"), TEXT("_"));
        CleanGraphName = CleanGraphName.Replace(TEXT("|"), TEXT("_"));
        CleanGraphName = CleanGraphName.Replace(TEXT("?"), TEXT("_"));
        CleanGraphName = CleanGraphName.Replace(TEXT("*"), TEXT("_"));
        
        FileName += FString::Printf(TEXT("_%s"), *CleanGraphName);
    }
    FileName += TEXT(".json");
    
    return FPaths::Combine(ExportDir, FileName);
}

bool FN2CFileExporter::CreateDirectoryTree(const FString& DirectoryPath)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.CreateDirectoryTree(*DirectoryPath);
}

bool FN2CFileExporter::WriteJsonToFile(const FString& FilePath, const FString& JsonContent, bool bCreateBackup)
{
    // Create backup if requested and file exists
    if (bCreateBackup && FPaths::FileExists(FilePath))
    {
        if (!CreateBackupFile(FilePath))
        {
            FN2CLogger::Get().LogWarning(FString::Printf(TEXT("Failed to create backup for: %s"), *FilePath));
        }
    }
    
    // Write the JSON content to file
    return FFileHelper::SaveStringToFile(JsonContent, *FilePath, FFileHelper::EEncodingOptions::AutoDetect);
}

bool FN2CFileExporter::ShouldUpdateFile(const FString& FilePath, const UBlueprint* Blueprint, bool bIncrementalExport)
{
    if (!bIncrementalExport)
    {
        return true; // Always update if incremental export is disabled
    }
    
    if (!FPaths::FileExists(FilePath))
    {
        return true; // File doesn't exist, need to create it
    }
    
    // Compare modification times
    const FDateTime BlueprintTime = GetBlueprintModificationTime(Blueprint);
    const FDateTime FileTime = GetFileModificationTime(FilePath);
    
    // Update if Blueprint is newer than the exported file
    return BlueprintTime > FileTime;
}

int64 FN2CFileExporter::GetFileSize(const FString& FilePath)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.FileSize(*FilePath);
}

bool FN2CFileExporter::CreateBackupFile(const FString& FilePath)
{
    const FString BackupPath = GenerateBackupFileName(FilePath);
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.CopyFile(*BackupPath, *FilePath);
}

bool FN2CFileExporter::TranslateBlueprint(const UBlueprint* Blueprint, FN2CBlueprint& OutN2CBlueprint)
{
    if (!Blueprint)
    {
        return false;
    }
    
    // Get all graphs from the Blueprint
    TArray<UEdGraph*> AllGraphs;
    
    // Add Ubergraph pages (Event Graphs)
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph)
        {
            AllGraphs.Add(Graph);
        }
    }
    
    // Add function graphs
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (Graph)
        {
            AllGraphs.Add(Graph);
        }
    }
    
    // Add macro graphs if this is a macro library
    for (UEdGraph* Graph : Blueprint->MacroGraphs)
    {
        if (Graph)
        {
            AllGraphs.Add(Graph);
        }
    }
    
    if (AllGraphs.Num() == 0)
    {
        FN2CLogger::Get().LogWarning(FString::Printf(TEXT("Blueprint '%s' has no graphs to export"), *Blueprint->GetName()));
        return false;
    }
    
    // Collect nodes from all graphs
    TArray<UK2Node*> AllCollectedNodes;
    FN2CNodeCollector& Collector = FN2CNodeCollector::Get();
    
    for (UEdGraph* Graph : AllGraphs)
    {
        TArray<UK2Node*> GraphNodes;
        if (Collector.CollectNodesFromGraph(Graph, GraphNodes))
        {
            AllCollectedNodes.Append(GraphNodes);
        }
    }
    
    if (AllCollectedNodes.Num() == 0)
    {
        FN2CLogger::Get().LogWarning(FString::Printf(TEXT("No nodes collected from Blueprint '%s'"), *Blueprint->GetName()));
        return false;
    }
    
    // Translate nodes to N2C format
    FN2CNodeTranslator& Translator = FN2CNodeTranslator::Get();
    if (!Translator.GenerateN2CStruct(AllCollectedNodes))
    {
        FN2CLogger::Get().LogError(FString::Printf(TEXT("Failed to translate nodes for Blueprint '%s'"), *Blueprint->GetName()));
        return false;
    }
    
    // Get the generated Blueprint structure
    OutN2CBlueprint = Translator.GetN2CBlueprint();
    
    // Validate the result
    if (!OutN2CBlueprint.IsValid())
    {
        FN2CLogger::Get().LogError(FString::Printf(TEXT("Generated N2C Blueprint is invalid for '%s'"), *Blueprint->GetName()));
        return false;
    }
    
    return true;
}

FDateTime FN2CFileExporter::GetBlueprintModificationTime(const UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return FDateTime::MinValue();
    }
    
    // Get the package file path
    const FString PackageFilePath = FPackageName::LongPackageNameToFilename(Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    
    return GetFileModificationTime(PackageFilePath);
}

FDateTime FN2CFileExporter::GetFileModificationTime(const FString& FilePath)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.GetTimeStamp(*FilePath);
}

bool FN2CFileExporter::DeleteBlueprintJsonFiles(const FString& BlueprintPath, const UN2CTranslatorSettings* Settings)
{
    if (!Settings)
    {
        FN2CLogger::Get().LogError(TEXT("Cannot delete Blueprint JSON files: Settings is null"));
        return false;
    }

    // Convert from package path to relative path (e.g., "/Game/MyFolder/MyBlueprint" to "MyFolder/MyBlueprint")
    FString RelativePath = BlueprintPath;
    RelativePath = RelativePath.Replace(TEXT("/Game/"), TEXT(""));
    
    // Build export directory path
    const FString ExportDir = FPaths::Combine(Settings->GetFullExportPath(), FPaths::GetPath(RelativePath));
    const FString BaseFileName = FPaths::GetBaseFilename(RelativePath);
    
    bool bAllDeletedSuccessfully = true;
    int32 DeletedFileCount = 0;
    
    // Find and delete JSON files associated with this Blueprint
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    // Check for single file export (BlueprintName.json)
    const FString SingleFilePath = FPaths::Combine(ExportDir, BaseFileName + TEXT(".json"));
    if (PlatformFile.FileExists(*SingleFilePath))
    {
        if (PlatformFile.DeleteFile(*SingleFilePath))
        {
            DeletedFileCount++;
            FN2CLogger::Get().Log(
                FString::Printf(TEXT("Deleted JSON file: %s"), *SingleFilePath),
                EN2CLogSeverity::Debug
            );
        }
        else
        {
            bAllDeletedSuccessfully = false;
            FN2CLogger::Get().LogError(
                FString::Printf(TEXT("Failed to delete JSON file: %s"), *SingleFilePath)
            );
        }
    }
    
    // Check for multiple files export (BlueprintName_GraphName.json pattern)
    TArray<FString> JsonFiles;
    PlatformFile.FindFiles(JsonFiles, *ExportDir, TEXT(".json"));
    
    for (const FString& JsonFile : JsonFiles)
    {
        const FString FullJsonPath = FPaths::Combine(ExportDir, JsonFile);
        const FString JsonBaseName = FPaths::GetBaseFilename(JsonFile);
        
        // Check if this JSON file belongs to our Blueprint
        // Pattern: BlueprintName_GraphName.json or BlueprintName.json
        if (JsonBaseName.StartsWith(BaseFileName + TEXT("_")) || JsonBaseName == BaseFileName)
        {
            if (PlatformFile.DeleteFile(*FullJsonPath))
            {
                DeletedFileCount++;
                FN2CLogger::Get().Log(
                    FString::Printf(TEXT("Deleted JSON file: %s"), *FullJsonPath),
                    EN2CLogSeverity::Debug
                );
            }
            else
            {
                bAllDeletedSuccessfully = false;
                FN2CLogger::Get().LogError(
                    FString::Printf(TEXT("Failed to delete JSON file: %s"), *FullJsonPath)
                );
            }
        }
    }
    
    // Also delete any backup files associated with this Blueprint
    TArray<FString> BackupFiles;
    PlatformFile.FindFiles(BackupFiles, *ExportDir, TEXT("*backup.json"));
    
    for (const FString& BackupFile : BackupFiles)
    {
        const FString FullBackupPath = FPaths::Combine(ExportDir, BackupFile);
        const FString BackupBaseName = FPaths::GetBaseFilename(BackupFile);
        
        // Check if this backup file belongs to our Blueprint
        if (BackupBaseName.Contains(BaseFileName + TEXT("_")))
        {
            if (PlatformFile.DeleteFile(*FullBackupPath))
            {
                DeletedFileCount++;
                FN2CLogger::Get().Log(
                    FString::Printf(TEXT("Deleted backup file: %s"), *FullBackupPath),
                    EN2CLogSeverity::Debug
                );
            }
            else
            {
                bAllDeletedSuccessfully = false;
                FN2CLogger::Get().LogError(
                    FString::Printf(TEXT("Failed to delete backup file: %s"), *FullBackupPath)
                );
            }
        }
    }
    
    FN2CLogger::Get().Log(
        FString::Printf(TEXT("JSON cleanup complete for Blueprint '%s': %d files deleted"), *BlueprintPath, DeletedFileCount),
        EN2CLogSeverity::Info
    );
    
    return bAllDeletedSuccessfully;
}

FString FN2CFileExporter::GenerateBackupFileName(const FString& OriginalFilePath)
{
    const FDateTime Now = FDateTime::Now();
    const FString Timestamp = Now.ToString(TEXT("%Y%m%d_%H%M%S"));
    
    const FString BaseName = FPaths::GetBaseFilename(OriginalFilePath);
    const FString Extension = FPaths::GetExtension(OriginalFilePath);
    const FString Directory = FPaths::GetPath(OriginalFilePath);
    
    return FPaths::Combine(Directory, FString::Printf(TEXT("%s_%s_backup.%s"), *BaseName, *Timestamp, *Extension));
}