// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#include "Core/N2CToolbarCommand.h"

#include "Styling/AppStyle.h"
#include "Framework/Commands/UICommandList.h"
#include "Utils/N2CLogger.h"

#define LOCTEXT_NAMESPACE "NodeToCode"

const FName FN2CToolbarCommand::CommandName_Open = TEXT("NodeToCode_OpenWindow");
const FName FN2CToolbarCommand::CommandName_Collect = TEXT("NodeToCode_CollectNodes");
const FName FN2CToolbarCommand::CommandName_CopyJson = TEXT("NodeToCode_CopyJson");
const FName FN2CToolbarCommand::CommandName_ExportAll = TEXT("NodeToCode_ExportAll");
const FName FN2CToolbarCommand::CommandName_ExportCurrent = TEXT("NodeToCode_ExportCurrent");
const FName FN2CToolbarCommand::CommandName_TranslatorSettings = TEXT("NodeToCode_TranslatorSettings");
const FText FN2CToolbarCommand::CommandLabel_Open = NSLOCTEXT("NodeToCode", "OpenWindow", "Open Node to Code");
const FText FN2CToolbarCommand::CommandLabel_Collect = NSLOCTEXT("NodeToCode", "CollectNodes", "Collect and Translate Nodes");
const FText FN2CToolbarCommand::CommandLabel_CopyJson = NSLOCTEXT("NodeToCode", "CopyJson", "Copy Blueprint JSON");
const FText FN2CToolbarCommand::CommandLabel_ExportAll = NSLOCTEXT("NodeToCode", "ExportAll", "Export All Blueprints");
const FText FN2CToolbarCommand::CommandLabel_ExportCurrent = NSLOCTEXT("NodeToCode", "ExportCurrent", "Export Current Blueprint");
const FText FN2CToolbarCommand::CommandLabel_TranslatorSettings = NSLOCTEXT("NodeToCode", "TranslatorSettings", "Translator Settings...");
const FText FN2CToolbarCommand::CommandTooltip_Open = NSLOCTEXT("NodeToCode", "OpenWindowTooltip", "Open the Node to Code window");
const FText FN2CToolbarCommand::CommandTooltip_Collect = NSLOCTEXT("NodeToCode", "CollectNodesTooltip", "Collect nodes from current Blueprint graph and translate to code");
const FText FN2CToolbarCommand::CommandTooltip_CopyJson = NSLOCTEXT("NodeToCode", "CopyJsonTooltip", "Copy the serialized Blueprint JSON to clipboard");
const FText FN2CToolbarCommand::CommandTooltip_ExportAll = NSLOCTEXT("NodeToCode", "ExportAllTooltip", "Export all Blueprints in the project to JSON files");
const FText FN2CToolbarCommand::CommandTooltip_ExportCurrent = NSLOCTEXT("NodeToCode", "ExportCurrentTooltip", "Export the current Blueprint to JSON file");
const FText FN2CToolbarCommand::CommandTooltip_TranslatorSettings = NSLOCTEXT("NodeToCode", "TranslatorSettingsTooltip", "Open the translator settings panel");

FN2CToolbarCommand::FN2CToolbarCommand()
    : TCommands<FN2CToolbarCommand>(
        TEXT("NodeToCode"), // Context name for fast lookup
        NSLOCTEXT("NodeToCode", "NodeToCode", "Node to Code"), // Localized context name
        NAME_None, // Parent
        FAppStyle::GetAppStyleSetName() // Icon style
    )
{
}

void FN2CToolbarCommand::RegisterCommands()
{
    FN2CLogger::Get().Log(TEXT("Registering N2C toolbar commands"), EN2CLogSeverity::Debug);
    
    UI_COMMAND(
        OpenWindowCommand,
        "Open Node to Code Editor",
        "Open the Node to Code Editor window",
        EUserInterfaceActionType::Button,
        FInputChord()
    );

    UI_COMMAND(
        CollectNodesCommand,
        "Translate Blueprint Graph to Code",
        "Translate current Blueprint graph to code.\nResults will be in the Node to Code Editor window.",
        EUserInterfaceActionType::Button,
        FInputChord()
    );
    
    UI_COMMAND(
        CopyJsonCommand,
        "Copy Blueprint JSON",
        "Copy the serialized Blueprint JSON to clipboard for external use",
        EUserInterfaceActionType::Button,
        FInputChord()
    );

    UI_COMMAND(
        ExportAllBlueprintsCommand,
        "Export All Blueprints",
        "Export all Blueprints in the project to JSON files",
        EUserInterfaceActionType::Button,
        FInputChord()
    );

    UI_COMMAND(
        ExportCurrentBlueprintCommand,
        "Export Current Blueprint",
        "Export the current Blueprint to JSON file",
        EUserInterfaceActionType::Button,
        FInputChord()
    );

    UI_COMMAND(
        TranslatorSettingsCommand,
        "Translator Settings...",
        "Open the translator settings panel",
        EUserInterfaceActionType::Button,
        FInputChord()
    );
    
    FN2CLogger::Get().Log(TEXT("N2C toolbar commands registered"), EN2CLogSeverity::Debug);
}

#undef LOCTEXT_NAMESPACE
