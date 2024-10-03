#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_AddPinInterface.h"
#include "K2Node_DebugPrint.generated.h"

UCLASS()
class DEBUGPRINT_API UK2Node_DebugPrint : public UK2Node, public IK2Node_AddPinInterface 
{
    GENERATED_BODY()
    
public:
    
    /** The number of value options this node currently has */
    UPROPERTY()
    int32 NumValuePins;
    
    virtual void AllocateDefaultPins() override;
    virtual FText GetTooltipText() const override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
    virtual FName GetCornerIcon() const override;

    //~ Begin UK2Node Interface
    // virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual FText GetMenuCategory() const override;
    // virtual FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const override;
    virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
    virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
    // virtual void PostReconstructNode() override;
    virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

    // IK2Node_AddPinInterface
    virtual void AddInputPin() override;
    virtual bool CanAddPin() const override;

    virtual void RemoveInputPin(UEdGraphPin* PinToRemove) override;
    
    virtual void PostReconstructNode() override;
    virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins);
};
