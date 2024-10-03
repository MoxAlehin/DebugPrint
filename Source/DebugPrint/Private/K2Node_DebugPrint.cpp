#include "K2Node_DebugPrint.h"

#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "EditorCategoryUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "EdGraphUtilities.h"
#include "GraphEditorSettings.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "K2Node"

class UGraphEditorSettings;

namespace
{
// static const FName LessPinName(TEXT("Less"));
}

class FKCHandler_DebugPrint : public FNodeHandlingFunctor
{
public:
    FKCHandler_DebugPrint(FKismetCompilerContext& InCompilerContext) : FNodeHandlingFunctor(InCompilerContext)
    {
    }

    // Мы убираем весь код, относящийся к BoolTermMap и RegisterNets, так как это не нужно для простого соединения Exec пинов.

    virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) override
    {
        // Приводим ноду к типу UK2Node_DebugPrint
        UK2Node_DebugPrint* DebugPrintNode = CastChecked<UK2Node_DebugPrint>(Node);

        // Получаем пин "Execute" (входной Exec пин)
        UEdGraphPin* ExecPin = DebugPrintNode->FindPin(UEdGraphSchema_K2::PN_Execute);
    
        // Получаем пин "Then" (выходной Exec пин)
        UEdGraphPin* ThenPin = DebugPrintNode->FindPin(UEdGraphSchema_K2::PN_Then);

        // Получаем входной Wildcard пин
        UEdGraphPin* WildcardPin = DebugPrintNode->FindPin(TEXT("InputValue"));

        // Проверка на наличие подключений
        if (!ExecPin || !ThenPin || ExecPin->LinkedTo.Num() == 0)
        {
            CompilerContext.MessageLog.Error(*LOCTEXT("ExecPinError", "Error: Exec pin not found or not connected").ToString(), DebugPrintNode);
            return;
        }

        // Проверяем, был ли пин "Wildcard" подключён, и если да, обрабатываем его
        if (WildcardPin && WildcardPin->LinkedTo.Num() > 0)
        {
            // Обрабатываем тип данных, связанный с Wildcard пином
            UEdGraphPin* LinkedPin = WildcardPin->LinkedTo[0];
        
            // Далее ты можешь добавить логику, которая обрабатывает входные данные
            // например, выводит их в лог или использует в дальнейших вычислениях
        }

        // Генерируем простую "NOP" инструкцию для соединения Exec пинов
        FBlueprintCompiledStatement& Statement = Context.AppendStatementForNode(DebugPrintNode);
        Statement.Type = KCST_Nop;

        // Добавляем Goto-фиксацию для выхода через пин "Then"
        Context.GotoFixupRequestMap.Add(&Statement, ThenPin);
    }
};

void UK2Node_DebugPrint::AllocateDefaultPins()
{
    // Execs
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

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
    // Вызов базовой реализации метода
    Super::ReallocatePinsDuringReconstruction(OldPins);

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

    // Уведомляем Blueprint редактор об изменениях
    GetGraph()->NotifyGraphChanged();
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

// FNodeHandlingFunctor* UK2Node_DebugPrint::CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const
// {
//     return new FKCHandler_DebugPrint(CompilerContext);
// }

#undef LOCTEXT_NAMESPACE
