#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_DebugPrint.generated.h"

UCLASS()
class DEBUGPRINT_API UK2Node_DebugPrint : public UK2Node
{
    GENERATED_BODY()
    
    virtual void AllocateDefaultPins() override;
    virtual FText GetTooltipText() const override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
    virtual FName GetCornerIcon() const override;

    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual FText GetMenuCategory() const override;
    virtual FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const override;

    virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
    virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
};
