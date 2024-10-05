#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_EditablePinBase.h"
#include "K2Node_AddPinInterface.h"
#include "K2Node_DebugPrint.generated.h"

UENUM(BlueprintType)
enum EPrintType : uint8
{
	PrintInColumns UMETA(DisplayName="Print In Columns", Tooltip="Prints each value in columns, aligning labels and keys."),
	PrintLabels UMETA(DisplayName="Print With Labels", Tooltip="Prints each value with a label."),
	PrintNewLine UMETA(DisplayName="Print With New Lines", Tooltip="Prints each value on a new line."),
	PrintReplace UMETA(DisplayName="Print and Replace", Tooltip="Prints in one line but overwrites the content each time, even without overriding the key."),
	PrintInline UMETA(DisplayName="Print Inline", Tooltip="Prints all content in one line like a standard print string.")
};

UCLASS()
class DEBUGPRINT_API UK2Node_DebugPrint : public UK2Node_EditablePinBase, public IK2Node_AddPinInterface
{
    GENERATED_BODY()
    
public:

	// Добавляем массив строк
	UPROPERTY(EditAnywhere, Category = "Settings")
	TArray<FString> ValueLabels;

	UFUNCTION(BlueprintCallable, Category = "Debug")
	static void ArrayDebugPrint(
		const UObject* WorldContextObject,
		const TArray<FString>& Values,
		FName Key,
		const FString& Separator,
		const FString SourceValueLabels,
		FLinearColor TextColor,
		float Duration,
		const FString& NodeGuidString,
		TEnumAsByte<enum EPrintType> Type
	);

	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface

    //~ Begin UEdGraphNode Interface
    virtual void AllocateDefaultPins() override;
    virtual FText GetTooltipText() const override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
    virtual FName GetCornerIcon() const override;
	virtual bool ShouldShowNodeProperties() const override { return true; }
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

	//~ Begin UK2Node_EditablePinBase Interface
	virtual bool ShouldUseConstRefParams() const override { return true; }
	virtual UEdGraphPin* CreatePinFromUserDefinition(const TSharedPtr<FUserPinInfo> NewPinInfo) override;
	virtual bool CanCreateUserDefinedPin(const FEdGraphPinType& InPinType, EEdGraphPinDirection InDesiredDirection, FText& OutErrorMessage) override;
	// End of UK2Node_EditablePinBase Interface
	
    //~ Begin IK2Node_AddPinInterface
    virtual void AddInputPin() override;
    virtual bool CanAddPin() const override;
    virtual void RemoveInputPin(UEdGraphPin* PinToRemove) override;
    // End of IK2Node_AddPinInterface
	
    /**
     * Notification from the editor that the user wants to change the PinType on a selected pin
     *
     * @param Pin	The pin the user wants to change the type for
     */
    void ChangePinType(UEdGraphPin* Pin);
    /**
     * Whether the user can change the pintype on a selected pin
     *
     * @param Pin	The pin in question
     */
    bool CanChangePinType(UEdGraphPin* Pin) const;

	TArray<UEdGraphPin*> GetValuePins() const;
    
private:
    void ResetPinToWildcard(UEdGraphPin* PinToReset);
};
