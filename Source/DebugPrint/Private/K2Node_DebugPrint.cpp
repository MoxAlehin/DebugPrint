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
    // Входной пин Exec
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    
    // Выходной пин Exec
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
    
    // Входной пин Wildcard (изначально без конкретного типа)
    UEdGraphPin* WildcardPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, TEXT("InputValue"));
    
    // Устанавливаем пин как wildcard
    WildcardPin->PinToolTip = TEXT("Wildcard input pin. Type will be determined by what is connected.");
    
    Super::AllocateDefaultPins();
}

void UK2Node_DebugPrint::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
    Super::NotifyPinConnectionListChanged(Pin);

    if (Pin->PinName == TEXT("InputValue") && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
    {
        // Если наш wildcard пин подключен к другому пину с типом, меняем тип на основе этого пина
        if (Pin->LinkedTo.Num() > 0)
        {
            // Получаем первый пин, к которому подключен наш Wildcard
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
}

void UK2Node_DebugPrint::GetNodeContextMenuActions(class UToolMenu* Menu,
    class UGraphNodeContextMenuContext* Context) const
{
    Super::GetNodeContextMenuActions(Menu, Context);
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
    return LOCTEXT("DebugPrint", "DebugPrint");
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
    return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::FlowControl);
}

FNodeHandlingFunctor* UK2Node_DebugPrint::CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const
{
    return new FKCHandler_DebugPrint(CompilerContext);
}

#undef LOCTEXT_NAMESPACE
