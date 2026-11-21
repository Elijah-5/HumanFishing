#include "HumanFishingSharkAnimation.h"
#include "HumanFishingShark.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimNodeSpaceConversions.h"
#include "Animation/AnimSequence.h"
#include "BoneControllers/AnimNode_ModifyBone.h"

namespace
{
struct FSharkAnimationProxy : FAnimInstanceProxy
{
    FAnimNode_SequencePlayer_Standalone Swim;
    FAnimNode_ConvertLocalToComponentSpace ToComponent;
    FAnimNode_ModifyBone Jaw;
    FAnimNode_ConvertComponentToLocalSpace ToLocal;
    UAnimSequence* Sequence=nullptr;
    float Rate=1.f,Open=0.f;
    explicit FSharkAnimationProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance){}
    virtual void Initialize(UAnimInstance* Instance) override
    {
        FAnimInstanceProxy::Initialize(Instance);
        Swim.SetLoopAnimation(true);
        ToComponent.LocalPose.SetLinkNode(&Swim);
        Jaw.ComponentPose.SetLinkNode(&ToComponent);
        Jaw.BoneToModify.BoneName=TEXT("Jaw");
        Jaw.RotationMode=BMM_Additive;Jaw.RotationSpace=BCS_BoneSpace;
        Jaw.TranslationMode=BMM_Ignore;Jaw.ScaleMode=BMM_Ignore;Jaw.Alpha=1.f;
        ToLocal.ComponentPose.SetLinkNode(&Jaw);
        FAnimationUpdateSharedContext Shared;
        ToLocal.Initialize_AnyThread(FAnimationInitializeContext(this,&Shared));
    }
    virtual void PreUpdate(UAnimInstance* Instance,float Delta) override
    {
        FAnimInstanceProxy::PreUpdate(Instance,Delta);
        Sequence=CastChecked<UHumanFishingSharkAnimation>(Instance)->SwimAnimation;
        if(const auto* Shark=Cast<AHumanFishingShark>(Instance->GetOwningActor()))
        {Rate=Shark->GetSwimAnimationRate();Open=Shark->GetJawOpen();}
    }
    virtual void CacheBones() override
    {ToLocal.CacheBones_AnyThread(FAnimationCacheBonesContext(this));}
    virtual void UpdateAnimationNode(const FAnimationUpdateContext& Context) override
    {
        Swim.SetSequence(Sequence);Swim.SetPlayRate(Rate);
        Jaw.Rotation=FRotator(0,0,Open*25.f);
        ToLocal.Update_AnyThread(Context);
    }
    virtual bool Evaluate(FPoseContext& Output) override
    {ToLocal.Evaluate_AnyThread(Output);return true;}
};
}
FAnimInstanceProxy* UHumanFishingSharkAnimation::CreateAnimInstanceProxy()
{return new FSharkAnimationProxy(this);}
void UHumanFishingSharkAnimation::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy)
{delete static_cast<FSharkAnimationProxy*>(Proxy);}
