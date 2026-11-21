#include "HumanFishingEditorLibrary.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Modules/ModuleManager.h"
#include "HumanFishingPlayerUI.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Self.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
IMPLEMENT_MODULE(FDefaultModuleImpl, HumanFishingEditor)

bool UHumanFishingEditorLibrary::HideLegacyHUD(UBlueprint* Blueprint)
{
    auto* Widget=Cast<UWidgetBlueprint>(Blueprint);
    if(!Widget || !Widget->WidgetTree || !Widget->WidgetTree->RootWidget)return false;
    Widget->WidgetTree->RootWidget->SetVisibility(ESlateVisibility::Collapsed);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);return true;
}

int32 UHumanFishingEditorLibrary::ReplaceTemplateMesh(UBlueprint* Blueprint, USkeletalMesh* Mesh)
{
    if (!Blueprint || !Mesh || !Blueprint->SimpleConstructionScript) return 0;
    int32 Count=0;
    for (auto* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        if (auto* Component=Cast<USkeletalMeshComponent>(Node->ComponentTemplate))
            if (auto* Existing=Component->GetSkeletalMeshAsset())
                if (Existing->GetPathName().StartsWith(TEXT("/Game/Characters/Mannequins/")))
                { Component->Modify(); Component->SetSkeletalMesh(Mesh); ++Count; }
    if (Count) { FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint); FKismetEditorUtilities::CompileBlueprint(Blueprint); }
    return Count;
}

FString UHumanFishingEditorLibrary::DescribeBlueprint(UBlueprint* Blueprint)
{
    TArray<TSharedPtr<FJsonValue>> GraphValues;
    if (!Blueprint) return TEXT("[]");
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    for (auto* Graph : Graphs)
    {
        auto G=MakeShared<FJsonObject>(); G->SetStringField(TEXT("graph"),Graph->GetName());
        TArray<TSharedPtr<FJsonValue>> Nodes;
        for (const auto& NodeRef : Graph->Nodes)
        {
            auto* Node = NodeRef.Get();
            auto N=MakeShared<FJsonObject>(); N->SetStringField(TEXT("id"),Node->NodeGuid.ToString());
            N->SetStringField(TEXT("name"),Node->GetName());
            N->SetStringField(TEXT("class"),Node->GetClass()->GetName());
            N->SetStringField(TEXT("title"),Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
            TArray<TSharedPtr<FJsonValue>> Pins;
            for(auto* Pin : Node->Pins)
            {
                auto P=MakeShared<FJsonObject>(); P->SetStringField(TEXT("name"),Pin->PinName.ToString());
                P->SetStringField(TEXT("direction"),Pin->Direction==EGPD_Input ? TEXT("in"):TEXT("out"));
                P->SetStringField(TEXT("type"),Pin->PinType.PinCategory.ToString());
                P->SetStringField(TEXT("default"),Pin->DefaultValue);
                TArray<TSharedPtr<FJsonValue>> Links;
                for(auto* Link : Pin->LinkedTo) Links.Add(MakeShared<FJsonValueString>(Link->GetOwningNode()->GetName()+TEXT(".")+Link->PinName.ToString()));
                P->SetArrayField(TEXT("links"),Links); Pins.Add(MakeShared<FJsonValueObject>(P));
            }
            N->SetArrayField(TEXT("pins"),Pins); Nodes.Add(MakeShared<FJsonValueObject>(N));
        }
        G->SetArrayField(TEXT("nodes"),Nodes); GraphValues.Add(MakeShared<FJsonValueObject>(G));
    }
    FString Result; auto Writer=TJsonWriterFactory<>::Create(&Result); FJsonSerializer::Serialize(GraphValues, Writer); return Result;
}


namespace
{
UEdGraphNode* NamedNode(UEdGraph* Graph,const TCHAR* Name)
{
    for(const auto& Node : Graph->Nodes) if(Node->GetName()==Name) return Node.Get();
    return nullptr;
}
UEdGraph* NamedGraph(UBlueprint* BP,const TCHAR* Name)
{
    for(auto& Graph : BP->FunctionGraphs) if(Graph->GetName()==Name) return Graph.Get();
    for(auto& Graph : BP->UbergraphPages) if(Graph->GetName()==Name) return Graph.Get();
    return nullptr;
}
void Wire(UEdGraphPin* A,UEdGraphPin* B)
{
    check(A && B);check(GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(A,B));
}
UK2Node_CallFunction* NativeCall(UEdGraph* G,const FName Function,int32 X,int32 Y)
{
    auto* N=NewObject<UK2Node_CallFunction>(G);
    G->AddNode(N,false,false);N->CreateNewGuid();N->FunctionReference.SetExternalMember(Function,UHumanFishingBlueprintLibrary::StaticClass());
    N->AllocateDefaultPins();N->NodePosX=X;N->NodePosY=Y;N->NodeComment=TEXT("HF v2 bait transaction");
    if(auto* Context=N->FindPin(TEXT("WorldContextObject")))
    {
        auto* Self=NewObject<UK2Node_Self>(G);G->AddNode(Self,false,false);Self->CreateNewGuid();Self->AllocateDefaultPins();
        Self->NodePosX=X-150;Self->NodePosY=Y+150;Wire(Self->FindPin(UEdGraphSchema_K2::PN_Self),Context);
    }
    return N;
}
void After(UEdGraphPin* Output,UK2Node_CallFunction* Call)
{
    const auto Previous=Output->LinkedTo;Output->BreakAllPinLinks();
    Wire(Output,Call->FindPin(UEdGraphSchema_K2::PN_Execute));
    for(auto* Pin:Previous)Wire(Call->FindPin(UEdGraphSchema_K2::PN_Then),Pin);
}
void Before(UEdGraphNode* Target,UK2Node_CallFunction* Call)
{
    auto* Input=Target->FindPin(UEdGraphSchema_K2::PN_Execute);const auto Previous=Input->LinkedTo;Input->BreakAllPinLinks();
    for(auto* Pin:Previous)Wire(Pin,Call->FindPin(UEdGraphSchema_K2::PN_Execute));
    Wire(Call->FindPin(UEdGraphSchema_K2::PN_Then),Input);
}
void Guard(UEdGraphPin* Output,UK2Node_CallFunction* Predicate,bool bImpure)
{
    const auto Previous=Output->LinkedTo;Output->BreakAllPinLinks();
    auto* Branch=NewObject<UK2Node_IfThenElse>(Output->GetOwningNode()->GetGraph());
    Branch->GetGraph()->AddNode(Branch,false,false);Branch->CreateNewGuid();Branch->AllocateDefaultPins();
    Branch->NodePosX=Predicate->NodePosX+260;Branch->NodePosY=Predicate->NodePosY;
    if(bImpure){ Wire(Output,Predicate->FindPin(UEdGraphSchema_K2::PN_Execute));Wire(Predicate->FindPin(UEdGraphSchema_K2::PN_Then),Branch->GetExecPin()); }
    else Wire(Output,Branch->GetExecPin());
    Wire(Predicate->GetReturnValuePin(),Branch->GetConditionPin());
    for(auto* Pin:Previous)Wire(Branch->GetThenPin(),Pin);
}
}

bool UHumanFishingEditorLibrary::PatchFishingBlueprint(UBlueprint* Blueprint)
{
    if(!Blueprint) return false;
    auto* Cast=NamedGraph(Blueprint,TEXT("CastBait"));auto* Hook=NamedGraph(Blueprint,TEXT("HookHuman"));
    auto* Reset=NamedGraph(Blueprint,TEXT("ResetFishing"));auto* Aim=NamedGraph(Blueprint,TEXT("BeginFishingAim"));
    auto* Events=NamedGraph(Blueprint,TEXT("EventGraph"));
    if(!Cast || !Hook || !Reset || !Aim || !Events) return false;
    for(const auto& N:Cast->Nodes) if(N->NodeComment==TEXT("HF v2 bait transaction")) return true;
    // Assert the known anchors before touching any connections. An upstream graph
    // redesign must be inspected rather than silently patched at the wrong place.
    if(!NamedNode(Cast,TEXT("K2Node_IfThenElse_1034")) || !NamedNode(Hook,TEXT("K2Node_CallFunction_1066")) ||
       !NamedNode(Events,TEXT("K2Node_Event_1184"))) return false;
    Blueprint->Modify();
    Guard(NamedNode(Cast,TEXT("K2Node_IfThenElse_1034"))->FindPin(UEdGraphSchema_K2::PN_Then),
        NativeCall(Cast,TEXT("ReserveFishingBait"),-600,-400),true);
    After(NamedNode(Hook,TEXT("K2Node_CallFunction_1066"))->FindPin(UEdGraphSchema_K2::PN_Then),
        NativeCall(Hook,TEXT("HookFishingBait"),-600,-400));
    auto* Finish=NativeCall(Reset,TEXT("FinishFishingBait"),-600,-400);
    Finish->FindPin(TEXT("bRetrieveBeforeHook"))->DefaultValue=TEXT("false");
    After(NamedNode(Reset,TEXT("K2Node_FunctionEntry_0"))->FindPin(UEdGraphSchema_K2::PN_Then),Finish);
    for(const TCHAR* Name : {TEXT("K2Node_CallFunction_1064"),TEXT("K2Node_CallFunction_1104"),TEXT("K2Node_CallFunction_1108")})
    {
        auto* Retrieve=NativeCall(Events,TEXT("FinishFishingBait"),-600,-600);
        Retrieve->FindPin(TEXT("bRetrieveBeforeHook"))->DefaultValue=TEXT("true");
        Before(NamedNode(Events,Name),Retrieve);
    }
    After(NamedNode(Events,TEXT("K2Node_Event_1184"))->FindPin(UEdGraphSchema_K2::PN_Then),
        NativeCall(Events,TEXT("PrepareFishingPause"),-600,-800));
    Guard(NamedNode(Aim,TEXT("K2Node_FunctionEntry_0"))->FindPin(UEdGraphSchema_K2::PN_Then),
        NativeCall(Aim,TEXT("CanUseFishingInput"),-600,-400),false);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FCompilerResultsLog Results;
    FKismetEditorUtilities::CompileBlueprint(Blueprint,EBlueprintCompileOptions::None,&Results);
    return Results.NumErrors==0;
}
