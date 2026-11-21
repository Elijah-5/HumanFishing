#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HumanFishingEditorLibrary.generated.h"
class UBlueprint;
class USkeletalMesh;
UCLASS()
class HUMANFISHINGEDITOR_API UHumanFishingEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Human Fishing|Editor")
    static FString DescribeBlueprint(UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable, Category="Human Fishing|Editor")
    static bool PatchFishingBlueprint(UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable, Category="Human Fishing|Editor")
    static int32 ReplaceTemplateMesh(UBlueprint* Blueprint, USkeletalMesh* Mesh);
    UFUNCTION(BlueprintCallable,Category="Human Fishing|Editor")
    static bool HideLegacyHUD(UBlueprint* Blueprint);
};
