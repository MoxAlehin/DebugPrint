#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_EditablePinBase.h"
#include "K2Node_AddPinInterface.h"
#include "K2Node_DebugPrint.generated.h"

UCLASS()
class DEBUGPRINT_API UK2Node_DebugPrint : public UK2Node_EditablePinBase, public IK2Node_AddPinInterface
{
    GENERATED_BODY()
    
public:
    
    /** The number of value options this node currently has */
    UPROPERTY()
    int32 NumValuePins;

    //~ Begin UEdGraphNode Interface
    virtual void AllocateDefaultPins() override;
    virtual FText GetTooltipText() const override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
    virtual FName GetCornerIcon() const override;
    //~ End of UEdGraphNode Interface

    //~ Begin UK2Node Interface
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual FText GetMenuCategory() const override;
    virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
    virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
    virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
    virtual void PostReconstructNode() override;
    virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
    // End of Begin UK2Node Interface

    //~ Begin IK2Node_AddPinInterface
    virtual void AddInputPin() override;
    virtual bool CanAddPin() const override;
    virtual void RemoveInputPin(UEdGraphPin* PinToRemove) override;
    // End of IK2Node_AddPinInterface

    //~ Begin UK2Node_EditablePinBase Interface
    virtual bool ShouldUseConstRefParams() const override { return true; }
    virtual UEdGraphPin* CreatePinFromUserDefinition(const TSharedPtr<FUserPinInfo> NewPinInfo) override;
    virtual bool CanCreateUserDefinedPin(const FEdGraphPinType& InPinType, EEdGraphPinDirection InDesiredDirection, FText& OutErrorMessage) override;
    // End of UK2Node_EditablePinBase Interface
    
private:
    void ResetPinToWildcard(UEdGraphPin* PinToReset);
};
