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

#define LOCTEXT_NAMESPACE "K2Node"

class UK2Node_MakeArray;
class UGraphEditorSettings;

void UK2Node_DebugPrint::ArrayDebugPrint(
    const UObject* WorldContextObject,
    const TArray<FString>& Values,
    bool bReplace,
    FName Key,
    const FString& Separator,
    bool bNewLine,
    bool bPrintLabel,
    bool bPrintToScreen,
    bool bPrintToLog,
    FLinearColor TextColor,
    float Duration,
    const FString& NodeGUIDString)
{
    if (bNewLine)
    {
        for (int32 i = 0; i < Values.Num(); ++i)
        {
            FString Value = Values[i];
            FName UniqueKey = Key;
            
            // Если Replace == true, используем GUID с индексом
            if (bReplace)
            {
                FString UniqueKeyString = FString::Printf(TEXT("%s_%d"), *NodeGUIDString, i);
                UniqueKey = FName(*UniqueKeyString);
            }

            // Печатаем каждое значение по отдельности
            UKismetSystemLibrary::PrintString(
                WorldContextObject,
                Value,
                bPrintToLog,
                bPrintToScreen,
                TextColor,
                Duration,
                UniqueKey
            );
        }
    }
    else
    {
        // 1. Собираем строку через Join String Array
        FString InString = FString::Join(Values, *Separator);

        // 2. Если Replace == true, конвертируем GUID в строку и в Name
        if (bReplace)
        {
            Key = FName(NodeGUIDString);
        }

        // Используем UKismetSystemLibrary::PrintString для вывода на экран и в лог
        UKismetSystemLibrary::PrintString(
            WorldContextObject,
            InString,
            bPrintToLog,
            bPrintToScreen,
            TextColor,
            Duration,
            Key
        );
    }
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

void UK2Node_DebugPrint::AllocateDefaultPins()
{
    // Execs
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

    for(int32 i = 0; i < UserDefinedPins.Num(); i++)
    {
        FText DummyErrorMsg;
        if ((!IsEditable() || CanCreateUserDefinedPin(UserDefinedPins[i]->PinType, UserDefinedPins[i]->DesiredPinDirection, DummyErrorMsg)) && !FindPin(UserDefinedPins[i]->PinName))
        {
            UserDefinedPins.RemoveAt(i);
            NumValuePins++;
        }
    }
    
    // Value Wildcard Pins
    for (int32 i = 0; i < NumValuePins; i++)
    {
        FName PinName = FName(*FString::Printf(TEXT("Value_%d"), i));
        UEdGraphPin* WildcardPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, PinName);
        WildcardPin->PinToolTip = TEXT("Wildcard input pin. Type will be determined by what is connected.");
    }

    // Print String Options
    UEdGraphPin* ReplacePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, TEXT("bReplace"));
    ReplacePin->bAdvancedView = true;
    ReplacePin->DefaultValue = TEXT("true");
    ReplacePin->PinToolTip = TEXT("If true, the node will replace any existing on-screen messages, similar to using a non-empty key, but without the need to specify one.");

    UEdGraphPin* KeyPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, TEXT("Key"));
    KeyPin->bAdvancedView = true;
    KeyPin->DefaultValue = TEXT("None");
    KeyPin->PinToolTip = TEXT("If a non-empty key is provided, the message will replace any existing on-screen messages with the same key.");

    UEdGraphPin* SeparatorPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, TEXT("Separator"));
    SeparatorPin->bAdvancedView = true;
    SeparatorPin->DefaultValue = TEXT(" ");
    SeparatorPin->PinToolTip = TEXT("The string used to separate each value.");

    UEdGraphPin* NewLinePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, TEXT("bNewLine"));
    NewLinePin->bAdvancedView = true;
    NewLinePin->DefaultValue = TEXT("true");
    NewLinePin->PinToolTip = TEXT("Whether or not to print each value on a new line.");

    UEdGraphPin* PrintToScreenPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, TEXT("bPrintToScreen"));
    PrintToScreenPin->bAdvancedView = true;
    PrintToScreenPin->DefaultValue = TEXT("true");
    PrintToScreenPin->PinToolTip = TEXT("Whether or not to print the output to the screen.");

    UEdGraphPin* PrintToLogPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, TEXT("bPrintToLog"));
    PrintToLogPin->bAdvancedView = true;
    PrintToLogPin->DefaultValue = TEXT("true");
    PrintToLogPin->PinToolTip = TEXT("Whether or not to print the output to the log.");

    UEdGraphPin* TextColorPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, TEXT("TextColor"));
    TextColorPin->PinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
    TextColorPin->bAdvancedView = true;
    TextColorPin->DefaultValue = TEXT("(R=0.500000,G=0.000000,B=0.500000,A=1.000000)");
    TextColorPin->PinToolTip = TEXT("The color of the text to display.");

    UEdGraphPin* DurationPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, TEXT("Duration"));
    DurationPin->bAdvancedView = true;
    DurationPin->DefaultValue = TEXT("2.0");
    DurationPin->PinToolTip = TEXT("The display duration (if Print to Screen is True). Using negative number will result in loading the duration time from the config.");

    if (AdvancedPinDisplay == ENodeAdvancedPins::NoPins)
        AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
    Super::AllocateDefaultPins();
}

void UK2Node_DebugPrint::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
    Super::NotifyPinConnectionListChanged(Pin);

    // Проверяем, что пин является Wildcard и имеет подключение
    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
    {
        if (Pin->LinkedTo.Num() > 0)
        {
            // Получаем первый пин, к которому подключён наш Wildcard
            UEdGraphPin* LinkedPin = Pin->LinkedTo[0];
            
            // Устанавливаем тип данных пина в соответствии с подключенным пином
            Pin->PinType = LinkedPin->PinType;
        }
        else
        {
            // Если связь разорвана, возвращаем тип Wildcard
            Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
        }
    }
    ReconstructNode();
}

void UK2Node_DebugPrint::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
    bool bIsErrorFree = true;

    // 1. Создаем промежуточную ноду MakeArray
    UK2Node_MakeArray* MakeArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
    TArray<UEdGraphPin*> ValuePins = GetValuePins();
    MakeArrayNode->NumInputs = ValuePins.Num();
    MakeArrayNode->AllocateDefaultPins();
    
    // 2. Создаем промежуточную ноду для вызова функции ArrayDebugPrint
    UK2Node_CallFunction* DebugPrintNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    DebugPrintNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UK2Node_DebugPrint, ArrayDebugPrint), UK2Node_DebugPrint::StaticClass());
    DebugPrintNode->AllocateDefaultPins();
    
    // 3. Получаем пины функции ArrayDebugPrint
    UEdGraphPin* ValuesPin = DebugPrintNode->FindPin(TEXT("Values"));
    UEdGraphPin* ReplacePin = DebugPrintNode->FindPin(TEXT("bReplace"));
    UEdGraphPin* KeyPin = DebugPrintNode->FindPin(TEXT("Key"));
    UEdGraphPin* SeparatorPin = DebugPrintNode->FindPin(TEXT("Separator"));
    UEdGraphPin* NewLinePin = DebugPrintNode->FindPin(TEXT("bNewLine"));
    UEdGraphPin* PrintToScreenPin = DebugPrintNode->FindPin(TEXT("bPrintToScreen"));
    UEdGraphPin* PrintToLogPin = DebugPrintNode->FindPin(TEXT("bPrintToLog"));
    UEdGraphPin* TextColorPin = DebugPrintNode->FindPin(TEXT("TextColor"));
    UEdGraphPin* DurationPin = DebugPrintNode->FindPin(TEXT("Duration"));
    UEdGraphPin* NodeGuidPin = DebugPrintNode->FindPin(TEXT("NodeGUIDString"));

    // 4. Соединяем пин с результатом MakeArray с пином Values
    bIsErrorFree &= Schema->TryCreateConnection(MakeArrayNode->GetOutputPin(), ValuesPin);
    
    // 5. Соединяем Value пины с пинами MakeArray
    TArray<UEdGraphPin*> MakeArrayPins;
    TArray<UEdGraphPin*> MakeArrayOutputPins;
    MakeArrayNode->GetKeyAndValuePins(MakeArrayPins, MakeArrayOutputPins);

    for (int32 i = 0; i < ValuePins.Num(); i++)
    {
        // bIsErrorFree &= Schema->TryCreateConnection(ValuePins[i], MakeArrayPins[i]);
        bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*ValuePins[i], *MakeArrayPins[i]).CanSafeConnect();;
    }

    // 6. Подключаем exec пины
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *DebugPrintNode->GetExecPin()).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *DebugPrintNode->GetThenPin()).CanSafeConnect();

    // Подключаем остальные параметры
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("bReplace")), *ReplacePin).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("Key")), *KeyPin).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("Separator")), *SeparatorPin).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("bNewLine")), *NewLinePin).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("bPrintToScreen")), *PrintToScreenPin).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("bPrintToLog")), *PrintToLogPin).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("TextColor")), *TextColorPin).CanSafeConnect();
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("Duration")), *DurationPin).CanSafeConnect();

    // Устанавливаем значение NodeGUID как значение по умолчанию для соответствующего пина
    NodeGuidPin->DefaultValue = NodeGuid.ToString();

    // bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*PrintStringThenPin, *ThenPin).CanSafeConnect(); // Connecting default allocated pins
    // bIsErrorFree &= Schema->TryCreateConnection(PrintStringThenPin, ThenPin); // Connecting only inner Node with each other
    
    if (!bIsErrorFree)
    {
        CompilerContext.MessageLog.Error(*LOCTEXT("InternalConnectionError", "IsValidNode: Internal connection error. @@").ToString(), this);
    }
    
    BreakAllNodeLinks();
}

void UK2Node_DebugPrint::AddInputPin()
{
    Modify();
    
    NumValuePins++;
    
    ReconstructNode();
}

//////////////////////////////////////////////////////////////////////////////////////
// VISUALS
//////////////////////////////////////////////////////////////////////////////////////

bool UK2Node_DebugPrint::CanAddPin() const
{
    return true;  // Нода поддерживает кнопку "Add Pin"
}

void UK2Node_DebugPrint::RemoveInputPin(UEdGraphPin* PinToRemove)
{
    if (!PinToRemove || !Pins.Contains(PinToRemove))
    {
        return;
    }

    // Открываем транзакцию для поддержки undo/redo
    FScopedTransaction Transaction(LOCTEXT("RemovePinTx", "Remove Pin"));
    Modify();

    // Разрываем все соединения перед удалением пина
    PinToRemove->BreakAllPinLinks();

    // Удаляем пин из массива
    Pins.Remove(PinToRemove);

    // Уменьшаем количество пинов
    NumValuePins--;

    // Пересчитываем имена оставшихся пинов
    int32 Index = 0;
    for (UEdGraphPin* Pin : Pins)
    {
        if (Pin->PinName.ToString().StartsWith(TEXT("Value_")))
        {
            // Переименовываем оставшиеся пины
            Pin->Modify();
            Pin->PinName = FName(*FString::Printf(TEXT("Value_%d"), Index));
            Index++;
        }
    }

    // Перестраиваем ноду
    ReconstructNode();

    // Уведомляем Blueprint о том, что нода была изменена
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UK2Node_DebugPrint::PostReconstructNode()
{
    // Восстановление типов данных на основе подключений
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
    // Очищаем существующие пины
    Pins.Empty();

    // Повторное создание всех пинов
    AllocateDefaultPins();

    // Сопоставляем старые пины с новыми и восстанавливаем их типы данных
    for (UEdGraphPin* OldPin : OldPins)
    {
        UEdGraphPin* NewPin = FindPin(OldPin->PinName);
        if (NewPin && NewPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
        {
            // Если это wildcard пин, восстанавливаем его тип
            NewPin->PinType = OldPin->PinType;

            // Восстанавливаем соединения
            for (UEdGraphPin* LinkedPin : OldPin->LinkedTo)
            {
                NewPin->MakeLinkTo(LinkedPin);
            }
        }
    }
    
    RestoreSplitPins(OldPins);

    // Уведомляем Blueprint редактор об изменениях
    GetGraph()->NotifyGraphChanged();
}

UEdGraphPin* UK2Node_DebugPrint::CreatePinFromUserDefinition(const TSharedPtr<FUserPinInfo> NewPinInfo)
{
    // Создаем новый Wildcard пин с типом, указанным в UserPinInfo
    UEdGraphPin* NewPin = CreatePin(EGPD_Input, NewPinInfo->PinType, FName(*FString::Printf(TEXT("Value_%d"), NumValuePins)));

    // Устанавливаем значение по умолчанию для пина
    // const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
    // K2Schema->SetPinAutogeneratedDefaultValue(NewPin, NewPinInfo->PinDefaultValue);
    // ReconstructNode();
    // Увеличиваем счетчик Value пинов (например, NumValuePins)
    // NumValuePins++;

    // Возвращаем новый пин
    return NewPin;
}

bool UK2Node_DebugPrint::CanCreateUserDefinedPin(const FEdGraphPinType& InPinType,
    EEdGraphPinDirection InDesiredDirection, FText& OutErrorMessage)
{
    // Запрещаем добавление Output пинов
    if (InDesiredDirection == EGPD_Output)
    {
        OutErrorMessage = LOCTEXT("OutputPinNotAllowed", "Cannot add Output pins!");
        return false;
    }

    // Запрещаем добавление Exec пинов
    if (InPinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
    {
        OutErrorMessage = LOCTEXT("ExecPinNotAllowed", "Cannot add Exec pins!");
        return false;
    }
        
    // Запрещаем добавление Map, Array и Set пинов
    if (InPinType.IsContainer())
    {
        OutErrorMessage = LOCTEXT("ContainerTypeNotAllowed", "Cannot add Map, Array or Set pins!");
        return false;
    }

    // По умолчанию разрешаем добавление пина
    return true;
}

void UK2Node_DebugPrint::ChangePinType(UEdGraphPin* Pin)
{
}

bool UK2Node_DebugPrint::CanChangePinType(UEdGraphPin* Pin) const
{
    // Разрешаем изменение типа только для Value пинов
    // return Pin && Pin->PinName.ToString().StartsWith(TEXT("Value_"));
    return true;
}

TArray<UEdGraphPin*> UK2Node_DebugPrint::GetValuePins() const
{
    TArray<UEdGraphPin*> ValuePins;

    for (UEdGraphPin* Pin : Pins)
    {
        // Проверяем, является ли пин "Value_" пином
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

    // Проверяем, является ли пин пином значений (Value)
    if (Context && Context->Pin && Context->Pin->PinName.ToString().StartsWith(TEXT("Value_")))
    {
        // Добавляем действие "Remove Pin"
        FToolMenuSection& Section = Menu->AddSection("K2NodeDebugPrint", LOCTEXT("RemovePinHeader", "Remove Pin"));
        Section.AddMenuEntry(
            "RemovePin",
            LOCTEXT("RemovePin", "Remove Pin"),
            LOCTEXT("RemovePinTooltip", "Remove this input pin."),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateUObject(const_cast<UK2Node_DebugPrint*>(this), &UK2Node_DebugPrint::RemoveInputPin, const_cast<UEdGraphPin*>(Context->Pin)))
        );

        // Добавляем действие "Reset to Wildcard" только если пин не подключен
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

    // Открываем транзакцию для поддержки undo/redo
    FScopedTransaction Transaction(LOCTEXT("ResetPinToWildcardTx", "Reset Pin to Wildcard"));
    Modify();

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    
    // Если пин дочерний то его нужно Recombine
    UEdGraphPin* ParentPin = PinToReset->ParentPin ? PinToReset->ParentPin : PinToReset;
    Schema->RecombinePin(PinToReset);
    PinToReset = ParentPin;

    // Меняем тип пина на Wildcard
    PinToReset->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
    PinToReset->PinType.PinSubCategory = NAME_None;
    PinToReset->PinType.PinSubCategoryObject = nullptr;

    // Обновляем Blueprint, чтобы отразить изменения
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());

    // Перестраиваем ноду для визуального обновления
    ReconstructNode();
}

#undef LOCTEXT_NAMESPACE
