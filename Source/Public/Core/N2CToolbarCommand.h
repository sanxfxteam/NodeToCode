// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FN2CToolbarCommand : public TCommands<FN2CToolbarCommand>
{
public:
    FN2CToolbarCommand();

    // TCommands interface
    virtual void RegisterCommands() override;

    // Commands
    TSharedPtr<FUICommandInfo> OpenWindowCommand;
    TSharedPtr<FUICommandInfo> CollectNodesCommand;
    TSharedPtr<FUICommandInfo> CopyJsonCommand;
    TSharedPtr<FUICommandInfo> ExportAllBlueprintsCommand;
    TSharedPtr<FUICommandInfo> ExportCurrentBlueprintCommand;
    TSharedPtr<FUICommandInfo> TranslatorSettingsCommand;

    // Command names and labels
    static const FName CommandName_Open;
    static const FName CommandName_Collect;
    static const FName CommandName_CopyJson;
    static const FName CommandName_ExportAll;
    static const FName CommandName_ExportCurrent;
    static const FName CommandName_TranslatorSettings;
    static const FText CommandLabel_Open;
    static const FText CommandLabel_Collect;
    static const FText CommandLabel_CopyJson;
    static const FText CommandLabel_ExportAll;
    static const FText CommandLabel_ExportCurrent;
    static const FText CommandLabel_TranslatorSettings;
    static const FText CommandTooltip_Open;
    static const FText CommandTooltip_Collect;
    static const FText CommandTooltip_CopyJson;
    static const FText CommandTooltip_ExportAll;
    static const FText CommandTooltip_ExportCurrent;
    static const FText CommandTooltip_TranslatorSettings;
};
