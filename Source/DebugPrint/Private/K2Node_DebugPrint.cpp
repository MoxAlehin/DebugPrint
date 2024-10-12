// Copyright MoxAlehin. All Rights Reserved.

#include "K2Node_DebugPrint.h"

#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "EditorCategoryUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeArray.h"
#include "KismetCompiler.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "DebugPrintDeveloperSettings.h"

#define LOCTEXT_NAMESPACE "K2Node"

class UK2Node_MakeArray;
class UGraphEditorSettings;

void UK2Node_DebugPrint::ArrayDebugPrint(
    const UObject* WorldContextObject,
    const TArray<FString>& Values,
    FName Key,
    const FString& Separator,
    const FString SourceValueLabels,
    FLinearColor TextColor,
    float Duration,
    const FString& NodeGuidString,
    TEnumAsByte<enum EPrintType> Type
    )
{
    // If the type is inline or replace, print all values on the same line or overwrite the previous output
    if (Type == EPrintType::PrintInline || Type == EPrintType::PrintReplace)
    {
        FName ActualKey = Key == "" || Key == NAME_None ? FName(NodeGuidString) : Key;
        UKismetSystemLibrary::PrintString(
                WorldContextObject,
                FString::Join(Values, *Separator),
                true,
                false,
                TextColor,
                Duration,
                Type == EPrintType::PrintInline ? Key : ActualKey
        );
    }
    else
    {
        TArray<FString> ValueLabels;
        SourceValueLabels.ParseIntoArray(ValueLabels, TEXT("#"));

        // If the type is "PrintInColumns", align all labels to form a table-like structure
        if (Type == EPrintType::PrintInColumns)
        {
            int32 MaxLength = 0;
            for (const FString& Label : ValueLabels)
            {
                MaxLength = FMath::Max(MaxLength, Label.Len());
            }

            // Add padding to align labels
            for (int32 i = 0; i < ValueLabels.Num(); i++)
            {
                int32 Padding = MaxLength - ValueLabels[i].Len();
                ValueLabels[i] += FString::ChrN(Padding, ' ');
            }
        }

        // Print each value individually with labels or in columns if specified
        for (int32 i = 0; i < Values.Num(); i++)
        {
            FString KeyString = Key.ToString();
            FString ActualKeyString = KeyString == "" || Key == NAME_None ? NodeGuidString : KeyString;
            ActualKeyString = FString::Printf(TEXT("%s_%d"), *ActualKeyString, i);

            FString ActualValue = Values[i];
            if (Type == EPrintType::PrintLabels || Type == EPrintType::PrintInColumns)
                ActualValue = ValueLabels[i] + Separator + ActualValue;
            
            UKismetSystemLibrary::PrintString(
                WorldContextObject,
                ActualValue,
                true,
                false,
                TextColor,
                Duration,
                FName(ActualKeyString)
            );
        }
    }
}

void UK2Node_DebugPrint::AllocateDefaultPins()
{
    const UDebugPrintDeveloperSettings* PluginSettings = GetDefault<UDebugPrintDeveloperSettings>();
    
    // Create Exec pins
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
    
    // Create Value Wildcard Pins
    for (int32 i = 0; i < ValueLabels.Num(); ++i)
    {
        FName PinName = FName(*FString::Printf(TEXT("Value_%d"), i));
        UEdGraphPin* WildcardPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, PinName);
        WildcardPin->PinToolTip = TEXT("Wildcard input pin. Type will be determined by what is connected.");
        WildcardPin->PinFriendlyName = FText::FromString(ValueLabels[i]);
    }

    // Create Print String options

    UEdGraphPin* KeyPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, TEXT("Key"));
    KeyPin->bAdvancedView = true;
    KeyPin->DefaultValue = TEXT("None");
    KeyPin->PinToolTip = TEXT("If a non-empty key is provided, the message will replace any existing on-screen messages with the same key.");

    UEdGraphPin* SeparatorPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, TEXT("Separator"));
    SeparatorPin->bAdvancedView = true;
    SeparatorPin->DefaultValue = PluginSettings->Separator;
    SeparatorPin->PinToolTip = TEXT("The string used to separate each value.");

    UEdGraphPin* TextColorPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, TEXT("TextColor"));
    TextColorPin->PinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
    TextColorPin->bAdvancedView = true;
    TextColorPin->DefaultValue = PluginSettings->TextColor.ToString();
    TextColorPin->PinToolTip = TEXT("The color of the text to display.");

    UEdGraphPin* DurationPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, TEXT("Duration"));
    DurationPin->bAdvancedView = true;
    DurationPin->DefaultValue = FString::SanitizeFloat(PluginSettings->Duration);
    DurationPin->PinToolTip = TEXT("The display duration.");

    UEdGraphPin* PrintTypePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte, StaticEnum<EPrintType>(), TEXT("PrintType"));
    PrintTypePin->bAdvancedView = true;
    PrintTypePin->PinToolTip = TEXT("Defines how the content will be displayed. Each option adds functionality to the one above it.");
    PrintTypePin->DefaultValue = StaticEnum<EPrintType>()->GetNameStringByValue(PluginSettings->PrintType);

    if (AdvancedPinDisplay == ENodeAdvancedPins::NoPins)
        AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
    Super::AllocateDefaultPins();
}

void UK2Node_DebugPrint::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
    bool bIsErrorFree = true;

    // 1. Create a temporary MakeArray node
    UK2Node_MakeArray* MakeArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
    TArray<UEdGraphPin*> ValuePins = GetValuePins();
    MakeArrayNode->NumInputs = ValuePins.Num();
    MakeArrayNode->AllocateDefaultPins();
    
    // 2. Create a temporary node for the ArrayDebugPrint function
    UK2Node_CallFunction* DebugPrintNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    DebugPrintNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UK2Node_DebugPrint, ArrayDebugPrint), UK2Node_DebugPrint::StaticClass());
    DebugPrintNode->AllocateDefaultPins();
    
    // 3. Get pins from the ArrayDebugPrint function
    UEdGraphPin* ValuesPin = DebugPrintNode->FindPin(TEXT("Values"));
    UEdGraphPin* KeyPin = DebugPrintNode->FindPin(TEXT("Key"));
    UEdGraphPin* SeparatorPin = DebugPrintNode->FindPin(TEXT("Separator"));
    UEdGraphPin* TextColorPin = DebugPrintNode->FindPin(TEXT("TextColor"));
    UEdGraphPin* DurationPin = DebugPrintNode->FindPin(TEXT("Duration"));
    UEdGraphPin* NodeGuidPin = DebugPrintNode->FindPin(TEXT("NodeGUIDString"));
    UEdGraphPin* SourceValueLabelsPin = DebugPrintNode->FindPin(TEXT("SourceValueLabels"));
    UEdGraphPin* TypePin = DebugPrintNode->FindPin(TEXT("Type"));

    // 4. Connect the result pin of MakeArray to the Values pin
    bIsErrorFree &= Schema->TryCreateConnection(MakeArrayNode->GetOutputPin(), ValuesPin);
    
    // 5. Connect the value pins to the MakeArray pins
    TArray<UEdGraphPin*> MakeArrayPins;
    TArray<UEdGraphPin*> MakeArrayOutputPins;
    MakeArrayNode->GetKeyAndValuePins(MakeArrayPins, MakeArrayOutputPins);

    for (int32 i = 0; i < ValuePins.Num(); ++i)
    {
        bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*ValuePins[i], *MakeArrayPins[i]).CanSafeConnect();
    }

    // 6. Connect the exec pins
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *DebugPrintNode->GetExecPin()).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *DebugPrintNode->GetThenPin()).CanSafeConnect();

    // Connect the remaining parameters
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("Key")), *KeyPin).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("Separator")), *SeparatorPin).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("TextColor")), *TextColorPin).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("Duration")), *DurationPin).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("PrintType")), *TypePin).CanSafeConnect();

    // Set the NodeGUID as the default value for the corresponding pin
    NodeGuidPin->DefaultValue = NodeGuid.ToString();
    
    FString SourceValueLabels = "";
    for (int32 i = 0; i < ValueLabels.Num(); ++i)
    {
        SourceValueLabels += ValueLabels[i] == "" ? FString::Printf(TEXT("Value %d"), i) : ValueLabels[i];
        SourceValueLabels += "#";
    }
    
    SourceValueLabelsPin->DefaultValue = SourceValueLabels;
    
    if (!bIsErrorFree)
    {
        CompilerContext.MessageLog.Error(*LOCTEXT("InternalConnectionError", "IsValidNode: Internal connection error. @@").ToString(), this);
    }
    
    BreakAllNodeLinks();
}

void UK2Node_DebugPrint::AddInputPin()
{
    Modify();
    
    // Add an empty label when a new input pin is added
    ValueLabels.Add("");
    
    ReconstructNode();
}

void UK2Node_DebugPrint::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
    Super::NotifyPinConnectionListChanged(Pin);

    if (!Pin) // Check if the pin is valid
    {
        return;
    }

    // Check if the pin is a wildcard and has a connection
    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
    {
        if (Pin->LinkedTo.Num() > 0)
        {
            // Get the first pin connected to this wildcard pin
            UEdGraphPin* LinkedPin = Pin->LinkedTo[0];
            
            // Set the pin's type to match the connected pin's type
            Pin->PinType = LinkedPin->PinType;
        }
        else
        {
            // If disconnected, reset the pin type to wildcard
            Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
        }
    }
    ReconstructNode();
}

void UK2Node_DebugPrint::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
    if (PropertyName == TEXT("ValueLabels"))
    {
        ReconstructNode();
    }
    
    Super::PostEditChangeProperty(PropertyChangedEvent);
    GetGraph()->NotifyNodeChanged(this);
}

//////////////////////////////////////////////////////////////////////////////////////
// VISUALS
//////////////////////////////////////////////////////////////////////////////////////

bool UK2Node_DebugPrint::CanAddPin() const
{
    return true;  // Node supports the "Add Pin" button
}

void UK2Node_DebugPrint::RemoveInputPin(UEdGraphPin* PinToRemove)
{
    if (!PinToRemove || !Pins.Contains(PinToRemove))
    {
        return;
    }

    // Open a transaction to support undo/redo
    FScopedTransaction Transaction(LOCTEXT("RemovePinTx", "Remove Pin"));
    Modify();

    // Break all links before removing the pin
    PinToRemove->BreakAllPinLinks();

    // Remove the pin from the array
    Pins.Remove(PinToRemove);

    // Update the number of pins and remove the corresponding label
    FString Prefix = TEXT("Value_");
    int32 Index = FCString::Atoi(*PinToRemove->PinName.ToString().RightChop(Prefix.Len()));
    ValueLabels.RemoveAt(Index);

    // Recalculate the names of the remaining pins
    Index = 0;
    for (UEdGraphPin* Pin : Pins)
    {
        if (Pin->PinName.ToString().StartsWith(TEXT("Value_")))
        {
            // Rename the remaining pins
            Pin->Modify();
            Pin->PinName = FName(*FString::Printf(TEXT("Value_%d"), Index));
            Index++;
        }
    }

    // Rebuild the node
    ReconstructNode();

    // Notify the Blueprint that the node has changed
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UK2Node_DebugPrint::PostReconstructNode()
{
    // Restore pin types based on existing connections
    for (UEdGraphPin* Pin : Pins)
    {
        if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard && Pin->LinkedTo.Num() > 0)
        {
            Pin->PinType = Pin->LinkedTo[0]->PinType;
        }
    }

    Super::PostReconstructNode();
}

void UK2Node_DebugPrint::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    // Clear existing pins
    Pins.Empty();

    // Recreate all pins
    AllocateDefaultPins();

    // Restore the old pins and their types
    for (UEdGraphPin* OldPin : OldPins)
    {
        UEdGraphPin* NewPin = FindPin(OldPin->PinName);
        if (NewPin && NewPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
        {
            // Restore wildcard pin type
            NewPin->PinType = OldPin->PinType;

            // Restore connections
            for (UEdGraphPin* LinkedPin : OldPin->LinkedTo)
            {
                NewPin->MakeLinkTo(LinkedPin);
            }
        }
    }
    
    RestoreSplitPins(OldPins);

    // Notify the Blueprint editor that changes have been made
    GetGraph()->NotifyGraphChanged();
}

UEdGraphPin* UK2Node_DebugPrint::CreatePinFromUserDefinition(const TSharedPtr<FUserPinInfo> NewPinInfo)
{
    UEdGraphPin* NewPin = CreatePin(EGPD_Input, NewPinInfo->PinType, FName(*FString::Printf(TEXT("Value_%d"), ValueLabels.Num())));
    ValueLabels.Add(NewPinInfo->PinName.ToString());
    UserDefinedPins.RemoveAt(0);
    return NewPin;
}

bool UK2Node_DebugPrint::CanCreateUserDefinedPin(const FEdGraphPinType& InPinType,
    EEdGraphPinDirection InDesiredDirection, FText& OutErrorMessage)
{
    // Disallow Output pins
    if (InDesiredDirection == EGPD_Output)
    {
        OutErrorMessage = LOCTEXT("OutputPinNotAllowed", "Cannot add Output pins!");
        return false;
    }

    // Disallow Exec pins
    if (InPinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
    {
        OutErrorMessage = LOCTEXT("ExecPinNotAllowed", "Cannot add Exec pins!");
        return false;
    }
        
    // Disallow Map, Array, and Set container pins
    if (InPinType.IsContainer())
    {
        OutErrorMessage = LOCTEXT("ContainerTypeNotAllowed", "Cannot add Map, Array or Set pins!");
        return false;
    }

    // Default: allow adding the pin
    return true;
}

TArray<UEdGraphPin*> UK2Node_DebugPrint::GetValuePins() const
{
    TArray<UEdGraphPin*> ValuePins;

    for (UEdGraphPin* Pin : Pins)
    {
        // Check if the pin name starts with "Value_"
        if (Pin->PinName.ToString().StartsWith(TEXT("Value_")))
        {
            ValuePins.Add(Pin);
        }
    }

    return ValuePins;
}

void UK2Node_DebugPrint::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
    Super::GetNodeContextMenuActions(Menu, Context);

    // Check if the pin is a value pin (starting with "Value_")
    if (Context && Context->Pin && Context->Pin->PinName.ToString().StartsWith(TEXT("Value_")))
    {
        // Add the "Remove Pin" action
        FToolMenuSection& Section = Menu->AddSection("K2NodeDebugPrint", LOCTEXT("RemovePinHeader", "Remove Pin"));
        Section.AddMenuEntry(
            "RemovePin",
            LOCTEXT("RemovePin", "Remove Pin"),
            LOCTEXT("RemovePinTooltip", "Remove this input pin."),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateUObject(const_cast<UK2Node_DebugPrint*>(this), &UK2Node_DebugPrint::RemoveInputPin, const_cast<UEdGraphPin*>(Context->Pin)))
        );

        // Add the "Reset to Wildcard" option if the pin is not connected and not already a wildcard
        if (Context->Pin->LinkedTo.Num() == 0 && Context->Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
        {
            Section.AddMenuEntry(
                "ResetPinToWildcard",
                LOCTEXT("ResetPinToWildcard", "Reset pin to Wildcard"),
                LOCTEXT("ResetPinToWildcardTooltip", "Reset this pin's type to Wildcard."),
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateUObject(const_cast<UK2Node_DebugPrint*>(this), &UK2Node_DebugPrint::ResetPinToWildcard, const_cast<UEdGraphPin*>(Context->Pin)))
            );
        }
    }
}

FText UK2Node_DebugPrint::GetTooltipText() const
{
    return LOCTEXT("DebugPrintNode_Tooltip", "DebugPrintNode_Tooltip");
}

FLinearColor UK2Node_DebugPrint::GetNodeTitleColor() const
{
    return FLinearColor(1.f, 1.f, 0.f);
}

FText UK2Node_DebugPrint::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return LOCTEXT("DebugPrint", "Debug Print");
}

FSlateIcon UK2Node_DebugPrint::GetIconAndTint(FLinearColor& OutColor) const
{
    static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
    return Icon;
}

FName UK2Node_DebugPrint::GetCornerIcon() const
{
    return TEXT("Graph.Message.MessageIcon");
}

void UK2Node_DebugPrint::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UClass* ActionKey = GetClass();
    if (ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);

        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

FText UK2Node_DebugPrint::GetMenuCategory() const
{
    return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Development);
}

void UK2Node_DebugPrint::ResetPinToWildcard(UEdGraphPin* PinToReset)
{
    if (!PinToReset || PinToReset->LinkedTo.Num() > 0)
    {
        return;
    }

    // Open a transaction to support undo/redo
    FScopedTransaction Transaction(LOCTEXT("ResetPinToWildcardTx", "Reset Pin to Wildcard"));
    Modify();

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    
    // If the pin is a child, it needs to be recombined
    UEdGraphPin* ParentPin = PinToReset->ParentPin ? PinToReset->ParentPin : PinToReset;
    Schema->RecombinePin(PinToReset);
    PinToReset = ParentPin;

    // Change the pin type to wildcard
    PinToReset->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
    PinToReset->PinType.PinSubCategory = NAME_None;
    PinToReset->PinType.PinSubCategoryObject = nullptr;

    // Mark the Blueprint as modified to reflect the changes
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());

    // Rebuild the node for visual updates
    ReconstructNode();
}

#undef LOCTEXT_NAMESPACE
