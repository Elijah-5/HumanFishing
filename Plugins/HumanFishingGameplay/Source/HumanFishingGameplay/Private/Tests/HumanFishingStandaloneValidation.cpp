// Development-only end-to-end fixture. Never writes to the player's save profile.
#include "CoreMinimal.h"
#if !UE_BUILD_SHIPPING
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformFileManager.h"
#include "HumanFishingInventory.h"
#include "HumanFishingAbilities.h"
#include "HumanFishingSharkAnimation.h"
#include "HumanFishingPlayerUI.h"
#include "HumanFishingSave.h"
#include "HumanFishingShopMenu.h"
#include "HumanFishingShark.h"
#include "HumanFishingJellyshock.h"
#include "Components/BoxComponent.h"
#include "HumanFishingTravelZone.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "ShaderCompiler.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

namespace
{
struct FStandaloneValidation
{
    int32 Step=0;
    double Next=FPlatformTime::Seconds()+3, Started=FPlatformTime::Seconds();
    bool bFailed=false;
    bool bQuick=FParse::Param(FCommandLine::Get(),TEXT("HFQuickSmoke"));
    bool bReadmeCapture=FParse::Param(FCommandLine::Get(),TEXT("HFReadmeCapture"));
    double NextFrame=0;
    int32 DemoFrame=0;
    FString PendingScreenshot;
    double ScreenshotAt=0;
    TWeakObjectPtr<UHumanFishingShopMenu> Shop;
    TWeakObjectPtr<AHumanFishingShark> EquipmentShark;
    TWeakObjectPtr<AHumanFishingJellyshock> DisplayJelly;
    FTransform InitialTail;
    FVector ShotTargetPosition=FVector::ZeroVector;
    UWorld* World() const
    {
        if (GEngine) for (const auto& Context:GEngine->GetWorldContexts())
            if (Context.World() && Context.World()->IsGameWorld()) return Context.World();
        return nullptr;
    }
    void Check(bool Condition,const TCHAR* Message)
    {
        if (!Condition) { bFailed=true; UE_LOG(LogTemp,Error,TEXT("HF_STANDALONE_FAIL %s"),Message); }
        else UE_LOG(LogTemp,Display,TEXT("HF_STANDALONE_OK %s"),Message);
    }
    void Screenshot(const TCHAR* Name,double Delay=.75)
    {
        if(bReadmeCapture)return;
        PendingScreenshot=Name;
        if(GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
            PendingScreenshot.ReplaceInline(TEXT("1280"),*FString::FromInt(GEngine->GameViewport->Viewport->GetSizeXY().X));
        ScreenshotAt=FPlatformTime::Seconds()+Delay;
    }
    void CapturePending()
    {
        const FString Dir=FPaths::ProjectSavedDir()/TEXT("Validation/Images");
        IFileManager::Get().MakeDirectory(*Dir,true);
        FScreenshotRequest::RequestScreenshot(Dir/(PendingScreenshot+TEXT(".png")),true,false);
        if (GEngine->GameViewport && GEngine->GameViewport->Viewport)
        { auto Size=GEngine->GameViewport->Viewport->GetSizeXY(); UE_LOG(LogTemp,Display,TEXT("HF_CAPTURE %s %dx%d"),*PendingScreenshot,Size.X,Size.Y); }
        PendingScreenshot.Reset();
    }
    void CaptureDemo(double Now)
    {
        if(!bReadmeCapture || !bQuick || Now<NextFrame)return;
        const TCHAR* Clip=Step==2 || Step==3?TEXT("shop-expansion"):
            Step==6 || Step==7?TEXT("jellyshock-stun"):
            Step==9 || Step==10?TEXT("violet-jellyshock"):nullptr;
        if(!Clip)return;
        const FString Dir=FPaths::ProjectSavedDir()/TEXT("Validation/ReadmeFrames")/Clip;
        IFileManager::Get().MakeDirectory(*Dir,true);
        FScreenshotRequest::RequestScreenshot(Dir/FString::Printf(TEXT("%05d.png"),DemoFrame++),true,false);
        NextFrame=Now+.08;
    }
    AHumanFishingTravelZone* Zone(UWorld* W,const FName Destination)
    {
        for (TActorIterator<AHumanFishingTravelZone> It(W);It;++It)
            if (It->DestinationMap.GetAssetName()==Destination.ToString()) return *It;
        return nullptr;
    }
    void Enter(UWorld* W,APawn* Fish,FName Destination,bool bTravel)
    {
        auto* Z=Zone(W,Destination); Check(Z!=nullptr,TEXT("Destination has an explicit entrance"));
        if (!Z) return;
        Fish->SetActorLocation(Z->GetActorLocation());
        Check(!UHumanFishingBlueprintLibrary::CanUseFishingInput(Fish),TEXT("Entrance blocks fishing input"));
        if (bTravel) Z->Interact();
    }
    bool Tick(float)
    {
        const double Now=FPlatformTime::Seconds();
        if (Now-Started>600) { Check(false,TEXT("Fixture timed out")); Finish(); return false; }
        if(GShaderCompilingManager)
        {
            // Uncooked -game sessions do not have the editor's shader finalize tick.
            GShaderCompilingManager->ProcessAsyncResults(true,false);
        }
        if(!PendingScreenshot.IsEmpty() && Now>=ScreenshotAt)CapturePending();
        CaptureDemo(Now);
        if (Now<Next) return true;
        UWorld* W=World(); if (!W || !W->GetGameInstance()) return true;
        auto* PC=UGameplayStatics::GetPlayerController(W,0);
        auto* Fish=PC?PC->GetPawn():nullptr;
        auto* UI=Fish?Fish->FindComponentByClass<UHumanFishingPlayerUI>():nullptr;
        auto* I=W->GetGameInstance()->GetSubsystem<UHumanFishingInventory>();
        auto* Save=W->GetGameInstance()->GetSubsystem<UHumanFishingSaveSubsystem>();
        const FName Level(*UGameplayStatics::GetCurrentLevelName(W));
        if (!PC || !I || !Save) return true;
        if (bQuick)
        {
            if (!UI) return true;
            switch (Step)
            {
            case 0:
                I->SetSeaweed(200);
                UI->TryStoreCatch(TEXT("Farmer"));
                Fish->SetActorLocation(FVector(740,0,-850));PC->SetControlRotation(FRotator(0,180,0));
                if (auto* Class=LoadClass<UHumanFishingShopMenu>(nullptr,TEXT("/Game/HumanFishing/UI/WBP_Shop.WBP_Shop_C")))
                { Shop=CreateWidget<UHumanFishingShopMenu>(PC,Class);Shop->AddToViewport(100);PC->SetShowMouseCursor(true); }
                Check(Shop.IsValid(),TEXT("Shop opens for smoke check"));break;
            case 1:
                Check(UI->IsShopOpen(),TEXT("Shop owns input"));
                Screenshot(TEXT("ShopSmoke1280"));break;
            case 2:
                Check(I->BuyBackpackUpgrade() && I->GetColumns()==5,TEXT("Backpack expansion applies while shop is open"));
                Screenshot(TEXT("ShopExpanded1280"));break;
            case 3:
                FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Escape,FModifierKeysState(),0,false,0,0));
                Check(!UI->IsShopOpen(),TEXT("Escape closes the shop"));
                Check(I->BuyJellyshock(2) && I->BuyTidalFins(),TEXT("Buy defense capsules and unlock water sprint"));
                // A passing map shark must not intercept the fixture's target shot.
                for(TActorIterator<AHumanFishingShark> It(W);It;++It)It->Destroy();
                Fish->SetActorLocation(FVector(-800,800,-800));PC->SetControlRotation(FRotator(0,0,0));break;
            case 4:
                {
                    FVector View;FRotator Facing;PC->GetPlayerViewPoint(View,Facing);
                    EquipmentShark=W->SpawnActor<AHumanFishingShark>(Fish->GetActorLocation()+Facing.Vector()*700.f,FRotator::ZeroRotator);
                    Check(EquipmentShark.IsValid(),TEXT("Spawn animated shark"));
                    if(EquipmentShark.IsValid())
                    {EquipmentShark->ChaseSpeed=1.f;auto* Mesh=EquipmentShark->FindComponentByClass<USkeletalMeshComponent>();
                        InitialTail=Mesh->GetSocketTransform(TEXT("Tail"),RTS_Component);}
                    DisplayJelly=W->SpawnActor<AHumanFishingJellyshock>(View+Facing.Vector()*420.f+FRotationMatrix(Facing).GetUnitAxis(EAxis::Y)*100.f,FRotator(0,Facing.Yaw,0));
                    if(DisplayJelly.IsValid()){DisplayJelly->SetActorTickEnabled(false);DisplayJelly->SetLifeSpan(0);Screenshot(TEXT("JellyshockModel1280"));}
                }break;
            case 5:
                if(DisplayJelly.IsValid())DisplayJelly->Destroy();
                if(EquipmentShark.IsValid())
                {
                    auto* Mesh=EquipmentShark->FindComponentByClass<USkeletalMeshComponent>();
                    Check(Mesh->GetAnimInstance() && Mesh->GetAnimInstance()->IsA<UHumanFishingSharkAnimation>(),TEXT("Native swim and jaw graph active"));
                    Check(!Mesh->GetSocketTransform(TEXT("Tail"),RTS_Component).Equals(InitialTail,.001f),TEXT("Tail bones actually animate"));
                    FVector View;FRotator Facing;PC->GetPlayerViewPoint(View,Facing);
                    EquipmentShark->SetActorLocation(Fish->GetActorLocation()+Facing.Vector()*300.f);
                    Screenshot(TEXT("SharkJaw1280"));
                }break;
            case 6:
                Check(EquipmentShark.IsValid() && EquipmentShark->GetJawOpen()>.5f,TEXT("Shark opens its mouth near the fish"));
                if(EquipmentShark.IsValid())ShotTargetPosition=EquipmentShark->GetActorLocation();
                Check(UI->GetAbilities() && UI->GetAbilities()->FireJellyshock(),TEXT("Launch capsule from water ability"));
                Check(I->GetJellyshockCount()==1,TEXT("Launch consumes exactly one capsule"));
                Screenshot(TEXT("JellyshockBurst1280"),.3);break;
            case 7:
                Check(EquipmentShark.IsValid() && EquipmentShark->GetStunTimeRemaining()>20.f,TEXT("Projectile collision stuns shark"));
                if(EquipmentShark.IsValid())
                {
                    Check(EquipmentShark->GetActorLocation().Equals(ShotTargetPosition,2.f),TEXT("Stunned shark stays in place"));
                    FVector View;FRotator Facing;PC->GetPlayerViewPoint(View,Facing);
                    EquipmentShark->SetActorLocation(Fish->GetActorLocation()+Facing.Vector()*70.f);
                    Check(!UI->GetAbilities()->QueryJellyshockAim(true).bBlocked,TEXT("Point-blank shark is not treated as a wall"));
                    EquipmentShark->SetActorLocation(ShotTargetPosition);
                }
                Screenshot(TEXT("JellyshockStun1280"));break;
            case 8:
                if(EquipmentShark.IsValid())EquipmentShark->ApplyStun(.1f);
                {
                    FVector View;FRotator Facing;PC->GetPlayerViewPoint(View,Facing);
                    auto* Wall=W->SpawnActor<AActor>(Fish->GetActorLocation()+Facing.Vector()*55.f,FRotator::ZeroRotator);
                    auto* Collision=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(Collision);
                    Collision->SetBoxExtent(FVector(15,100,100));Collision->SetCollisionObjectType(ECC_WorldStatic);
                    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Collision->SetCollisionResponseToAllChannels(ECR_Block);
                    Collision->RegisterComponent();Collision->SetWorldLocation(Fish->GetActorLocation()+Facing.Vector()*55.f);
                    Check(UI->GetAbilities()->QueryJellyshockAim(true).bBlocked,TEXT("Obstructed muzzle detected"));
                    Check(!UI->GetAbilities()->FireJellyshock() && I->GetJellyshockCount()==1,TEXT("Blocked launch does not spend stock"));
                    Wall->Destroy();
                }break;
            case 9:
                Check(EquipmentShark.IsValid() && EquipmentShark->GetState()!=EHumanFishingSharkState::Stunned,TEXT("Shark recovers after stun expires"));
                Check(I->BuyVioletJellyshock(),TEXT("Buy premium violet capsule"));
                if(EquipmentShark.IsValid())
                {
                    FVector View;FRotator Facing;PC->GetPlayerViewPoint(View,Facing);
                    EquipmentShark->SetActorLocation(Fish->GetActorLocation()+Facing.Vector()*450.f);
                    EquipmentShark->SetActorTickEnabled(false);
                }
                Check(UI->GetAbilities()->FireVioletJellyshock(),TEXT("Launch violet capsule"));
                Check(I->GetVioletJellyshockCount()==0 && I->GetJellyshockCount()==1,TEXT("Violet shot consumes only violet stock"));
                Screenshot(TEXT("VioletJellyshockBurst1280"),.4);break;
            case 10:
                Check(!EquipmentShark.IsValid() || EquipmentShark->IsActorBeingDestroyed(),TEXT("Violet hit defeats shark"));
                {
                    FVector View;FRotator Facing;PC->GetPlayerViewPoint(View,Facing);
                    DisplayJelly=W->SpawnActor<AHumanFishingJellyshock>(View+Facing.Vector()*420.f+FRotationMatrix(Facing).GetUnitAxis(EAxis::Y)*100.f,FRotator(0,Facing.Yaw,0));
                    if(DisplayJelly.IsValid()){DisplayJelly->ConfigureVariant(true);DisplayJelly->SetActorTickEnabled(false);DisplayJelly->SetLifeSpan(0);Screenshot(TEXT("VioletJellyshockModel1280"));}
                }break;
            case 11:
                if(DisplayJelly.IsValid())DisplayJelly->Destroy();
                Finish();return false;
            }
            ++Step;Next=Now+3.;return true;
        }
        if (Step>1 && Step<16 && !UI) return true;
        if ((Step==2 && Level!=Save->ShoreLevel) || (Step==6 && Level!=Save->OceanLevel) ||
            (Step==13 && Level!=Save->CoveLevel) || (Step==15 && Level!=Save->OceanLevel) ||
            (Step==16 && Level!=Save->MenuLevel) || (Step==17 && Level!=Save->OceanLevel)) return true;
        UE_LOG(LogTemp,Display,TEXT("HF_STANDALONE_STEP %d %s"),Step,*Level.ToString());
        switch(Step)
        {
        case 0: Screenshot(TEXT("SaveMenu1280")); break;
        case 1: Check(Save->StartSlot(W,0,true),TEXT("Create isolated save slot")); break;
        case 2:
            I->ToggleBait();Check(!I->CanCastSelectedBait(),TEXT("New game bill stock is zero"));
            Check(!UHumanFishingBlueprintLibrary::ReserveFishingBait(Fish),TEXT("Empty cast rejected in game"));
            Screenshot(TEXT("EmptyBait1280")); break;
        case 3:
            Check(UI->TryStoreCatch(TEXT("BeachTourist")),TEXT("Capture enters backpack"));
            Check(I->RedeemCatches(UI->GetCatalog())>0,TEXT("Redeem catch for seaweed"));
            I->SetSeaweed(200);
            Check(I->BuyBait(EHumanFishingBait::HundredDollarBill,5),TEXT("Purchase five bills"));
            Check(I->BuyBait(EHumanFishingBait::Flash,2),TEXT("Purchase two Flash bait"));
            Check(UHumanFishingBlueprintLibrary::ReserveFishingBait(Fish),TEXT("Reserve through fishing interface"));
            UHumanFishingBlueprintLibrary::FinishFishingBait(Fish,true);
            Check(I->GetBaitCount(EHumanFishingBait::HundredDollarBill)==5,TEXT("Retrieve refunds in game"));
            Check(UHumanFishingBlueprintLibrary::ReserveFishingBait(Fish),TEXT("Reserve next cast"));
            UHumanFishingBlueprintLibrary::HookFishingBait(Fish);UHumanFishingBlueprintLibrary::FinishFishingBait(Fish,false);
            Check(I->GetBaitCount(EHumanFishingBait::HundredDollarBill)==4,TEXT("Hook consumes in game"));
            Screenshot(TEXT("BaitStock1280"));break;
        case 4: Enter(W,Fish,Save->OceanLevel,false);Screenshot(TEXT("ShoreEntrance1280"));break;
        case 5: Enter(W,Fish,Save->OceanLevel,true);break;
        case 6:
            Check(AHumanFishingTravelZone::FindNearby(Fish)==nullptr,TEXT("Ocean spawn outside entrances"));
            if (auto* Z=Zone(W,Save->CoveLevel))Check(Z->IsLocked(Fish),TEXT("Cove locked before purchase"));
            for(TActorIterator<AHumanFishingShark> It(W);It;++It)
            { auto* Mesh=It->FindComponentByClass<USkeletalMeshComponent>();Check(Mesh && Mesh->GetSkeletalMeshAsset() && Mesh->GetAnimInstance(),TEXT("Shark has animated skeletal appearance")); }
            Screenshot(TEXT("OceanArrival1280"));break;
        case 7:
            Check(UI->TryStoreCatch(TEXT("BeachTourist")) && UI->TryStoreCatch(TEXT("Farmer")),TEXT("Shop backpack displays held catches"));
            Fish->SetActorLocation(FVector(740,0,-850));PC->SetControlRotation(FRotator(0,180,0));
            if (auto* Class=LoadClass<UHumanFishingShopMenu>(nullptr,TEXT("/Game/HumanFishing/UI/WBP_Shop.WBP_Shop_C")))
            { Shop=CreateWidget<UHumanFishingShopMenu>(PC,Class);Shop->AddToViewport(100);PC->SetShowMouseCursor(true);PC->SetInputMode(FInputModeUIOnly()); }
            Check(Shop.IsValid(),TEXT("Shop widget opens"));break;
        case 8:
            Check(UI->IsShopOpen() && !UHumanFishingBlueprintLibrary::CanUseFishingInput(Fish),TEXT("Shop blocks fishing input"));
            Screenshot(TEXT("Shop1280"));
            Check(I->RedeemCatch(UI->GetCatalog(),I->GetCatches()[0].Id)>0 && I->GetCatches().Num()==1,TEXT("Sell selected catch keeps other catches"));
            Check(I->BuyCoveKey(),TEXT("Buy permanent Cove key"));
            { int32 Balance=I->GetSeaweed();Check(!I->BuyCoveKey() && I->GetSeaweed()==Balance,TEXT("Repeat key purchase rejected")); }break;
        case 9:
            Check(I->RedeemCatches(UI->GetCatalog())>0 && I->GetCatches().IsEmpty(),TEXT("Sell all redeems the remaining backpack"));
            PC->ConsoleCommand(TEXT("r.SetRes 1920x1080w"));break;
        case 10:Screenshot(TEXT("Shop1920"));break;
        case 11:
            FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Escape,FModifierKeysState(),0,false,0,0));
            Check(!UI->IsShopOpen(),TEXT("Escape closes shop through Slate keyboard input"));
            if (Shop.IsValid() && Shop->IsInViewport())Shop->CloseShop();
            Enter(W,Fish,Save->CoveLevel,false);Screenshot(TEXT("CoveUnlocked1920"));break;
        case 12:Enter(W,Fish,Save->CoveLevel,true);break;
        case 13:
            Check(I->HasCoveKey() && I->GetBaitCount(EHumanFishingBait::HundredDollarBill)==4,TEXT("Travel keeps key and bait stock"));
            Check(AHumanFishingTravelZone::FindNearby(Fish)==nullptr,TEXT("Cove spawn outside entrance"));
            Screenshot(TEXT("ScenicCove1920"));break;
        case 14:Enter(W,Fish,Save->OceanLevel,true);break;
        case 15:
            Check(Fish->GetActorLocation().X< -1800,TEXT("Cove return preserves Arrival spawn tag"));
            Check(Save->Flush() && Save->ReturnToMenu(W),TEXT("Save and return to menu"));break;
        case 16:Check(Save->StartSlot(W,0,false),TEXT("Continue saved slot"));break;
        case 17:
            Check(I->HasCoveKey() && I->GetBaitCount(EHumanFishingBait::HundredDollarBill)==4 && I->GetBaitCount(EHumanFishingBait::Flash)==2,TEXT("Continue restores key and both bait stocks"));
            Check(I->GetLifetimeCatchCount()==3 && I->GetCatches().IsEmpty(),TEXT("Continue preserves redeemed capture history"));
            Screenshot(TEXT("Continue1920"));break;
        case 18:Finish();return false;
        }
        ++Step;Next=Now+3.;return true;
    }
    void Finish()
    {
        const FString Result=bFailed?TEXT("FAIL"):TEXT("PASS");
        FFileHelper::SaveStringToFile(Result,*(FPaths::ProjectSavedDir()/(bQuick?TEXT("Validation/SmokeResult.txt"):TEXT("Validation/StandaloneResult.txt"))));
        UE_LOG(LogTemp,Display,TEXT("HF_STANDALONE_RESULT %s"),*Result);
        FPlatformMisc::RequestExit(false);
    }
};
TSharedPtr<FStandaloneValidation> Validation;
FAutoConsoleCommand ValidateCommand(TEXT("hf.validate"),TEXT("Run isolated end-to-end validation; requires -HFValidation -UserDir=<...humanfishing-validation-profile>."),
    FConsoleCommandDelegate::CreateLambda([]
    {
        FString UserDir;
        if(!FParse::Param(FCommandLine::Get(),TEXT("HFValidation")) ||
           !FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir) || !UserDir.Contains(TEXT("humanfishing-validation-profile")))
        {UE_LOG(LogTemp,Warning,TEXT("Validation requires a disposable profile; player saves were not touched."));return;}
        if(Validation.IsValid())return;
        Validation=MakeShared<FStandaloneValidation>();
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float DT){return Validation->Tick(DT);}),
            Validation->bReadmeCapture?.01f:.25f);
    }));
}
#endif
