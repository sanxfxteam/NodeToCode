// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "Models/N2CBlueprint.h"
#include "N2CTranslatorTypes.h"
#include "N2CTranslatorSettings.h"

/**
 * @class FN2CFileExporter
 * @brief Handles exporting Blueprint JSON data to files
 *
 * This class manages the file writing operations for Blueprint JSON exports,
 * including directory creation, file naming, and various export strategies.
 */
class NODETOCODE_API FN2CFileExporter
{
public:
    /** Export a Blueprint to file(s) according to settings */
    static FN2CExportResult ExportBlueprintToFile(const UBlueprint* Blueprint, const UN2CTranslatorSettings* Settings);

    /** Export a single Blueprint as one JSON file containing all graphs */
    static FN2CExportResult ExportSingleFile(const UBlueprint* Blueprint, const FN2CBlueprint& N2CBlueprint, const UN2CTranslatorSettings* Settings);

    /** Export a Blueprint as multiple JSON files, one per graph */
    static FN2CExportResult ExportMultipleFiles(const UBlueprint* Blueprint, const FN2CBlueprint& N2CBlueprint, const UN2CTranslatorSettings* Settings);

    /** Export using hybrid strategy based on graph count */
    static FN2CExportResult ExportHybrid(const UBlueprint* Blueprint, const FN2CBlueprint& N2CBlueprint, const UN2CTranslatorSettings* Settings);

    /** Generate the export file path for a Blueprint */
    static FString GenerateExportPath(const UBlueprint* Blueprint, const FN2CGraph* Graph, const UN2CTranslatorSettings* Settings);

    /** Create directory tree for the given path */
    static bool CreateDirectoryTree(const FString& DirectoryPath);

    /** Write JSON content to file with optional backup */
    static bool WriteJsonToFile(const FString& FilePath, const FString& JsonContent, bool bCreateBackup = false);

    /** Check if file should be overwritten based on modification times */
    static bool ShouldUpdateFile(const FString& FilePath, const UBlueprint* Blueprint, bool bIncrementalExport);

    /** Get file size of exported JSON */
    static int64 GetFileSize(const FString& FilePath);

    /** Create backup of existing file */
    static bool CreateBackupFile(const FString& FilePath);

private:
    /** Translate Blueprint to N2C format */
    static bool TranslateBlueprint(const UBlueprint* Blueprint, FN2CBlueprint& OutN2CBlueprint);

    /** Get the Blueprint's asset modification time */
    static FDateTime GetBlueprintModificationTime(const UBlueprint* Blueprint);

    /** Get file modification time */
    static FDateTime GetFileModificationTime(const FString& FilePath);

    /** Generate a unique backup filename */
    static FString GenerateBackupFileName(const FString& OriginalFilePath);
};