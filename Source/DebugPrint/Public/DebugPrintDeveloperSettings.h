// Copyright Mox Alehin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_DebugPrint.h"
#include "Engine/DeveloperSettings.h"
#include "DebugPrintDeveloperSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Debug Print Plugin Settings"))
class DEBUGPRINT_API UDebugPrintDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
    /** The color used for printing text */
    UPROPERTY(EditAnywhere, config, Category = "Default")
    FLinearColor TextColor = FLinearColor(1.f, 0.f, 1.f, 1.f);

    /** The separator between values */
    UPROPERTY(EditAnywhere, config, Category = "Default")
    FString Separator = TEXT(" | ");

    /** Duration for which the printed string will appear on the screen */
    UPROPERTY(EditAnywhere, config, Category = "Default")
    float Duration = 5.0f;

    /** Enum defining the print type */
    UPROPERTY(EditAnywhere, config, Category = "Default")
    TEnumAsByte<EPrintType> PrintType = EPrintType::PrintInColumns;
};
