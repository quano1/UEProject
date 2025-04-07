// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Execution/TLLRN_RigUnit_InverseExecution.h"
#include "Units/Execution/TLLRN_RigUnit_Hierarchy.h"

#include "SkeletalDebugRendering.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "AnimCustomInstanceHelper.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "SceneManagement.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"
#include "TLLRN_ControlRig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigComponent)

// CVar to disable control rig execution within a component
static TAutoConsoleVariable<int32> CVarTLLRN_ControlRigDisableExecutionComponent(TEXT("TLLRN_ControlRig.DisableExecutionInComponent"), 0, TEXT("if nonzero we disable the execution of Control Rigs inside a TLLRN_ControlRigComponent."));

FTLLRN_ControlRigAnimInstanceProxy* FTLLRN_ControlRigComponentMappedElement::GetAnimProxyOnGameThread() const
{
	if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(SceneComponent))
	{
		if (UTLLRN_ControlRigAnimInstance* AnimInstance = Cast<UTLLRN_ControlRigAnimInstance>(SkeletalMeshComponent->GetAnimInstance()))
		{
			return AnimInstance->GetTLLRN_ControlRigProxyOnGameThread();
		}
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
TMap<FString, TSharedPtr<SNotificationItem>> UTLLRN_ControlRigComponent::EditorNotifications;
#endif

struct FSkeletalMeshToMap
{
	TWeakObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
	TArray<FTLLRN_ControlRigComponentMappedBone> Bones;
	TArray<FTLLRN_ControlRigComponentMappedCurve> Curves;
};

FCriticalSection gPendingSkeletalMeshesLock;
TMap<TWeakObjectPtr<UTLLRN_ControlRigComponent>, TArray<FSkeletalMeshToMap> > gPendingSkeletalMeshes;

UTLLRN_ControlRigComponent::UTLLRN_ControlRigComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	bTickInEditor = true;
	bAutoActivate = true;

	TLLRN_ControlRig = nullptr;
	bResetTransformBeforeTick = true;
	bResetInitialsBeforeConstruction = true;
	bUpdateRigOnTick = true;
	bUpdateInEditor = true;
	bDrawBones = true;
	bShowDebugDrawing = false;
	bIsInsideInitializeBracket = false;
	bWantsInitializeComponent = true;
	
	bEnableLazyEvaluation = false;
	LazyEvaluationPositionThreshold = 0.1f;
	LazyEvaluationRotationThreshold = 0.5f;
	LazyEvaluationScaleThreshold = 0.01f;
	bNeedsEvaluation = true;

	bNeedToInitialize = false;
}

#if WITH_EDITOR
void UTLLRN_ControlRigComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigComponent, TLLRN_ControlRigClass))
	{
		TLLRN_ControlRig = nullptr;
		SetupTLLRN_ControlRigIfRequired();
	}
	else if (PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigComponent, MappedElements))
	{
		ValidateMappingData();
	}
}

void UTLLRN_ControlRigComponent::PostLoad()
{
	Super::PostLoad();
}
#endif

void UTLLRN_ControlRigComponent::BeginDestroy()
{
	Super::BeginDestroy();

	FScopeLock Lock(&gPendingSkeletalMeshesLock);
	gPendingSkeletalMeshes.Remove(this);
}

void UTLLRN_ControlRigComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// after compile, we have to reinitialize
	// because it needs new execution code
	// since memory has changed, similar to FAnimNode_TLLRN_ControlRig::PostSerialize
	if (Ar.IsObjectReferenceCollector())
	{
		if (TLLRN_ControlRig)
		{
			TLLRN_ControlRig->Initialize();
		}
	}
}

void UTLLRN_ControlRigComponent::OnRegister()
{
	Super::OnRegister();
	
	TLLRN_ControlRig = nullptr;

	{
		FScopeLock Lock(&gPendingSkeletalMeshesLock);
		gPendingSkeletalMeshes.FindOrAdd(this);
	}

	// call to Initialize() should be delayed until TickComponent
	
	// Initialize() here directly is not enough because
	// in case of PIE, OnRegister() is called before BP native events like
	// OnPreInitialize are bound, which does not happen until
	// 1. Re-Registration ( Sequencer / Level Editor / non- PIE use cases, and there is no way to detect it here)
	// 2. InitializeComponent() (PIE + Cooked build only, not called for editor use cases)

	// so to avoid calling Initialize() in both OnRegister() and InitializeComponent()
	// we have to delay Initialize() until TickComponent, which is called all the time
	// in both editor + cooked.

	bNeedToInitialize = true;

	if (AActor* Actor = GetOwner())
	{
		Actor->PrimaryActorTick.bStartWithTickEnabled = true;
		Actor->PrimaryActorTick.bCanEverTick = true;
		Actor->PrimaryActorTick.bTickEvenWhenPaused = true;
	}
}

void UTLLRN_ControlRigComponent::OnUnregister()
{
	Super::OnUnregister();

	bool bBeginDestroyed = URigVMHost::IsGarbageOrDestroyed(this);
	if (!bBeginDestroyed)
	{
		if (AActor* Actor = GetOwner())
		{
			bBeginDestroyed = URigVMHost::IsGarbageOrDestroyed(Actor);
		}
	}

	if (!bBeginDestroyed)
	{
		for (TPair<USkeletalMeshComponent*, FCachedSkeletalMeshComponentSettings>& Pair : CachedSkeletalMeshComponentSettings)
		{
			if (Pair.Key)
			{
				if (Pair.Key->IsValidLowLevel() &&
					!URigVMHost::IsGarbageOrDestroyed(Pair.Key) &&
					IsValid(Pair.Key) &&
					!Pair.Key->IsUnreachable())
				{
					Pair.Value.Apply(Pair.Key);
				}
			}
		}
	}
	else
	{
		FScopeLock Lock(&gPendingSkeletalMeshesLock);
		gPendingSkeletalMeshes.Remove(this);
	}

	CachedSkeletalMeshComponentSettings.Reset();
}

void UTLLRN_ControlRigComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if (bNeedToInitialize)
	{
		Initialize();
		bNeedToInitialize = false;
	}
	
	if(!bUpdateRigOnTick)
	{
		return;
	}

	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		FScopeLock Lock(&gPendingSkeletalMeshesLock);
		TArray<FSkeletalMeshToMap>* PendingSkeletalMeshes = gPendingSkeletalMeshes.Find(this);

		if (PendingSkeletalMeshes != nullptr && PendingSkeletalMeshes->Num() > 0)
		{
			for (const FSkeletalMeshToMap& SkeletalMeshToMap : *PendingSkeletalMeshes)
			{
				AddMappedSkeletalMesh(
					SkeletalMeshToMap.SkeletalMeshComponent.Get(),
					SkeletalMeshToMap.Bones,
					SkeletalMeshToMap.Curves
				);
			}

			PendingSkeletalMeshes->Reset();
		}
	}

	Update(DeltaTime);
}

FPrimitiveSceneProxy* UTLLRN_ControlRigComponent::CreateSceneProxy()
{
	return new FTLLRN_ControlRigSceneProxy(this);
}

FBoxSphereBounds UTLLRN_ControlRigComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBox BBox(ForceInit);

	if (TLLRN_ControlRig)
	{
		// Get bounding box for the debug drawings if they are drawn 
		if (bShowDebugDrawing)
		{ 
			const FRigVMDrawInterface& DrawInterface = TLLRN_ControlRig->GetDrawInterface();

			for (int32 InstructionIndex = 0; InstructionIndex < DrawInterface.Num(); InstructionIndex++)
			{
				const FRigVMDrawInstruction& Instruction = DrawInterface[InstructionIndex];

				FTransform Transform = Instruction.Transform * GetComponentToWorld();
				for (const FVector& Position : Instruction.Positions)
				{
					BBox += Transform.TransformPosition(Position);
				}
			}
		}

		const FTransform Transform = GetComponentToWorld();

		// Get bounding box for bones
		if (UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
		{
			Hierarchy->ForEach<FTLLRN_RigTransformElement>([&BBox, Transform, Hierarchy](FTLLRN_RigTransformElement* TransformElement) -> bool
			{
				BBox += Transform.TransformPosition(Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::CurrentGlobal).GetLocation());
				return true;
			});
		}
	}

	if (BBox.IsValid)
	{
		// Points are in world space, so no need to transform.
		return FBoxSphereBounds(BBox);
	}
	else
	{
		const FVector BoxExtent(1.f);
		return FBoxSphereBounds(LocalToWorld.GetLocation(), BoxExtent, 1.f);
	}
}


UTLLRN_ControlRig* UTLLRN_ControlRigComponent::GetTLLRN_ControlRig()
{
	return SetupTLLRN_ControlRigIfRequired();
}

bool UTLLRN_ControlRigComponent::CanExecute()
{
	if(CVarTLLRN_ControlRigDisableExecutionComponent->GetInt() != 0)
	{
		return false;
	}
	
	if(UTLLRN_ControlRig* CR = GetTLLRN_ControlRig())
	{
		return CR->CanExecute();
	}
	
	return false;
}

float UTLLRN_ControlRigComponent::GetAbsoluteTime() const
{
	if(TLLRN_ControlRig)
	{
		return TLLRN_ControlRig->AbsoluteTime;
	}
	return 0.f;
}

void UTLLRN_ControlRigComponent::OnPreInitialize_Implementation(UTLLRN_ControlRigComponent* Component)
{
	OnPreInitializeDelegate.Broadcast(Component);
}

void UTLLRN_ControlRigComponent::OnPostInitialize_Implementation(UTLLRN_ControlRigComponent* Component)
{
	ValidateMappingData();
	OnPostInitializeDelegate.Broadcast(Component);
}

void UTLLRN_ControlRigComponent::OnPreConstruction_Implementation(UTLLRN_ControlRigComponent* Component)
{
	OnPreConstructionDelegate.Broadcast(Component);
}

void UTLLRN_ControlRigComponent::OnPostConstruction_Implementation(UTLLRN_ControlRigComponent* Component)
{
	OnPostConstructionDelegate.Broadcast(Component);
}

void UTLLRN_ControlRigComponent::OnPreForwardsSolve_Implementation(UTLLRN_ControlRigComponent* Component)
{
	OnPreForwardsSolveDelegate.Broadcast(Component);
}

void UTLLRN_ControlRigComponent::OnPostForwardsSolve_Implementation(UTLLRN_ControlRigComponent* Component)
{
	OnPostForwardsSolveDelegate.Broadcast(Component);
}

void UTLLRN_ControlRigComponent::Initialize()
{
	if(bIsInsideInitializeBracket)
	{
		return;
	}

	TGuardValue<bool> InitializeBracket(bIsInsideInitializeBracket, true);
	
	if (!UserDefinedElements.IsEmpty())
	{
		MappedElements = UserDefinedElements;
	}
	ValidateMappingData();

#if WITH_EDITOR
	if (bUpdateInEditor)
	{
		FEditorScriptExecutionGuard AllowScripts;
		OnPreInitialize(this);
	}
	else
#endif
	{
		OnPreInitialize(this);
	}
	
	if(UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if (CR->IsInitializing())
		{
			ReportError(TEXT("Initialize is being called recursively."));
		}
		else
		{
			CR->DrawInterface.Reset();
			CR->GetHierarchy()->ResetPoseToInitial(ETLLRN_RigElementType::All);
			CR->RequestInit();
		}
	}

	// we want to make sure all components driven by control rig component tick
	// after the control rig component such that by the time they tick they can
	// send the latest data for rendering
	// also need to make sure all components driving the control rig component tick before
	// the control rig component ticks
	TMap<TObjectPtr<USceneComponent>, ETLLRN_ControlRigComponentMapDirection> MappedComponents;
	TSet<TObjectPtr<USceneComponent>> MappedComponentsWithErrors;
	for (const FTLLRN_ControlRigComponentMappedElement& MappedElement : MappedElements)
	{
		if (MappedElement.SceneComponent)
		{
			if (ETLLRN_ControlRigComponentMapDirection* Direction = MappedComponents.Find(MappedElement.SceneComponent))
			{
				if (*Direction != MappedElement.Direction)
				{
					// elements from the same component should not be mapped to both input and output
					MappedComponentsWithErrors.Add(MappedElement.SceneComponent);
					continue;
				}
			}

			// input elements should tick before TLLRN_ControlRig updates
			// output elements should tick after TLLRN_ControlRig updates
			if (MappedElement.Direction == ETLLRN_ControlRigComponentMapDirection::Output)
			{
				MappedElement.SceneComponent->AddTickPrerequisiteComponent(this);
			}
			else
			{
				AddTickPrerequisiteComponent(MappedElement.SceneComponent);
			}

			MappedComponents.Add(MappedElement.SceneComponent) = MappedElement.Direction;
			
			// make sure that the animation is updated so that bone transforms are updated on the mapped component
			// (otherwise, FTLLRN_ControlRigAnimInstanceProxy::Evaluate is never called when moving a control in the editor)
			if (USkeletalMeshComponent* Component = Cast<USkeletalMeshComponent>(MappedElement.SceneComponent))
			{
				Component->SetUpdateAnimationInEditor(bUpdateInEditor);
			}
		}
	}

	for (const TObjectPtr<USceneComponent>& ErrorComponent : MappedComponentsWithErrors)
	{
		FString Message = FString::Printf(
				TEXT("Elements from the same component (%s) should not be mapped to both input and output,"
					" because it creates ambiguity when inferring tick order."), *ErrorComponent.GetName());
		UE_LOG(LogTLLRN_ControlRig, Warning, TEXT("%s: %s"), *GetPathName(), *Message);
	}
}

void UTLLRN_ControlRigComponent::Update(float DeltaTime)
{
	if(UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(!CanExecute())
		{
			return;
		}
		
		if (CR->IsExecuting() || CR->IsInitializing())
		{
			ReportError(TEXT("Update is being called recursively."));
		}
		else
		{
			CR->SetDeltaTime(DeltaTime);
			CR->bResetInitialTransformsBeforeConstruction = bResetInitialsBeforeConstruction;

			// todo: set log
			// todo: set external data providers

			if (bResetTransformBeforeTick)
			{
				CR->GetHierarchy()->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
			}

#if WITH_EDITOR
			// if we are recording any change - let's clear the undo stack
			if(UTLLRN_RigHierarchy* Hierarchy = CR->GetHierarchy())
			{
				if(Hierarchy->IsTracingChanges())
				{
					Hierarchy->ResetTransformStack();
				}
			}
#endif

			TransferInputs();

			if(bNeedsEvaluation)
			{
#if WITH_EDITOR
				if(UTLLRN_RigHierarchy* Hierarchy = CR->GetHierarchy())
				{
					if(Hierarchy->IsTracingChanges())
					{
						Hierarchy->StorePoseForTrace(TEXT("UTLLRN_ControlRigComponent::BeforeEvaluate"));
					}
				}
#endif

				// Animation will only evaluate if DeltaTime > 0
				if (CR->IsAdditive() && DeltaTime > 0)
				{
					CR->ClearPoseBeforeBackwardsSolve();
				}

				{
					// Necessary for FStackAttributeContainer that uses a FAnimStackAllocator (TMemStackAllocator) which allocates from FMemStack.
					// When allocating memory from FMemStack we need to explicitly use FMemMark to ensure items are freed when the scope exits. 
					FMemMark Mark(FMemStack::Get());
					TempAttributeContainer = MakeUnique<UE::Anim::FStackAttributeContainer>();
					UTLLRN_ControlRig::FAnimAttributeContainerPtrScope AttributeScope(CR, *TempAttributeContainer);
				
					CR->Evaluate_AnyThread();
				}

#if WITH_EDITOR
				if(UTLLRN_RigHierarchy* Hierarchy = CR->GetHierarchy())
				{
					if(Hierarchy->IsTracingChanges())
					{
						Hierarchy->StorePoseForTrace(TEXT("UTLLRN_ControlRigComponent::AfterEvaluate"));
					}
				}
#endif
			}
		} 

		if (bShowDebugDrawing)
		{
			if (CR->DrawInterface.Instructions.Num() > 0)
			{ 
				MarkRenderStateDirty();
			}
		}

#if WITH_EDITOR
		// if we are recording any change - dump the trace to file
		if(UTLLRN_RigHierarchy* Hierarchy = CR->GetHierarchy())
		{
			if(Hierarchy->IsTracingChanges())
			{
				Hierarchy->DumpTransformStackToFile();
			}
		}
#endif
	} 
}

TArray<FName> UTLLRN_ControlRigComponent::GetElementNames(ETLLRN_RigElementType ElementType)
{
	TArray<FName> Names;

	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		for (FTLLRN_RigBaseElement* Element : *CR->GetHierarchy())
		{
			if(Element->IsTypeOf(ElementType))
			{
				Names.Add(Element->GetFName());
			}
		}
	}

	return Names;
}

bool UTLLRN_ControlRigComponent::DoesElementExist(FName Name, ETLLRN_RigElementType ElementType)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		return CR->GetHierarchy()->GetIndex(FTLLRN_RigElementKey(Name, ElementType)) != INDEX_NONE;
	}
	return false;
}

void UTLLRN_ControlRigComponent::ClearMappedElements()
{
	if (!EnsureCalledOutsideOfBracket(TEXT("ClearMappedElements")))
	{
		return;
	}

	MappedElements.Reset();
	MappedElements = UserDefinedElements;
	ValidateMappingData();
	Initialize();
}

void UTLLRN_ControlRigComponent::SetMappedElements(TArray<FTLLRN_ControlRigComponentMappedElement> NewMappedElements)
{
	if (!EnsureCalledOutsideOfBracket(TEXT("SetMappedElements")))
	{
		return;
	}

	MappedElements = NewMappedElements;
	ValidateMappingData();
	Initialize();
}

void UTLLRN_ControlRigComponent::AddMappedElements(TArray<FTLLRN_ControlRigComponentMappedElement> NewMappedElements)
{
	if (!EnsureCalledOutsideOfBracket(TEXT("AddMappedElements")))
	{
		return;
	}

	MappedElements.Append(NewMappedElements);
	ValidateMappingData();
	Initialize();
}

void UTLLRN_ControlRigComponent::AddMappedComponents(TArray<FTLLRN_ControlRigComponentMappedComponent> Components)
{
	if (!EnsureCalledOutsideOfBracket(TEXT("AddMappedComponents")))
	{
		return;
	}

	TArray<FTLLRN_ControlRigComponentMappedElement> ElementsToMap;

	for (const FTLLRN_ControlRigComponentMappedComponent& ComponentToMap : Components)
	{
		if (ComponentToMap.Component == nullptr ||
			ComponentToMap.ElementName.IsNone())
		{
			continue;
		}

		USceneComponent* Component = ComponentToMap.Component;

		FTLLRN_ControlRigComponentMappedElement ElementToMap;
		ElementToMap.ComponentReference.OtherActor = Component->GetOwner() != GetOwner() ? Component->GetOwner() : nullptr;
		ElementToMap.ComponentReference.ComponentProperty = GetComponentNameWithinActor(Component);

		ElementToMap.ElementName = ComponentToMap.ElementName;
		ElementToMap.ElementType = ComponentToMap.ElementType;
		ElementToMap.Direction = ComponentToMap.Direction;

		ElementsToMap.Add(ElementToMap);
	}

	AddMappedElements(ElementsToMap);
}

void UTLLRN_ControlRigComponent::AddMappedSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComponent, TArray<FTLLRN_ControlRigComponentMappedBone> Bones, TArray<
                                                 FTLLRN_ControlRigComponentMappedCurve> Curves, const ETLLRN_ControlRigComponentMapDirection InDirection)
{
	if (SkeletalMeshComponent == nullptr)
	{
		return;
	}

	if (!EnsureCalledOutsideOfBracket(TEXT("AddMappedSkeletalMesh")))
	{
		return;
	}

	UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired();
	if (CR == nullptr)
	{
		// if we don't have a valid rig yet - delay it until tick component
		FSkeletalMeshToMap PendingMesh;
		PendingMesh.SkeletalMeshComponent = SkeletalMeshComponent;
		PendingMesh.Bones = Bones;
		PendingMesh.Curves = Curves;

		FScopeLock Lock(&gPendingSkeletalMeshesLock);
		TArray<FSkeletalMeshToMap>& PendingSkeletalMeshes = gPendingSkeletalMeshes.FindOrAdd(this);
		PendingSkeletalMeshes.Add(PendingMesh);
		return;
	}

	TArray<FTLLRN_ControlRigComponentMappedElement> ElementsToMap;
	TArray<FTLLRN_ControlRigComponentMappedBone> BonesToMap = Bones;
	if (BonesToMap.Num() == 0)
	{
		if (const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset())
		{
			if (const USkeleton* Skeleton = SkeletalMesh->GetSkeleton())
			{
				CR->GetHierarchy()->ForEach<FTLLRN_RigBoneElement>([SkeletalMeshComponent, &BonesToMap](FTLLRN_RigBoneElement* BoneElement) -> bool
				{
					if (SkeletalMeshComponent->GetBoneIndex(BoneElement->GetFName()) != INDEX_NONE)
					{
						FTLLRN_ControlRigComponentMappedBone BoneToMap;
						BoneToMap.Source = BoneElement->GetFName();
						BoneToMap.Target = BoneElement->GetFName();
						BonesToMap.Add(BoneToMap);
					}
					return true;
				});
			}
			else
			{
				ReportError(FString::Printf(TEXT("%s does not have a Skeleton set."), *SkeletalMesh->GetPathName()));
			}
		}
	}

	TArray<FTLLRN_ControlRigComponentMappedCurve> CurvesToMap = Curves;
	if (CurvesToMap.Num() == 0)
	{
		if (USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset())
		{
			if (USkeleton* Skeleton = SkeletalMesh->GetSkeleton())
			{
				CR->GetHierarchy()->ForEach<FTLLRN_RigCurveElement>([Skeleton, &CurvesToMap](FTLLRN_RigCurveElement* CurveElement) -> bool
                {
					FTLLRN_ControlRigComponentMappedCurve CurveToMap;
					CurveToMap.Source = CurveElement->GetFName();
					CurveToMap.Target = CurveElement->GetFName();
					CurvesToMap.Add(CurveToMap);
					return true;
				});
			}
			else
			{
				ReportError(FString::Printf(TEXT("%s does not have a Skeleton set."), *SkeletalMesh->GetPathName()));
			}
		}
	}

	for (const FTLLRN_ControlRigComponentMappedBone& BoneToMap : BonesToMap)
	{
		if (BoneToMap.Source.IsNone() ||
			BoneToMap.Target.IsNone())
		{
			continue;
		}

		FTLLRN_ControlRigComponentMappedElement ElementToMap;
		ElementToMap.ComponentReference.OtherActor = SkeletalMeshComponent->GetOwner() != GetOwner() ? SkeletalMeshComponent->GetOwner() : nullptr;
		ElementToMap.ComponentReference.ComponentProperty = GetComponentNameWithinActor(SkeletalMeshComponent);

		ElementToMap.ElementName = BoneToMap.Source;
		ElementToMap.ElementType = ETLLRN_RigElementType::Bone;
		ElementToMap.TransformName = BoneToMap.Target;
		ElementToMap.Direction = InDirection;

		ElementsToMap.Add(ElementToMap);
	}

	for (const FTLLRN_ControlRigComponentMappedCurve& CurveToMap : CurvesToMap)
	{
		if (CurveToMap.Source.IsNone() ||
			CurveToMap.Target.IsNone())
		{
			continue;
		}

		FTLLRN_ControlRigComponentMappedElement ElementToMap;
		ElementToMap.ComponentReference.OtherActor = SkeletalMeshComponent->GetOwner() != GetOwner() ? SkeletalMeshComponent->GetOwner() : nullptr;
		ElementToMap.ComponentReference.ComponentProperty = GetComponentNameWithinActor(SkeletalMeshComponent);

		ElementToMap.ElementName = CurveToMap.Source;
		ElementToMap.ElementType = ETLLRN_RigElementType::Curve;
		ElementToMap.TransformName = CurveToMap.Target;

		ElementsToMap.Add(ElementToMap);
	}

	AddMappedElements(ElementsToMap);
}

void UTLLRN_ControlRigComponent::AddMappedCompleteSkeletalMesh(USkeletalMeshComponent* SkeletalMeshComponent, const ETLLRN_ControlRigComponentMapDirection InDirection)
{
	AddMappedSkeletalMesh(SkeletalMeshComponent, TArray<FTLLRN_ControlRigComponentMappedBone>(), TArray<FTLLRN_ControlRigComponentMappedCurve>(), InDirection);
}

void UTLLRN_ControlRigComponent::SetBoneInitialTransformsFromSkeletalMesh(USkeletalMesh* InSkeletalMesh)
{
	if (InSkeletalMesh)
	{
		if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
		{
			CR->SetBoneInitialTransformsFromSkeletalMesh(InSkeletalMesh);
			bResetInitialsBeforeConstruction = false;
		}
	}
}

FTransform UTLLRN_ControlRigComponent::GetBoneTransform(FName BoneName, ETLLRN_ControlRigComponentSpace Space)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		const int32 BoneIndex = CR->GetHierarchy()->GetIndex(FTLLRN_RigElementKey(BoneName, ETLLRN_RigElementType::Bone));
		if (BoneIndex != INDEX_NONE)
		{
			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				return CR->GetHierarchy()->GetLocalTransform(BoneIndex);
			}
			else
			{
				FTransform RootTransform = CR->GetHierarchy()->GetGlobalTransform(BoneIndex);
				ConvertTransformFromTLLRN_RigSpace(RootTransform, Space);
				return RootTransform;
			}
		}
	}

	return FTransform::Identity;
}

FTransform UTLLRN_ControlRigComponent::GetInitialBoneTransform(FName BoneName, ETLLRN_ControlRigComponentSpace Space)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		const int32 BoneIndex = CR->GetHierarchy()->GetIndex(FTLLRN_RigElementKey(BoneName, ETLLRN_RigElementType::Bone));
		if (BoneIndex != INDEX_NONE)
		{
			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				return CR->GetHierarchy()->GetInitialLocalTransform(BoneIndex);
			}
			else
			{
				FTransform RootTransform = CR->GetHierarchy()->GetInitialGlobalTransform(BoneIndex);
				ConvertTransformFromTLLRN_RigSpace(RootTransform, Space);
				return RootTransform;
			}
		}
	}

	return FTransform::Identity;
}

void UTLLRN_ControlRigComponent::SetBoneTransform(FName BoneName, FTransform Transform, ETLLRN_ControlRigComponentSpace Space, float Weight, bool bPropagateToChildren)
{
	if (Weight <= SMALL_NUMBER)
	{
		return;
	}

	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		int32 BoneIndex = CR->GetHierarchy()->GetIndex(FTLLRN_RigElementKey(BoneName, ETLLRN_RigElementType::Bone));
		if (BoneIndex != INDEX_NONE)
		{
			ConvertTransformToTLLRN_RigSpace(Transform, Space);

			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				if (Weight >= 1.f - SMALL_NUMBER)
				{
					CR->GetHierarchy()->SetLocalTransform(BoneIndex, Transform, bPropagateToChildren);
				}
				else
				{
					FTransform PreviousTransform = CR->GetHierarchy()->GetLocalTransform(BoneIndex);
					FTransform BlendedTransform = FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousTransform, Transform, Weight);
					CR->GetHierarchy()->SetLocalTransform(BoneIndex, Transform, bPropagateToChildren);
				}
			}
			else
			{
				if (Weight >= 1.f - SMALL_NUMBER)
				{
					CR->GetHierarchy()->SetGlobalTransform(BoneIndex, Transform, bPropagateToChildren);
				}
				else
				{
					FTransform PreviousTransform = CR->GetHierarchy()->GetGlobalTransform(BoneIndex);
					FTransform BlendedTransform = FTLLRN_ControlRigMathLibrary::LerpTransform(PreviousTransform, Transform, Weight);
					CR->GetHierarchy()->SetGlobalTransform(BoneIndex, Transform, bPropagateToChildren);
				}
			}
		}
	}
}

void UTLLRN_ControlRigComponent::SetInitialBoneTransform(FName BoneName, FTransform InitialTransform, ETLLRN_ControlRigComponentSpace Space, bool bPropagateToChildren)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		const int32 BoneIndex = CR->GetHierarchy()->GetIndex(FTLLRN_RigElementKey(BoneName, ETLLRN_RigElementType::Bone));
		if (BoneIndex != INDEX_NONE)
		{
			if(!CR->IsRunningPreConstruction() && !CR->IsRunningPostConstruction())
			{
				ReportError(TEXT("SetInitialBoneTransform should only be called during OnPreConstruction / OnPostConstruction."));
				return;
			}

			ConvertTransformToTLLRN_RigSpace(InitialTransform, Space);

			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				const int32 ParentIndex = CR->GetHierarchy()->GetFirstParent(BoneIndex);
				if(ParentIndex != INDEX_NONE)
				{
					InitialTransform = InitialTransform * CR->GetHierarchy()->GetInitialGlobalTransform(ParentIndex);
				}
			}

			CR->GetHierarchy()->SetInitialGlobalTransform(BoneIndex, InitialTransform, bPropagateToChildren);
		}
	}	
}

bool UTLLRN_ControlRigComponent::GetControlBool(FName ControlName)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigControlElement* ControlElement = CR->GetHierarchy()->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
		{
			if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Bool)
			{
				return CR->GetHierarchy()->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<bool>();
			}
		}
	}

	return false;
}

float UTLLRN_ControlRigComponent::GetControlFloat(FName ControlName)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigControlElement* ControlElement = CR->GetHierarchy()->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
		{
			if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Float || ControlElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat)
			{
				return CR->GetHierarchy()->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<float>();
			}
		}
	}

	return 0.f;
}

int32 UTLLRN_ControlRigComponent::GetControlInt(FName ControlName)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigControlElement* ControlElement = CR->GetHierarchy()->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
		{
			if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer)
			{
				return CR->GetHierarchy()->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<int>();
			}
		}
	}

	return 0.f;
}

FVector2D UTLLRN_ControlRigComponent::GetControlVector2D(FName ControlName)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigControlElement* ControlElement = CR->GetHierarchy()->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
		{
			if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Vector2D)
			{
				const FVector3f Value = CR->GetHierarchy()->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FVector3f>();
				return FVector2D(Value.X, Value.Y);
			}
		}
	}

	return FVector2D::ZeroVector;
}

FVector UTLLRN_ControlRigComponent::GetControlPosition(FName ControlName, ETLLRN_ControlRigComponentSpace Space)
{
	return GetControlTransform(ControlName, Space).GetLocation();
}

FRotator UTLLRN_ControlRigComponent::GetControlRotator(FName ControlName, ETLLRN_ControlRigComponentSpace Space)
{
	return GetControlTransform(ControlName, Space).GetRotation().Rotator();
}

FVector UTLLRN_ControlRigComponent::GetControlScale(FName ControlName, ETLLRN_ControlRigComponentSpace Space)
{
	return GetControlTransform(ControlName, Space).GetScale3D();
}

FTransform UTLLRN_ControlRigComponent::GetControlTransform(FName ControlName, ETLLRN_ControlRigComponentSpace Space)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigControlElement* ControlElement = CR->GetHierarchy()->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
		{
			FTransform Transform;
			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				Transform = CR->GetHierarchy()->GetTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
			}
			else
			{
				Transform = CR->GetHierarchy()->GetTransform(ControlElement, ETLLRN_RigTransformType::CurrentGlobal);
				ConvertTransformFromTLLRN_RigSpace(Transform, Space);
			}
			return Transform;
		}
	}

	return FTransform::Identity;
}

void UTLLRN_ControlRigComponent::SetControlBool(FName ControlName, bool Value)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		CR->SetControlValue<bool>(ControlName, Value);
	}
}

void UTLLRN_ControlRigComponent::SetControlFloat(FName ControlName, float Value)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		CR->SetControlValue<float>(ControlName, Value);
	}
}

void UTLLRN_ControlRigComponent::SetControlInt(FName ControlName, int32 Value)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		CR->SetControlValue<int32>(ControlName, Value);
	}
}

void UTLLRN_ControlRigComponent::SetControlVector2D(FName ControlName, FVector2D Value)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		CR->SetControlValue<FVector2D>(ControlName, Value);
	}
}

void UTLLRN_ControlRigComponent::SetControlPosition(FName ControlName, FVector Value, ETLLRN_ControlRigComponentSpace Space)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigControlElement* ControlElement = CR->GetHierarchy()->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
		{
			FTransform InputTransform = FTransform::Identity;
			InputTransform.SetLocation(Value);

			FTransform PreviousTransform = FTransform::Identity;

			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				PreviousTransform = CR->GetHierarchy()->GetTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal); 
			}
			else
			{
				PreviousTransform = CR->GetHierarchy()->GetTransform(ControlElement, ETLLRN_RigTransformType::CurrentGlobal); 
				ConvertTransformToTLLRN_RigSpace(InputTransform, Space);
			}

			PreviousTransform.SetTranslation(InputTransform.GetTranslation());
			SetControlTransform(ControlName, PreviousTransform, Space);
		}
	}
}

void UTLLRN_ControlRigComponent::SetControlRotator(FName ControlName, FRotator Value, ETLLRN_ControlRigComponentSpace Space)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigControlElement* ControlElement = CR->GetHierarchy()->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
		{
			FTransform InputTransform = FTransform::Identity;
			InputTransform.SetRotation(FQuat(Value));

			FTransform PreviousTransform = FTransform::Identity;

			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				PreviousTransform = CR->GetHierarchy()->GetTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal); 
			}
			else
			{
				PreviousTransform = CR->GetHierarchy()->GetTransform(ControlElement, ETLLRN_RigTransformType::CurrentGlobal); 
				ConvertTransformToTLLRN_RigSpace(InputTransform, Space);
			}

			PreviousTransform.SetRotation(InputTransform.GetRotation());
			SetControlTransform(ControlName, PreviousTransform, Space);
		}
	}
}

void UTLLRN_ControlRigComponent::SetControlScale(FName ControlName, FVector Value, ETLLRN_ControlRigComponentSpace Space)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigControlElement* ControlElement = CR->GetHierarchy()->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
		{
			FTransform InputTransform = FTransform::Identity;
			InputTransform.SetScale3D(Value);

			FTransform PreviousTransform = FTransform::Identity;

			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				PreviousTransform = CR->GetHierarchy()->GetTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal); 
			}
			else
			{
				PreviousTransform = CR->GetHierarchy()->GetTransform(ControlElement, ETLLRN_RigTransformType::CurrentGlobal); 
				ConvertTransformToTLLRN_RigSpace(InputTransform, Space);
			}

			PreviousTransform.SetScale3D(InputTransform.GetScale3D());
			SetControlTransform(ControlName, PreviousTransform, Space);
		}
	}
}

void UTLLRN_ControlRigComponent::SetControlTransform(FName ControlName, FTransform Value, ETLLRN_ControlRigComponentSpace Space)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigControlElement* ControlElement = CR->GetHierarchy()->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
		{
			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				CR->GetHierarchy()->SetTransform(ControlElement, Value, ETLLRN_RigTransformType::CurrentLocal, true);
			}
			else
			{
				ConvertTransformToTLLRN_RigSpace(Value, Space);
				CR->GetHierarchy()->SetTransform(ControlElement, Value, ETLLRN_RigTransformType::CurrentGlobal, true);
			}
		}
	}
}

FTransform UTLLRN_ControlRigComponent::GetControlOffset(FName ControlName, ETLLRN_ControlRigComponentSpace Space)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigControlElement* ControlElement = CR->GetHierarchy()->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
		{
			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				return CR->GetHierarchy()->GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
			}
			else
			{
				FTransform RootTransform = CR->GetHierarchy()->GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::CurrentGlobal);
				ConvertTransformFromTLLRN_RigSpace(RootTransform, Space);
				return RootTransform;
			}
		}
	}

	return FTransform::Identity;
}

void UTLLRN_ControlRigComponent::SetControlOffset(FName ControlName, FTransform OffsetTransform, ETLLRN_ControlRigComponentSpace Space)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigControlElement* ControlElement = CR->GetHierarchy()->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
		{
			if (Space != ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				ConvertTransformToTLLRN_RigSpace(OffsetTransform, Space);
			}

			CR->GetHierarchy()->SetControlOffsetTransform(ControlElement, OffsetTransform,
				Space == ETLLRN_ControlRigComponentSpace::LocalSpace ? ETLLRN_RigTransformType::CurrentLocal : ETLLRN_RigTransformType::CurrentGlobal, true, false); 
		}
	}
}

FTransform UTLLRN_ControlRigComponent::GetSpaceTransform(FName SpaceName, ETLLRN_ControlRigComponentSpace Space)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigNullElement* NullElement = CR->GetHierarchy()->Find<FTLLRN_RigNullElement>(FTLLRN_RigElementKey(SpaceName, ETLLRN_RigElementType::Control)))
		{
			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				return CR->GetHierarchy()->GetTransform(NullElement, ETLLRN_RigTransformType::CurrentLocal);
			}
			else
			{
				FTransform RootTransform = CR->GetHierarchy()->GetTransform(NullElement, ETLLRN_RigTransformType::CurrentGlobal);
				ConvertTransformFromTLLRN_RigSpace(RootTransform, Space);
				return RootTransform;
			}
		}
	}

	return FTransform::Identity;
}

FTransform UTLLRN_ControlRigComponent::GetInitialSpaceTransform(FName SpaceName, ETLLRN_ControlRigComponentSpace Space)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigNullElement* NullElement = CR->GetHierarchy()->Find<FTLLRN_RigNullElement>(FTLLRN_RigElementKey(SpaceName, ETLLRN_RigElementType::Control)))
		{
			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				return CR->GetHierarchy()->GetTransform(NullElement, ETLLRN_RigTransformType::InitialLocal);
			}
			else
			{
				FTransform RootTransform = CR->GetHierarchy()->GetTransform(NullElement, ETLLRN_RigTransformType::InitialGlobal);
				ConvertTransformFromTLLRN_RigSpace(RootTransform, Space);
				return RootTransform;
			}
		}
	}

	return FTransform::Identity;
}

void UTLLRN_ControlRigComponent::SetInitialSpaceTransform(FName SpaceName, FTransform InitialTransform, ETLLRN_ControlRigComponentSpace Space)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if(FTLLRN_RigNullElement* NullElement = CR->GetHierarchy()->Find<FTLLRN_RigNullElement>(FTLLRN_RigElementKey(SpaceName, ETLLRN_RigElementType::Control)))
		{
			if (Space == ETLLRN_ControlRigComponentSpace::LocalSpace)
			{
				CR->GetHierarchy()->SetTransform(NullElement, InitialTransform, ETLLRN_RigTransformType::InitialLocal, true, false);
			}
			else
			{
				ConvertTransformToTLLRN_RigSpace(InitialTransform, Space);
				CR->GetHierarchy()->SetTransform(NullElement, InitialTransform, ETLLRN_RigTransformType::InitialGlobal, true, false);
			}
		}
	}
}

UTLLRN_ControlRig* UTLLRN_ControlRigComponent::SetupTLLRN_ControlRigIfRequired()
{
	if(TLLRN_ControlRig != nullptr)
	{
		if (TLLRN_ControlRig->GetClass() != TLLRN_ControlRigClass)
		{
			SetTLLRN_ControlRig(nullptr);
		}
		else
		{
			return TLLRN_ControlRig;
		}
	}

	if(TLLRN_ControlRigClass)
	{
		UTLLRN_ControlRig* NewTLLRN_ControlRig = NewObject<UTLLRN_ControlRig>(this, TLLRN_ControlRigClass);

		SetTLLRN_ControlRig(NewTLLRN_ControlRig);

		if (TLLRN_ControlRigCreatedEvent.IsBound())
		{
			TLLRN_ControlRigCreatedEvent.Broadcast(this);
		}

		ValidateMappingData();
	}

	return TLLRN_ControlRig;
}
void UTLLRN_ControlRigComponent::SetTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	if (TLLRN_ControlRig != InTLLRN_ControlRig)
	{
		if (TLLRN_ControlRig)
		{
			TLLRN_ControlRig->OnInitialized_AnyThread().RemoveAll(this);
			TLLRN_ControlRig->OnPreConstruction_AnyThread().RemoveAll(this);
			TLLRN_ControlRig->OnPostConstruction_AnyThread().RemoveAll(this);
			TLLRN_ControlRig->OnPreForwardsSolve_AnyThread().RemoveAll(this);
			TLLRN_ControlRig->OnPostForwardsSolve_AnyThread().RemoveAll(this);
			TLLRN_ControlRig->OnExecuted_AnyThread().RemoveAll(this);

			// rename the previous rig.
			// GC will pick it up eventually - since we won't have any
			// owning pointers to it anymore.
			TLLRN_ControlRig->Rename(nullptr, GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
			TLLRN_ControlRig->MarkAsGarbage();
		}
		TLLRN_ControlRig = InTLLRN_ControlRig;
	}

	if (TLLRN_ControlRig)
	{
		TLLRN_ControlRig->OnInitialized_AnyThread().AddUObject(this, &UTLLRN_ControlRigComponent::HandleTLLRN_ControlRigInitializedEvent);
		TLLRN_ControlRig->OnPreConstruction_AnyThread().AddUObject(this, &UTLLRN_ControlRigComponent::HandleTLLRN_ControlRigPreConstructionEvent);
		TLLRN_ControlRig->OnPostConstruction_AnyThread().AddUObject(this, &UTLLRN_ControlRigComponent::HandleTLLRN_ControlRigPostConstructionEvent);
		TLLRN_ControlRig->OnPreForwardsSolve_AnyThread().AddUObject(this, &UTLLRN_ControlRigComponent::HandleTLLRN_ControlRigPreForwardsSolveEvent);
		TLLRN_ControlRig->OnPostForwardsSolve_AnyThread().AddUObject(this, &UTLLRN_ControlRigComponent::HandleTLLRN_ControlRigPostForwardsSolveEvent);
		TLLRN_ControlRig->OnExecuted_AnyThread().AddUObject(this, &UTLLRN_ControlRigComponent::HandleTLLRN_ControlRigExecutedEvent);

		TLLRN_ControlRig->GetDataSourceRegistry()->RegisterDataSource(UTLLRN_ControlRig::OwnerComponent, this);
		if(ObjectBinding.IsValid())
		{
			TLLRN_ControlRig->SetObjectBinding(ObjectBinding);
		}

		TLLRN_ControlRig->Initialize();
	}
}

void UTLLRN_ControlRigComponent::SetTLLRN_ControlRigClass(TSubclassOf<UTLLRN_ControlRig> InTLLRN_ControlRigClass)
{
	if (TLLRN_ControlRigClass != InTLLRN_ControlRigClass)
	{
		SetTLLRN_ControlRig(nullptr);
		TLLRN_ControlRigClass = InTLLRN_ControlRigClass;
		Initialize();
	}
}

void UTLLRN_ControlRigComponent::SetObjectBinding(UObject* InObjectToBind)
{
	if(!ObjectBinding.IsValid())
	{
		ObjectBinding = MakeShared<FTLLRN_ControlRigObjectBinding>();
	}
	ObjectBinding->BindToObject(InObjectToBind);

	if(UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		CR->SetObjectBinding(ObjectBinding);
	}
}

void UTLLRN_ControlRigComponent::ValidateMappingData()
{
	TMap<USkeletalMeshComponent*, FCachedSkeletalMeshComponentSettings> NewCachedSettings;

	if(TLLRN_ControlRig)
	{
		for (FTLLRN_ControlRigComponentMappedElement& MappedElement : MappedElements)
		{
			MappedElement.ElementIndex = INDEX_NONE;
			MappedElement.SubIndex = INDEX_NONE;

			AActor* MappedOwner = !MappedElement.ComponentReference.OtherActor.IsValid() ? GetOwner() : MappedElement.ComponentReference.OtherActor.Get();
			MappedElement.SceneComponent = Cast<USceneComponent>(MappedElement.ComponentReference.GetComponent(MappedOwner));

			// try again with the path to the component
			if (MappedElement.SceneComponent == nullptr)
			{
				FSoftComponentReference TempReference;
				TempReference.PathToComponent = MappedElement.ComponentReference.ComponentProperty.ToString();
				if (USceneComponent* TempSceneComponent = Cast<USceneComponent>(TempReference.GetComponent(MappedOwner)))
				{
					MappedElement.ComponentReference = TempReference;
					MappedElement.SceneComponent = TempSceneComponent;
				}
			}

			if (MappedElement.SceneComponent == nullptr ||
				MappedElement.SceneComponent == this ||
				MappedElement.ElementName.IsNone())
			{
				continue;
			}

			// cache the scene component also in the override component to avoid on further relying on names
			MappedElement.ComponentReference.OverrideComponent = MappedElement.SceneComponent;

			if (MappedElement.Direction == ETLLRN_ControlRigComponentMapDirection::Output && MappedElement.Weight <= SMALL_NUMBER)
			{
				continue;
			}

			FTLLRN_RigElementKey Key(MappedElement.ElementName, MappedElement.ElementType);
			MappedElement.ElementIndex = TLLRN_ControlRig->GetHierarchy()->GetIndex(Key);
			MappedElement.SubIndex = MappedElement.TransformIndex;

			if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MappedElement.SceneComponent))
			{
				//MappedElement.Space = ETLLRN_ControlRigComponentSpace::ComponentSpace;

				MappedElement.SubIndex = INDEX_NONE;
				if (MappedElement.TransformIndex >= 0 && MappedElement.TransformIndex < SkeletalMeshComponent->GetNumBones())
				{
					MappedElement.SubIndex = MappedElement.TransformIndex;
				}
				else if (!MappedElement.TransformName.IsNone())
				{
					if (USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset())
					{
						if (USkeleton* Skeleton = SkeletalMesh->GetSkeleton())
						{
							if (MappedElement.ElementType != ETLLRN_RigElementType::Curve)
							{
								// this is really getting the FMeshPoseBoneIndex
								MappedElement.SubIndex = SkeletalMeshComponent->GetBoneIndex(MappedElement.TransformName);
							}
						}
						else
						{
							ReportError(FString::Printf(TEXT("%s does not have a Skeleton set."), *SkeletalMesh->GetPathName()));
						}
					}

					// if we didn't find the bone, disable this mapped element
					if (MappedElement.ElementType != ETLLRN_RigElementType::Curve)
					{
						if (MappedElement.SubIndex == INDEX_NONE)
						{
							MappedElement.ElementIndex = INDEX_NONE;
							continue;
						}
					}
				}

				if (MappedElement.Direction == ETLLRN_ControlRigComponentMapDirection::Output)
				{
					if (!NewCachedSettings.Contains(SkeletalMeshComponent))
					{
						FCachedSkeletalMeshComponentSettings PreviousSettings(SkeletalMeshComponent);
						NewCachedSettings.Add(SkeletalMeshComponent, PreviousSettings);
					}

					//If the animinstance is a sequencer instance don't replace it that means we are already running an animation on the skeleton
					//and don't want to replace the anim instance.
					if (Cast<ISequencerAnimationSupport>(SkeletalMeshComponent->GetAnimInstance()) == nullptr)
					{
						SkeletalMeshComponent->SetAnimInstanceClass(UTLLRN_ControlRigAnimInstance::StaticClass());
					}
				}
			}
		}
	}

	// for the skeletal mesh components we no longer map, let's remove it
	for (TPair<USkeletalMeshComponent*, FCachedSkeletalMeshComponentSettings>& Pair : CachedSkeletalMeshComponentSettings)
	{
		FCachedSkeletalMeshComponentSettings* NewCachedSetting = NewCachedSettings.Find(Pair.Key);
		if (NewCachedSetting)
		{
			*NewCachedSetting = Pair.Value;
		}
		else
		{
			Pair.Value.Apply(Pair.Key);
		}
	}

	CachedSkeletalMeshComponentSettings = NewCachedSettings;
}

void UTLLRN_ControlRigComponent::TransferInputs()
{
	bNeedsEvaluation = true;
	if (TLLRN_ControlRig)
	{
		InputElementIndices.Reset();
		InputTransforms.Reset();
		
		for (FTLLRN_ControlRigComponentMappedElement& MappedElement : MappedElements)
		{
			if (MappedElement.ElementIndex == INDEX_NONE || MappedElement.Direction == ETLLRN_ControlRigComponentMapDirection::Output)
			{
				continue;
			}

			FTransform Transform = FTransform::Identity;
			if (MappedElement.SubIndex >= 0)
			{
				if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MappedElement.SceneComponent))
				{
					Transform = SkeletalMeshComponent->GetBoneTransform(MappedElement.SubIndex, FTransform::Identity);
					if(MappedElement.Space == ETLLRN_ControlRigComponentSpace::WorldSpace)
					{
						Transform = Transform * MappedElement.SceneComponent->GetComponentToWorld();
					}
				}
				else if (UInstancedStaticMeshComponent* InstancingComponent = Cast<UInstancedStaticMeshComponent>(MappedElement.SceneComponent))
				{
					if (MappedElement.SubIndex < InstancingComponent->GetNumRenderInstances())
					{
						InstancingComponent->GetInstanceTransform(MappedElement.SubIndex, Transform, true);
					}
					else
					{
						continue;
					}
				}
			}
			else
			{
				Transform = MappedElement.SceneComponent->GetComponentToWorld();
			}

			Transform = Transform * MappedElement.Offset;

			ConvertTransformToTLLRN_RigSpace(Transform, MappedElement.Space);

			InputElementIndices.Add(MappedElement.ElementIndex);
			InputTransforms.Add(Transform);
		}

		if(bEnableLazyEvaluation && InputTransforms.Num() > 0)
		{
			if(LastInputTransforms.Num() == InputTransforms.Num())
			{
				bNeedsEvaluation = false;

				const float PositionU = FMath::Abs(LazyEvaluationPositionThreshold);
				const float RotationU = FMath::Abs(LazyEvaluationRotationThreshold);
				const float ScaleU = FMath::Abs(LazyEvaluationScaleThreshold);

				for(int32 Index=0;Index<InputElementIndices.Num();Index++)
				{
					if(!FTLLRN_RigUnit_PoseGetDelta::AreTransformsEqual(
						InputTransforms[Index],
						LastInputTransforms[Index],
						PositionU,
						RotationU,
						ScaleU))
					{
						bNeedsEvaluation = true;
						break;
					}
				}
			}

			LastInputTransforms = InputTransforms;
		}

		if(bNeedsEvaluation)
		{
			for(int32 Index=0;Index<InputElementIndices.Num();Index++)
			{
				TLLRN_ControlRig->GetHierarchy()->SetGlobalTransform(InputElementIndices[Index], InputTransforms[Index]);
			}
		}

		if(UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
		{
			Hierarchy->ResetChangedCurveIndices();
			
#if WITH_EDITOR
			if(Hierarchy->IsTracingChanges())
			{
				Hierarchy->StorePoseForTrace(TEXT("UTLLRN_ControlRigComponent::TransferInputs"));
			}
#endif
		}
	}
}

void UTLLRN_ControlRigComponent::TransferOutputs()
{
	if (TLLRN_ControlRig)
	{
		if (TLLRN_ControlRig->IsAdditive())
		{
			if (MappedElements.Num() > 0)
			{
				if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MappedElements[0].SceneComponent))
				{
					if (USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset())
					{
						USceneComponent* LastComponent = nullptr;
						FTLLRN_ControlRigAnimInstanceProxy* Proxy = nullptr;
						
						TArray<FTransform> BoneSpaceTransforms;
						BoneSpaceTransforms.SetNumUninitialized(SkeletalMesh->GetRefSkeleton().GetNum());
						for (const FTLLRN_ControlRigComponentMappedElement& MappedElement : MappedElements)
						{
							if (LastComponent != MappedElement.SceneComponent || Proxy == nullptr)
							{
								Proxy = MappedElement.GetAnimProxyOnGameThread();
								if (Proxy)
								{
									Proxy->StoredTransforms.Reset();
									Proxy->StoredCurves.Reset();
									Proxy->StoredAttributes.Empty();
									LastComponent = MappedElement.SceneComponent;
								}
							}
							
							if (MappedElement.ElementIndex == INDEX_NONE || MappedElement.Direction == ETLLRN_ControlRigComponentMapDirection::Input)
							{
								continue;
							}

							if (MappedElement.ElementType == ETLLRN_RigElementType::Bone ||
								MappedElement.ElementType == ETLLRN_RigElementType::Control ||
								MappedElement.ElementType == ETLLRN_RigElementType::Null)
							{
								if (MappedElement.SubIndex >= 0 && BoneSpaceTransforms.IsValidIndex(MappedElement.SubIndex))
								{
									FTransform Transform = TLLRN_ControlRig->GetHierarchy()->GetLocalTransform(MappedElement.ElementIndex);
									BoneSpaceTransforms[MappedElement.SubIndex] = Transform;
								}
							}
						}

						TArray<FTransform> OutSpaceBases;
						OutSpaceBases.SetNumUninitialized(BoneSpaceTransforms.Num());
						SkeletalMesh->FillComponentSpaceTransforms(BoneSpaceTransforms, SkeletalMeshComponent->FillComponentSpaceTransformsRequiredBones, SkeletalMeshComponent->GetEditableComponentSpaceTransforms());

						if (Proxy)
						{
							Proxy->StoredAttributes.CopyFrom(*TempAttributeContainer);
						}	
					}
				}
			}
		}
		else
		{
			USceneComponent* LastComponent = nullptr;
			FTLLRN_ControlRigAnimInstanceProxy* Proxy = nullptr;

			for (const FTLLRN_ControlRigComponentMappedElement& MappedElement : MappedElements)
			{
				if (LastComponent != MappedElement.SceneComponent || Proxy == nullptr)
				{
					Proxy = MappedElement.GetAnimProxyOnGameThread();
					if (Proxy)
					{
						Proxy->StoredTransforms.Reset();
						Proxy->StoredCurves.Reset();
						Proxy->StoredAttributes.Empty();
						LastComponent = MappedElement.SceneComponent;
					}
				}
			}

			for (const FTLLRN_ControlRigComponentMappedElement& MappedElement : MappedElements)
			{
				if (MappedElement.ElementIndex == INDEX_NONE || MappedElement.Direction == ETLLRN_ControlRigComponentMapDirection::Input)
				{
					continue;
				}

				if (MappedElement.ElementType == ETLLRN_RigElementType::Bone ||
					MappedElement.ElementType == ETLLRN_RigElementType::Control ||
					MappedElement.ElementType == ETLLRN_RigElementType::Null)
				{
					FTransform Transform = TLLRN_ControlRig->GetHierarchy()->GetGlobalTransform(MappedElement.ElementIndex);
					ConvertTransformFromTLLRN_RigSpace(Transform, MappedElement.Space);

					Transform = Transform * MappedElement.Offset;

					if (MappedElement.SubIndex >= 0)
					{
						if (LastComponent != MappedElement.SceneComponent || Proxy == nullptr)
						{
							Proxy = MappedElement.GetAnimProxyOnGameThread();
							if (Proxy)
							{
								LastComponent = MappedElement.SceneComponent;
							}
						}

						if (Proxy && (MappedElement.SceneComponent != nullptr))
						{
							if(MappedElement.Space == ETLLRN_ControlRigComponentSpace::WorldSpace)
							{
								Transform = Transform.GetRelativeTransform(MappedElement.SceneComponent->GetComponentToWorld());
							}
							Proxy->StoredTransforms.FindOrAdd(FMeshPoseBoneIndex(MappedElement.SubIndex)) = Transform;
						}
						else if (UInstancedStaticMeshComponent* InstancingComponent = Cast<UInstancedStaticMeshComponent>(MappedElement.SceneComponent))
						{
							if (MappedElement.SubIndex < InstancingComponent->GetNumRenderInstances())
							{
								if (MappedElement.Weight < 1.f - SMALL_NUMBER)
								{
									FTransform Previous = FTransform::Identity;
									InstancingComponent->GetInstanceTransform(MappedElement.SubIndex, Previous, true);
									Transform = FTLLRN_ControlRigMathLibrary::LerpTransform(Previous, Transform, FMath::Clamp<float>(MappedElement.Weight, 0.f, 1.f));
								}
								InstancingComponent->UpdateInstanceTransform(MappedElement.SubIndex, Transform, true, true, true);
							}
						}
					}
					else
					{
						if (MappedElement.Weight < 1.f - SMALL_NUMBER)
						{
							FTransform Previous = MappedElement.SceneComponent->GetComponentToWorld();
							Transform = FTLLRN_ControlRigMathLibrary::LerpTransform(Previous, Transform, FMath::Clamp<float>(MappedElement.Weight, 0.f, 1.f));
						}
						MappedElement.SceneComponent->SetWorldTransform(Transform);
					}
				}
				else if (MappedElement.ElementType == ETLLRN_RigElementType::Curve)
				{
					if (MappedElement.ElementIndex >= 0)
					{
						if (LastComponent != MappedElement.SceneComponent || Proxy == nullptr)
						{
							Proxy = MappedElement.GetAnimProxyOnGameThread();
							if (Proxy)
							{
								LastComponent = MappedElement.SceneComponent;
							}
						}

						if (Proxy)
						{
							Proxy->StoredCurves.FindOrAdd(MappedElement.TransformName) = TLLRN_ControlRig->GetHierarchy()->GetCurveValue(MappedElement.ElementIndex);
						}
					}
				}
			}

			if (Proxy)
			{
				Proxy->StoredAttributes.CopyFrom(*TempAttributeContainer);
			}			
		}

#if WITH_EDITOR
		if(UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
		{
			if(Hierarchy->IsTracingChanges())
			{
				Hierarchy->StorePoseForTrace(TEXT("UTLLRN_ControlRigComponent::TransferOutputs"));
			}
		}
#endif
	}
}

FName UTLLRN_ControlRigComponent::GetComponentNameWithinActor(UActorComponent* InComponent)
{
	check(InComponent);
	
	FName ComponentProperty = InComponent->GetFName(); 

	if(AActor* Owner = InComponent->GetOwner())
	{
		// we need to see if the owner stores this component as a property
		for (TFieldIterator<FProperty> PropertyIt(Owner->GetClass()); PropertyIt; ++PropertyIt)
		{
			if(const FObjectPropertyBase* Property = CastField<FObjectPropertyBase>(*PropertyIt))
			{
				if(Property->GetObjectPropertyValue_InContainer(Owner) == InComponent)
				{
					ComponentProperty = Property->GetFName();
					break;
				}
			}
		}

#if WITH_EDITOR

		// validate that the property storage will return the right component.
		// this is a sanity check ensuring that ComponentReference will find the right component later.
		FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Owner->GetClass(), ComponentProperty);
		if(Property != nullptr)
		{
			UActorComponent* FoundComponent = Cast<UActorComponent>(Property->GetObjectPropertyValue_InContainer(Owner));
			check(FoundComponent == InComponent);
		}
		
#endif
	}
	return ComponentProperty;
}

void UTLLRN_ControlRigComponent::HandleTLLRN_ControlRigInitializedEvent(URigVMHost* InTLLRN_ControlRig, const FName& InEventName)
{
#if WITH_EDITOR
	if (bUpdateInEditor)
	{
		FEditorScriptExecutionGuard AllowScripts;
		OnPostInitialize(this);
	}
	else
#endif
	{
		OnPostInitialize(this);
	}
}

void UTLLRN_ControlRigComponent::HandleTLLRN_ControlRigPreConstructionEvent(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
{
	TArray<USkeletalMeshComponent*> ComponentsToTick;

	USceneComponent* LastComponent = nullptr;
	FTLLRN_ControlRigAnimInstanceProxy* Proxy = nullptr;

	for (FTLLRN_ControlRigComponentMappedElement& MappedElement : MappedElements)
	{
		if (LastComponent != MappedElement.SceneComponent || Proxy == nullptr)
		{
			Proxy = MappedElement.GetAnimProxyOnGameThread();
			if (Proxy)
			{
				Proxy->StoredTransforms.Reset();
				Proxy->StoredCurves.Reset();
				Proxy->StoredAttributes.Empty();
				LastComponent = MappedElement.SceneComponent;
			}
		}

		if (USkeletalMeshComponent* Component = Cast< USkeletalMeshComponent>(MappedElement.SceneComponent))
		{
			ComponentsToTick.AddUnique(Component);
		}
	}

	for (USkeletalMeshComponent* SkeletalMeshComponent : ComponentsToTick)
	{
		SkeletalMeshComponent->TickAnimation(0.f, false);
		SkeletalMeshComponent->RefreshBoneTransforms();
		SkeletalMeshComponent->RefreshFollowerComponents();
		SkeletalMeshComponent->UpdateComponentToWorld();
		SkeletalMeshComponent->FinalizeBoneTransform();
		SkeletalMeshComponent->MarkRenderTransformDirty();
		SkeletalMeshComponent->MarkRenderDynamicDataDirty();
	}

#if WITH_EDITOR
	if (bUpdateInEditor)
	{
		FEditorScriptExecutionGuard AllowScripts;
		OnPreConstruction(this);
	}
	else
#endif
	{
		OnPreConstruction(this);
	}
}

void UTLLRN_ControlRigComponent::HandleTLLRN_ControlRigPostConstructionEvent(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
{
#if WITH_EDITOR
	if (bUpdateInEditor)
	{
		FEditorScriptExecutionGuard AllowScripts;
		OnPostConstruction(this);
	}
	else
#endif
	{
		OnPostConstruction(this);
	}
}

void UTLLRN_ControlRigComponent::HandleTLLRN_ControlRigPreForwardsSolveEvent(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
{
#if WITH_EDITOR
	if (bUpdateInEditor)
	{
		FEditorScriptExecutionGuard AllowScripts;
		OnPreForwardsSolve(this);
	}
	else
#endif
	{
		OnPreForwardsSolve(this);
	}
}

void UTLLRN_ControlRigComponent::HandleTLLRN_ControlRigPostForwardsSolveEvent(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
{
#if WITH_EDITOR
	if (bUpdateInEditor)
	{
		FEditorScriptExecutionGuard AllowScripts;
		OnPostForwardsSolve(this);
	}
	else
#endif
	{
		OnPostForwardsSolve(this);
	}
}

void UTLLRN_ControlRigComponent::HandleTLLRN_ControlRigExecutedEvent(URigVMHost* InTLLRN_ControlRig, const FName& InEventName)
{
	TransferOutputs();
}

void UTLLRN_ControlRigComponent::ConvertTransformToTLLRN_RigSpace(FTransform& InOutTransform, ETLLRN_ControlRigComponentSpace FromSpace)
{
	switch (FromSpace)
	{
		case ETLLRN_ControlRigComponentSpace::WorldSpace:
		{
			InOutTransform = InOutTransform.GetRelativeTransform(GetComponentToWorld());
			break;
		}
		case ETLLRN_ControlRigComponentSpace::ActorSpace:
		{
			InOutTransform = InOutTransform.GetRelativeTransform(GetRelativeTransform());
			break;
		}
		case ETLLRN_ControlRigComponentSpace::ComponentSpace:
		case ETLLRN_ControlRigComponentSpace::TLLRN_RigSpace:
		case ETLLRN_ControlRigComponentSpace::LocalSpace:
		default:
		{
			// nothing to do
			break;
		}
	}
}

void UTLLRN_ControlRigComponent::ConvertTransformFromTLLRN_RigSpace(FTransform& InOutTransform, ETLLRN_ControlRigComponentSpace ToSpace)
{
	switch (ToSpace)
	{
		case ETLLRN_ControlRigComponentSpace::WorldSpace:
		{
			InOutTransform = InOutTransform * GetComponentToWorld();
			break;
		}
		case ETLLRN_ControlRigComponentSpace::ActorSpace:
		{
			InOutTransform = InOutTransform * GetRelativeTransform();
			break;
		}
		case ETLLRN_ControlRigComponentSpace::ComponentSpace:
		case ETLLRN_ControlRigComponentSpace::TLLRN_RigSpace:
		case ETLLRN_ControlRigComponentSpace::LocalSpace:
		default:
		{
			// nothing to do
			break;
		}
	}
}

bool UTLLRN_ControlRigComponent::EnsureCalledOutsideOfBracket(const TCHAR* InCallingFunctionName)
{
	if (UTLLRN_ControlRig* CR = SetupTLLRN_ControlRigIfRequired())
	{
		if (CR->IsRunningPreConstruction())
		{
			if (InCallingFunctionName)
			{
				ReportError(FString::Printf(TEXT("%s cannot be called during the PreConstructionEvent - use ConstructionScript instead."), InCallingFunctionName));
				return false;
			}
			else
			{
				ReportError(TEXT("Cannot be called during the PreConstructionEvent - use ConstructionScript instead."));
				return false;
			}
		}

		if (CR->IsRunningPostConstruction())
		{
			if (InCallingFunctionName)
			{
				ReportError(FString::Printf(TEXT("%s cannot be called during the PostConstructionEvent - use ConstructionScript instead."), InCallingFunctionName));
				return false;
			}
			else
			{
				ReportError(TEXT("Cannot be called during the PostConstructionEvent - use ConstructionScript instead."));
				return false;
			}
		}

		if (CR->IsInitializing())
		{
			if (InCallingFunctionName)
			{
				ReportError(FString::Printf(TEXT("%s cannot be called during the InitEvent - use ConstructionScript instead."), InCallingFunctionName));
				return false;
			}
			else
			{
				ReportError(TEXT("Cannot be called during the InitEvent - use ConstructionScript instead."));
				return false;
			}
		}

		if (CR->IsExecuting())
		{
			if (InCallingFunctionName)
			{
				ReportError(FString::Printf(TEXT("%s cannot be called during the ForwardsSolveEvent - use ConstructionScript instead."), InCallingFunctionName));
				return false;
			}
			else
			{
				ReportError(TEXT("Cannot be called during the ForwardsSolveEvent - use ConstructionScript instead."));
				return false;
			}
		}
	}
	
	return true;
}

void UTLLRN_ControlRigComponent::ReportError(const FString& InMessage)
{
	UE_LOG(LogTLLRN_ControlRig, Warning, TEXT("%s: %s"), *GetPathName(), *InMessage);

#if WITH_EDITOR

	if (GetWorld()->IsEditorWorld())
	{
		const TSharedPtr<SNotificationItem>* ExistingItemPtr = EditorNotifications.Find(InMessage);
		if (ExistingItemPtr)
		{
			const TSharedPtr<SNotificationItem>& ExistingItem = *ExistingItemPtr;
			if (ExistingItem.IsValid())
			{
				if (ExistingItem->HasActiveTimers())
				{
					return;
				}
				else
				{
					EditorNotifications.Remove(InMessage);
				}
			}
		}

		FNotificationInfo Info(FText::FromString(InMessage));
		Info.bUseSuccessFailIcons = true;
		Info.Image = FAppStyle::GetBrush(TEXT("MessageLog.Warning"));
		Info.bFireAndForget = true;
		Info.bUseThrobber = true;
		Info.FadeOutDuration = 8.0f;
		Info.ExpireDuration = Info.FadeOutDuration;
		TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
		NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);

		EditorNotifications.Add(InMessage, NotificationPtr);
	}
#endif
}

FTLLRN_ControlRigSceneProxy::FTLLRN_ControlRigSceneProxy(const UTLLRN_ControlRigComponent* InComponent)
: FPrimitiveSceneProxy(InComponent)
, TLLRN_ControlRigComponent(InComponent)
{
	bWillEverBeLit = false;
}

SIZE_T FTLLRN_ControlRigSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

void FTLLRN_ControlRigSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	if (TLLRN_ControlRigComponent->TLLRN_ControlRig == nullptr)
	{
		return;
	}

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];
			FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

			bool bShouldDrawBones = TLLRN_ControlRigComponent->bDrawBones && TLLRN_ControlRigComponent->TLLRN_ControlRig != nullptr;

			// make sure to check if we are within a preview / editor world
			// or the console variable draw bones is turned on
			if (bShouldDrawBones)
			{
				if (UWorld* World = TLLRN_ControlRigComponent->GetWorld())
				{
					if (!World->IsPreviewWorld())
					{
						const FEngineShowFlags& EngineShowFlags = ViewFamily.EngineShowFlags;
						bShouldDrawBones = EngineShowFlags.Bones != 0;
					}
				}
			}

			if (bShouldDrawBones)
			{
				const float RadiusMultiplier = TLLRN_ControlRigComponent->TLLRN_ControlRig->GetDebugBoneRadiusMultiplier();
				const FTransform Transform = TLLRN_ControlRigComponent->GetComponentToWorld();
				const float MaxDrawRadius = TLLRN_ControlRigComponent->Bounds.SphereRadius * 0.02f;

				UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigComponent->TLLRN_ControlRig->GetHierarchy();
				Hierarchy->ForEach<FTLLRN_RigBoneElement>([PDI, Hierarchy, Transform, MaxDrawRadius, RadiusMultiplier](FTLLRN_RigBoneElement* BoneElement) -> bool
                {
                    const int32 ParentIndex = Hierarchy->GetFirstParent(BoneElement->GetIndex());
					const FLinearColor LineColor = FLinearColor::White;

					FVector Start, End;
					if (ParentIndex >= 0)
					{
						Start = Hierarchy->GetGlobalTransform(ParentIndex).GetLocation();
						End = Hierarchy->GetGlobalTransform(BoneElement->GetIndex()).GetLocation();
					}
					else
					{
						Start = FVector::ZeroVector;
						End = Hierarchy->GetGlobalTransform(BoneElement->GetIndex()).GetLocation();
					}

					Start = Transform.TransformPosition(Start);
					End = Transform.TransformPosition(End);

					const float BoneLength = (End - Start).Size();
					// clamp by bound, we don't want too long or big
					const float Radius = FMath::Clamp<float>(BoneLength * 0.05f, 0.1f, MaxDrawRadius) * RadiusMultiplier;

					//Render Sphere for bone end point and a cone between it and its parent.
					SkeletalDebugRendering::DrawWireBone(PDI, Start, End, LineColor, SDPG_Foreground, Radius);

					return true;
				});
			}

			if (TLLRN_ControlRigComponent->bShowDebugDrawing)
			{ 
				const FRigVMDrawInterface& DrawInterface = TLLRN_ControlRigComponent->TLLRN_ControlRig->GetDrawInterface();

				for (int32 InstructionIndex = 0; InstructionIndex < DrawInterface.Num(); InstructionIndex++)
				{
					const FRigVMDrawInstruction& Instruction = DrawInterface[InstructionIndex];
					if (Instruction.Positions.Num() == 0)
					{
						continue;
					}

					FTransform InstructionTransform = Instruction.Transform * TLLRN_ControlRigComponent->GetComponentToWorld();
					switch (Instruction.PrimitiveType)
					{
						case ERigVMDrawSettings::Points:
						{
							for (const FVector& Point : Instruction.Positions)
							{
								PDI->DrawPoint(InstructionTransform.TransformPosition(Point), Instruction.Color, Instruction.Thickness, SDPG_Foreground);
							}
							break;
						}
						case ERigVMDrawSettings::Lines:
						{
							const TArray<FVector>& Points = Instruction.Positions;
							PDI->AddReserveLines(SDPG_Foreground, Points.Num() / 2, false, Instruction.Thickness > SMALL_NUMBER);
							for (int32 PointIndex = 0; PointIndex < Points.Num() - 1; PointIndex += 2)
							{
								PDI->DrawLine(InstructionTransform.TransformPosition(Points[PointIndex]), InstructionTransform.TransformPosition(Points[PointIndex + 1]), Instruction.Color, SDPG_Foreground, Instruction.Thickness);
							}
							break;
						}
						case ERigVMDrawSettings::LineStrip:
						{
							const TArray<FVector>& Points = Instruction.Positions;
							PDI->AddReserveLines(SDPG_Foreground, Points.Num() - 1, false, Instruction.Thickness > SMALL_NUMBER);
							for (int32 PointIndex = 0; PointIndex < Points.Num() - 1; PointIndex++)
							{
								PDI->DrawLine(InstructionTransform.TransformPosition(Points[PointIndex]), InstructionTransform.TransformPosition(Points[PointIndex + 1]), Instruction.Color, SDPG_Foreground, Instruction.Thickness);
							}
							break;
						}
					}
				}
			}
		}
	}
}

/**
*  Returns a struct that describes to the renderer when to draw this proxy.
*	@param		Scene view to use to determine our relevence.
*  @return		View relevance struct
*/
FPrimitiveViewRelevance FTLLRN_ControlRigSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance ViewRelevance;
	ViewRelevance.bDrawRelevance = IsShown(View);
	ViewRelevance.bDynamicRelevance = true;
	// ideally the TranslucencyRelevance should be filled out by the material, here we do it conservative
	ViewRelevance.bSeparateTranslucency = ViewRelevance.bNormalTranslucency = true;
	return ViewRelevance;
}

uint32 FTLLRN_ControlRigSceneProxy::GetMemoryFootprint(void) const
{
	return(sizeof(*this) + GetAllocatedSize());
}

uint32 FTLLRN_ControlRigSceneProxy::GetAllocatedSize(void) const
{
	return FPrimitiveSceneProxy::GetAllocatedSize();
}

