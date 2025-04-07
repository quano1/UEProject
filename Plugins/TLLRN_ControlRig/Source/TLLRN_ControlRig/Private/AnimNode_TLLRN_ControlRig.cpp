// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNode_TLLRN_ControlRig.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "GameFramework/Actor.h"
#include "Animation/NodeMappingContainer.h"
#include "AnimationRuntime.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "Animation/AnimCurveUtils.h"
#include "Animation/AnimStats.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_TLLRN_ControlRig)

#if WITH_EDITOR
#include "Editor.h"
#endif

FAnimNode_TLLRN_ControlRig::FAnimNode_TLLRN_ControlRig()
	: FAnimNode_TLLRN_ControlRigBase()
	, TLLRN_ControlRig(nullptr)
	, Alpha(1.f)
	, AlphaInputType(EAnimAlphaInputType::Float)
	, bAlphaBoolEnabled(true)
	, bSetRefPoseFromSkeleton(false)
	, AlphaCurveName(NAME_None)
	, LODThreshold(INDEX_NONE)
	, RefPoseSetterHash()
{
}

FAnimNode_TLLRN_ControlRig::~FAnimNode_TLLRN_ControlRig()
{
	if(TLLRN_ControlRig)
	{
		TLLRN_ControlRig->OnInitialized_AnyThread().RemoveAll(this);
	}
}

void FAnimNode_TLLRN_ControlRig::HandleOnInitialized_AnyThread(URigVMHost*, const FName&)
{
	RefPoseSetterHash.Reset();
}

void FAnimNode_TLLRN_ControlRig::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	TLLRN_ControlRigPerClass.Reset();
	if(DefaultTLLRN_ControlRigClass)
	{
		TLLRN_ControlRigClass = nullptr;
	}
	
	if(UpdateTLLRN_ControlRigIfNeeded(InAnimInstance, InAnimInstance->GetRequiredBones()))
	{
		UpdateTLLRN_ControlRigRefPoseIfNeeded(InProxy);
	}

	FAnimNode_TLLRN_ControlRigBase::OnInitializeAnimInstance(InProxy, InAnimInstance);

	InitializeProperties(InAnimInstance, GetTargetClass());
}

void FAnimNode_TLLRN_ControlRig::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += FString::Printf(TEXT("(%s)"), *GetNameSafe(TLLRN_ControlRigClass.Get()));
	DebugData.AddDebugItem(DebugLine);
	Source.GatherDebugData(DebugData.BranchFlow(1.f));
}

void FAnimNode_TLLRN_ControlRig::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	if (IsLODEnabled(Context.AnimInstanceProxy))
	{
		GetEvaluateGraphExposedInputs().Execute(Context);

		// alpha handlers
		InternalBlendAlpha = 0.f;
		switch (AlphaInputType)
		{
		case EAnimAlphaInputType::Float:
			InternalBlendAlpha = AlphaScaleBias.ApplyTo(AlphaScaleBiasClamp.ApplyTo(Alpha, Context.GetDeltaTime()));
			break;
		case EAnimAlphaInputType::Bool:
			InternalBlendAlpha = AlphaBoolBlend.ApplyTo(bAlphaBoolEnabled, Context.GetDeltaTime());
			break;
		case EAnimAlphaInputType::Curve:
			if (UAnimInstance* AnimInstance = Cast<UAnimInstance>(Context.AnimInstanceProxy->GetAnimInstanceObject()))
			{
				InternalBlendAlpha = AlphaScaleBiasClamp.ApplyTo(AnimInstance->GetCurveValue(AlphaCurveName), Context.GetDeltaTime());
			}
			break;
		};

		// Make sure Alpha is clamped between 0 and 1.
		InternalBlendAlpha = FMath::Clamp<float>(InternalBlendAlpha, 0.f, 1.f);

		PropagateInputProperties(Context.AnimInstanceProxy->GetAnimInstanceObject());
	}
	else
	{
		InternalBlendAlpha = 0.f;
	}

	if(const UAnimInstance* AnimInstance = Cast<UAnimInstance>(Context.GetAnimInstanceObject()))
	{
		(void)UpdateTLLRN_ControlRigIfNeeded(AnimInstance, Context.AnimInstanceProxy->GetRequiredBones());
	}

	UpdateTLLRN_ControlRigRefPoseIfNeeded(Context.AnimInstanceProxy);
	FAnimNode_TLLRN_ControlRigBase::Update_AnyThread(Context);

	TRACE_ANIM_NODE_VALUE(Context, TEXT("Class"), *GetNameSafe(TLLRN_ControlRigClass.Get()));
}

void FAnimNode_TLLRN_ControlRig::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FAnimNode_TLLRN_ControlRigBase::Initialize_AnyThread(Context);

	if (TLLRN_ControlRig)
	{
		//Don't Inititialize the Control Rig here it may have the wrong VM on the CDO
		SetTargetInstance(TLLRN_ControlRig);
		TLLRN_ControlRig->RequestInit();
		bTLLRN_ControlRigRequiresInitialization = true;
		LastBonesSerialNumberForCacheBones = 0;
	}
	else
	{
		SetTargetInstance(nullptr);
	}

	AlphaBoolBlend.Reinitialize();
	AlphaScaleBiasClamp.Reinitialize();
}

void FAnimNode_TLLRN_ControlRig::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	// make sure the inputs on the node are evaluated before propagating the inputs
	GetEvaluateGraphExposedInputs().Execute(Context);

	// we also need access to the properties when running construction event
	PropagateInputProperties(Context.AnimInstanceProxy->GetAnimInstanceObject());

	// update the control rig instance just in case the dynamic control rig class has changed
	if(const UAnimInstance* AnimInstance = Cast<UAnimInstance>(Context.GetAnimInstanceObject()))
	{
		(void)UpdateTLLRN_ControlRigIfNeeded(AnimInstance, Context.AnimInstanceProxy->GetRequiredBones());
	}

	FAnimNode_TLLRN_ControlRigBase::CacheBones_AnyThread(Context);

	FBoneContainer& RequiredBones = Context.AnimInstanceProxy->GetRequiredBones();
	InputToControlIndex.Reset();

	if(RequiredBones.IsValid())
	{
		RefPoseSetterHash.Reset();

		auto CacheMapping = [&](const TMap<FName, FName>& InMapping, FCurveMappings& InCurveMappings,
			const FAnimationCacheBonesContext& InContext, UTLLRN_RigHierarchy* InHierarchy)
		{
			for (auto Iter = InMapping.CreateConstIterator(); Iter; ++Iter)
			{
				// we need to have list of variables using pin
				const FName SourcePath = Iter.Key();
				const FName TargetPath = Iter.Value();

				if (SourcePath != NAME_None && TargetPath != NAME_None)
				{
					InCurveMappings.Add(SourcePath, TargetPath);

					if(InHierarchy)
					{
						const FTLLRN_RigElementKey Key(TargetPath, ETLLRN_RigElementType::Control);
						if(const FTLLRN_RigControlElement* ControlElement = InHierarchy->Find<FTLLRN_RigControlElement>(Key))
						{
							InputToControlIndex.Add(TargetPath, ControlElement->GetIndex());
							continue;
						}
					}
				}

				// @todo: should we clear the item if not found?
			}
		};

		UTLLRN_RigHierarchy* Hierarchy = nullptr;
		if(UTLLRN_ControlRig* CurrentTLLRN_ControlRig = GetTLLRN_ControlRig())
		{
			Hierarchy = CurrentTLLRN_ControlRig->GetHierarchy();
		}

		CacheMapping(InputMapping, InputCurveMappings, Context, Hierarchy);
		CacheMapping(OutputMapping, OutputCurveMappings, Context, Hierarchy);
	}
}

void FAnimNode_TLLRN_ControlRig::Evaluate_AnyThread(FPoseContext & Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()
	ANIM_MT_SCOPE_CYCLE_COUNTER_VERBOSE(TLLRN_ControlRig, !IsInGameThread());

	// evaluate 
	FAnimNode_TLLRN_ControlRigBase::Evaluate_AnyThread(Output);
}

void FAnimNode_TLLRN_ControlRig::PostSerialize(const FArchive& Ar)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()
}

UClass* FAnimNode_TLLRN_ControlRig::GetTargetClass() const
{
	if(TLLRN_ControlRigClass)
	{
		return TLLRN_ControlRigClass;
	}
	return DefaultTLLRN_ControlRigClass;
}

void FAnimNode_TLLRN_ControlRig::UpdateInput(UTLLRN_ControlRig* InTLLRN_ControlRig, FPoseContext& InOutput)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FAnimNode_TLLRN_ControlRigBase::UpdateInput(InTLLRN_ControlRig, InOutput);

	// now go through variable mapping table and see if anything is mapping through input
	if (InputMapping.Num() > 0 && InTLLRN_ControlRig)
	{
		UE::Anim::FCurveUtils::BulkGet(InOutput.Curve, InputCurveMappings, 
			[&InTLLRN_ControlRig](const FTLLRN_ControlTLLRN_RigCurveMapping& InBulkElement, float InValue)
			{
				FRigVMExternalVariable Variable = InTLLRN_ControlRig->GetPublicVariableByName(InBulkElement.SourceName);
				if (!Variable.bIsReadOnly && Variable.TypeName == TEXT("float"))
				{
					Variable.SetValue<float>(InValue);
				}
				else
				{
					UE_LOG(LogAnimation, Warning, TEXT("[%s] Missing Input Variable [%s]"), *GetNameSafe(InTLLRN_ControlRig->GetClass()), *InBulkElement.SourceName.ToString());
				}
			});
	}
}

void FAnimNode_TLLRN_ControlRig::UpdateOutput(UTLLRN_ControlRig* InTLLRN_ControlRig, FPoseContext& InOutput)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FAnimNode_TLLRN_ControlRigBase::UpdateOutput(InTLLRN_ControlRig, InOutput);

	if (OutputMapping.Num() > 0 && InTLLRN_ControlRig)
	{
		UE::Anim::FCurveUtils::BulkSet(InOutput.Curve, OutputCurveMappings, 
			[&InTLLRN_ControlRig](const FTLLRN_ControlTLLRN_RigCurveMapping& InBulkElement) -> float
			{
				FRigVMExternalVariable Variable = InTLLRN_ControlRig->GetPublicVariableByName(InBulkElement.SourceName);
				if (Variable.TypeName == TEXT("float"))
				{
					return Variable.GetValue<float>();
				}
				else
				{
					UE_LOG(LogAnimation, Warning, TEXT("[%s] Missing Output Variable [%s]"), *GetNameSafe(InTLLRN_ControlRig->GetClass()), *InBulkElement.SourceName.ToString());
				}
				
				return 0.0f;
			});
	}
}

void FAnimNode_TLLRN_ControlRig::SetTLLRN_ControlRigClass(TSubclassOf<UTLLRN_ControlRig> InTLLRN_ControlRigClass)
{
	if(DefaultTLLRN_ControlRigClass == nullptr)
	{
		DefaultTLLRN_ControlRigClass = TLLRN_ControlRigClass;
	}

	// this may be setting an invalid runtime rig class,
	// which will be validated during UpdateTLLRN_ControlRigIfNeeded
	TLLRN_ControlRigClass = InTLLRN_ControlRigClass;
}

bool FAnimNode_TLLRN_ControlRig::UpdateTLLRN_ControlRigIfNeeded(const UAnimInstance* InAnimInstance, const FBoneContainer& InRequiredBones)
{
	if (UClass* ExpectedClass = GetTargetClass())
	{
		if(TLLRN_ControlRig != nullptr)
		{
			if(TLLRN_ControlRig->GetClass() != ExpectedClass)
			{
				UTLLRN_ControlRig* NewTLLRN_ControlRig = nullptr;

				auto ReportErrorAndSwitchToDefaultRig = [this, InAnimInstance, InRequiredBones, ExpectedClass](const FString& InMessage) -> bool
				{
					static constexpr TCHAR Format[] =  TEXT("[%s] Cannot switch to runtime rig class '%s' - reverting to default. %s");
					UE_LOG(LogTLLRN_ControlRig, Warning, Format, *InAnimInstance->GetPathName(), *ExpectedClass->GetName(), *InMessage);

					// mark the class to be known - and nullptr - indicating that it is not supported.
					TLLRN_ControlRigPerClass.FindOrAdd(ExpectedClass, nullptr);

					// fall back to the default control rig and switch to that
					TLLRN_ControlRigClass = nullptr;
					
					return UpdateTLLRN_ControlRigIfNeeded(InAnimInstance, InRequiredBones);
				};

				// if we are reacting to a programmatic change
				// we need to perform validation between the two control rigs (old and new)
				if((TLLRN_ControlRigClass == ExpectedClass) &&
					DefaultTLLRN_ControlRigClass &&
					(ExpectedClass != DefaultTLLRN_ControlRigClass))
				{
					// check if we already created this before
					if(const TObjectPtr<UTLLRN_ControlRig>* ExistingTLLRN_ControlRig = TLLRN_ControlRigPerClass.Find(ExpectedClass))
					{
						NewTLLRN_ControlRig = *ExistingTLLRN_ControlRig;

						// the existing control rig is nullptr indicates that the class is not supported.
						// the warning will have been logged before - so it's not required to log it again.
						if(NewTLLRN_ControlRig == nullptr)
						{
							// fall back to the default control rig and switch to that
							TLLRN_ControlRigClass = nullptr;
							return UpdateTLLRN_ControlRigIfNeeded(InAnimInstance, InRequiredBones);
						}
					}
					else
					{
						if(ExpectedClass->IsNative())
						{
							static constexpr TCHAR Format[] = TEXT("Class '%s' is not supported (is it native).");
							return ReportErrorAndSwitchToDefaultRig(FString::Printf(Format, *ExpectedClass->GetName()));
						}
						
						// compare the two classes and make sure that the expected class is a super set in terms of
						// user defined properties.
						for (TFieldIterator<FProperty> PropertyIt(TLLRN_ControlRigClass); PropertyIt; ++PropertyIt)
						{
							const FProperty* OldProperty = *PropertyIt;
							if(OldProperty->IsNative())
							{
								continue;
							}

							const FProperty* NewProperty = ExpectedClass->FindPropertyByName(OldProperty->GetFName());
							if(NewProperty == nullptr)
							{
								static constexpr TCHAR Format[] = TEXT("Property / Variable '%s' is missing.");
								return ReportErrorAndSwitchToDefaultRig(FString::Printf(Format, *OldProperty->GetName()));
							}

							if(!NewProperty->SameType(OldProperty))
							{
								FString OldExtendedCPPType, NewExtendedCPPType;
								const FString OldCPPType = OldProperty->GetCPPType(&OldExtendedCPPType);
								const FString NewCPPType = NewProperty->GetCPPType(&NewExtendedCPPType);
								static constexpr TCHAR Format[] = TEXT("Property / Variable '%s' has incorrect type (is '%s', expected '%s').");
								return ReportErrorAndSwitchToDefaultRig(FString::Printf(Format, *NewProperty->GetName(), *(NewCPPType + NewExtendedCPPType), *(OldCPPType + OldExtendedCPPType)));
							}
						}
						
						// create a new control rig using the new class
						{
							// Let's make sure the GC isn't running when we try to create a new Control Rig.
							FGCScopeGuard GCGuard;
							
							NewTLLRN_ControlRig = NewObject<UTLLRN_ControlRig>(InAnimInstance->GetOwningComponent(), ExpectedClass);
							
							// If the object was created on a non-game thread, clear the async flag immediately, so that it can be
							// garbage collected in the future. 
							(void)NewTLLRN_ControlRig->AtomicallyClearInternalFlags(EInternalObjectFlags::Async);
														
							TLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
							TLLRN_ControlRig->GetObjectBinding()->BindToObject(InAnimInstance->GetOwningComponent());
							NewTLLRN_ControlRig->Initialize(true);
							NewTLLRN_ControlRig->RequestInit();
						}

						// temporarily set the new control rig to be the target instance
						TGuardValue<TObjectPtr<UObject>> TargetInstanceGuard(TargetInstance, NewTLLRN_ControlRig);

						// propagate all variable inputs
						PropagateInputProperties(InAnimInstance);

						// run construction on the rig
						NewTLLRN_ControlRig->Execute(FTLLRN_RigUnit_PrepareForExecution::EventName);

						const UTLLRN_RigHierarchy* OldHierarchy = TLLRN_ControlRig->GetHierarchy();
						const UTLLRN_RigHierarchy* NewHierarchy = NewTLLRN_ControlRig->GetHierarchy();

						// now compare the two rigs - we need to check bone hierarchy compatibility.
						const TArray<FTLLRN_RigElementKey> OldBoneKeys = OldHierarchy->GetBoneKeys(false);
						const TArray<FTLLRN_RigElementKey> NewBoneKeys = NewHierarchy->GetBoneKeys(false);
						for(const FTLLRN_RigElementKey& BoneKey : OldBoneKeys)
						{
							if(!NewBoneKeys.Contains(BoneKey))
							{
								static constexpr TCHAR Format[] = TEXT("Bone '%s' is missing from the rig.");
								return ReportErrorAndSwitchToDefaultRig(FString::Printf(Format, *BoneKey.Name.ToString()));
							}
						}

						// we also need to check curve hierarchy compatibility.
						const TArray<FTLLRN_RigElementKey> OldCurveKeys = OldHierarchy->GetCurveKeys();
						const TArray<FTLLRN_RigElementKey> NewCurveKeys = NewHierarchy->GetCurveKeys();
						for(const FTLLRN_RigElementKey& CurveKey : OldCurveKeys)
						{
							if(!NewCurveKeys.Contains(CurveKey))
							{
								static constexpr TCHAR Format[] = TEXT("Curve '%s' is missing from the rig.");
								return ReportErrorAndSwitchToDefaultRig(FString::Printf(Format, *CurveKey.Name.ToString()));
							}
						}

						// we also need to check that potentially exposed controls match
						for (int32 PropIdx = 0; PropIdx < DestPropertyNames.Num(); ++PropIdx)
						{
							if(const FTLLRN_RigControlElement* OldControlElement = TLLRN_ControlRig->FindControl(DestPropertyNames[PropIdx]))
							{
								const FTLLRN_RigControlElement* NewControlElement = NewTLLRN_ControlRig->FindControl(DestPropertyNames[PropIdx]);
								if(NewControlElement == nullptr)
								{
									static constexpr TCHAR Format[] = TEXT("Control '%s' is missing from the rig.");
									return ReportErrorAndSwitchToDefaultRig(FString::Printf(Format, *DestPropertyNames[PropIdx].ToString()));
								}

								if(NewControlElement->Settings.ControlType != OldControlElement->Settings.ControlType)
								{
									static const UEnum* ControlTypeEnum = StaticEnum<ETLLRN_RigControlType>();
									const FString OldType = ControlTypeEnum->GetDisplayNameTextByValue((int64)OldControlElement->Settings.ControlType).ToString();
									const FString NewType = ControlTypeEnum->GetDisplayNameTextByValue((int64)NewControlElement->Settings.ControlType).ToString();
									static constexpr TCHAR Format[] = TEXT("Control '%s' has the incorrect type (is '%s', expected '%s').");
									return ReportErrorAndSwitchToDefaultRig(FString::Printf(Format, *DestPropertyNames[PropIdx].ToString(), *NewType, *OldType));
								}
							}
						}

						// fall through: we have a compatible new control rig, let's just use that.
						TLLRN_ControlRigPerClass.FindOrAdd(NewTLLRN_ControlRig->GetClass(), NewTLLRN_ControlRig);
					}
				}

				// stop listening to the rig, store it for reuse
				TLLRN_ControlRig->OnInitialized_AnyThread().RemoveAll(this);
				TLLRN_ControlRigPerClass.FindOrAdd(TLLRN_ControlRig->GetClass(), TLLRN_ControlRig);
				TLLRN_ControlRig = nullptr;

				if(NewTLLRN_ControlRig)
				{
					Swap(NewTLLRN_ControlRig, TLLRN_ControlRig);
				}
			}
			else
			{
				// we have a control rig of the right class
				return false;
			}
		}

		if(TLLRN_ControlRig == nullptr)
		{
			// Let's make sure the GC isn't running when we try to create a new Control Rig.
			FGCScopeGuard GCGuard;
			
			TLLRN_ControlRig = NewObject<UTLLRN_ControlRig>(InAnimInstance->GetOwningComponent(), ExpectedClass);
			(void)TLLRN_ControlRig->AtomicallyClearInternalFlags(EInternalObjectFlags::Async);
			
			TLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
			TLLRN_ControlRig->GetObjectBinding()->BindToObject(InAnimInstance->GetOwningComponent());
			TLLRN_ControlRig->Initialize(true);
			TLLRN_ControlRig->RequestInit();
		}
		RefPoseSetterHash.Reset();
		TLLRN_ControlRig->OnInitialized_AnyThread().AddRaw(this, &FAnimNode_TLLRN_ControlRig::HandleOnInitialized_AnyThread);

		UpdateInputOutputMappingIfRequired(TLLRN_ControlRig, InRequiredBones);

		return true;
	}
	return false;
}

void FAnimNode_TLLRN_ControlRig::UpdateTLLRN_ControlRigRefPoseIfNeeded(const FAnimInstanceProxy* InProxy, bool bIncludePoseInHash)
{
	if(!bSetRefPoseFromSkeleton)
	{
		return;
	}

	int32 ExpectedHash = 0;
	ExpectedHash = HashCombine(ExpectedHash, (int32)reinterpret_cast<uintptr_t>(InProxy->GetAnimInstanceObject()));
	ExpectedHash = HashCombine(ExpectedHash, (int32)reinterpret_cast<uintptr_t>(InProxy->GetSkelMeshComponent()));

	if(InProxy->GetSkelMeshComponent())
	{
		ExpectedHash = HashCombine(ExpectedHash, (int32)reinterpret_cast<uintptr_t>(InProxy->GetSkelMeshComponent()->GetSkeletalMeshAsset()));
	}

	if(bIncludePoseInHash)
	{
		FMemMark Mark(FMemStack::Get());
		FCompactPose RefPose;
		RefPose.ResetToRefPose(InProxy->GetRequiredBones());

		for(const FCompactPoseBoneIndex& BoneIndex : RefPose.ForEachBoneIndex())
		{
			const FTransform& Transform = RefPose.GetRefPose(BoneIndex);
			const FQuat Rotation = Transform.GetRotation();

			ExpectedHash = HashCombine(ExpectedHash, GetTypeHash(Transform.GetTranslation()));
			ExpectedHash = HashCombine(ExpectedHash, GetTypeHash(FVector4(Rotation.X, Rotation.Y, Rotation.Z, Rotation.W)));
			ExpectedHash = HashCombine(ExpectedHash, GetTypeHash(Transform.GetScale3D()));
		}

		if(RefPoseSetterHash.IsSet() && (ExpectedHash == RefPoseSetterHash.GetValue()))
		{
			return;
		}

		TLLRN_ControlRig->SetBoneInitialTransformsFromCompactPose(&RefPose);
	}
	else
	{
		if(RefPoseSetterHash.IsSet() && (ExpectedHash == RefPoseSetterHash.GetValue()))
		{
			return;
		}

		TLLRN_ControlRig->SetBoneInitialTransformsFromAnimInstanceProxy(InProxy);
	}
	
	RefPoseSetterHash = ExpectedHash;
}

void FAnimNode_TLLRN_ControlRig::SetIOMapping(bool bInput, const FName& SourceProperty, const FName& TargetCurve)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	UClass* TargetClass = GetTargetClass();
	if (TargetClass)
	{
		UTLLRN_ControlRig* CDO = TargetClass->GetDefaultObject<UTLLRN_ControlRig>();
		if (CDO)
		{
			TMap<FName, FName>& MappingData = (bInput) ? InputMapping : OutputMapping;

			// if it's valid as of now, we add it
			bool bIsReadOnly = CDO->GetPublicVariableByName(SourceProperty).bIsReadOnly;
			if (!bInput || !bIsReadOnly)
			{
				if (TargetCurve == NAME_None)
				{
					MappingData.Remove(SourceProperty);
				}
				else
				{
					MappingData.FindOrAdd(SourceProperty) = TargetCurve;
				}
			}
		}
	}
}

FName FAnimNode_TLLRN_ControlRig::GetIOMapping(bool bInput, const FName& SourceProperty) const
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	const TMap<FName, FName>& MappingData = (bInput) ? InputMapping : OutputMapping;
	if (const FName* NameFound = MappingData.Find(SourceProperty))
	{
		return *NameFound;
	}

	return NAME_None;
}

void FAnimNode_TLLRN_ControlRig::InitializeProperties(const UObject* InSourceInstance, UClass* InTargetClass)
{
	// Build property lists
	SourceProperties.Reset(SourcePropertyNames.Num());
	DestProperties.Reset(SourcePropertyNames.Num());

	check(SourcePropertyNames.Num() == DestPropertyNames.Num());

	for (int32 Idx = 0; Idx < SourcePropertyNames.Num(); ++Idx)
	{
		FName& SourceName = SourcePropertyNames[Idx];
		UClass* SourceClass = InSourceInstance->GetClass();

		FProperty* SourceProperty = FindFProperty<FProperty>(SourceClass, SourceName);
		SourceProperties.Add(SourceProperty);
		DestProperties.Add(nullptr);
	}
}

void FAnimNode_TLLRN_ControlRig::PropagateInputProperties(const UObject* InSourceInstance)
{
	if (TargetInstance)
	{
		UTLLRN_ControlRig* TargetTLLRN_ControlRig = Cast<UTLLRN_ControlRig>((UObject*)TargetInstance);
		if(TargetTLLRN_ControlRig == nullptr)
		{
			return;
		}
		UTLLRN_RigHierarchy* TargetHierarchy = TargetTLLRN_ControlRig->GetHierarchy();
		if(TargetHierarchy == nullptr)
		{
			return;
		}
		
		// First copy properties
		check(SourceProperties.Num() == DestProperties.Num());
		for (int32 PropIdx = 0; PropIdx < SourceProperties.Num(); ++PropIdx)
		{
			FProperty* CallerProperty = SourceProperties[PropIdx];

			if(FTLLRN_RigControlElement* ControlElement = TargetTLLRN_ControlRig->FindControl(DestPropertyNames[PropIdx]))
			{
				const uint8* SrcPtr = CallerProperty->ContainerPtrToValuePtr<uint8>(InSourceInstance);

				bool bIsValid = false;
				FTLLRN_RigControlValue Value;
				switch(ControlElement->Settings.ControlType)
				{
					case ETLLRN_RigControlType::Bool:
					{
						if(ensure(CastField<FBoolProperty>(CallerProperty)))
						{
							Value = FTLLRN_RigControlValue::Make<bool>(*(bool*)SrcPtr);
							bIsValid = true;
						}
						break;
					}
					case ETLLRN_RigControlType::Float:
					case ETLLRN_RigControlType::ScaleFloat:
					{
						if(ensure(CastField<FFloatProperty>(CallerProperty)))
						{
							Value = FTLLRN_RigControlValue::Make<float>(*(float*)SrcPtr);
							bIsValid = true;
						}
						break;
					}
					case ETLLRN_RigControlType::Integer:
					{
						if(ensure(CastField<FIntProperty>(CallerProperty)))
						{
							Value = FTLLRN_RigControlValue::Make<int32>(*(int32*)SrcPtr);
							bIsValid = true;
						}
						break;
					}
					case ETLLRN_RigControlType::Vector2D:
					{
						FStructProperty* StructProperty = CastField<FStructProperty>(CallerProperty);
						if(ensure(StructProperty))
						{
							if(ensure(StructProperty->Struct == TBaseStructure<FVector2D>::Get()))
							{
								const FVector2D& SrcVector = *(FVector2D*)SrcPtr;
								Value = FTLLRN_RigControlValue::Make<FVector2D>(SrcVector);
								bIsValid = true;
							}
						}
						break;
					}
					case ETLLRN_RigControlType::Position:
					case ETLLRN_RigControlType::Scale:
					{
						FStructProperty* StructProperty = CastField<FStructProperty>(CallerProperty);
						if(ensure(StructProperty))
						{
							if(ensure(StructProperty->Struct == TBaseStructure<FVector>::Get()))
							{
								const FVector& SrcVector = *(FVector*)SrcPtr;  
								Value = FTLLRN_RigControlValue::Make<FVector>(SrcVector);
								bIsValid = true;
							}
						}
						break;
					}
					case ETLLRN_RigControlType::Rotator:
					{
						FStructProperty* StructProperty = CastField<FStructProperty>(CallerProperty);
						if(ensure(StructProperty))
						{
							if(ensure(StructProperty->Struct == TBaseStructure<FRotator>::Get()))
							{
								const FRotator& SrcRotator = *(FRotator*)SrcPtr;
								Value = FTLLRN_RigControlValue::Make<FRotator>(SrcRotator);
								bIsValid = true;
							}
						}
						break;
					}
					case ETLLRN_RigControlType::Transform:
					{
						FStructProperty* StructProperty = CastField<FStructProperty>(CallerProperty);
						if(ensure(StructProperty))
						{
							if(ensure(StructProperty->Struct == TBaseStructure<FTransform>::Get()))
							{
								const FTransform& SrcTransform = *(FTransform*)SrcPtr;  
								Value = FTLLRN_RigControlValue::Make<FTransform>(SrcTransform);
								bIsValid = true;
							}
						}
						break;
					}
					case ETLLRN_RigControlType::TransformNoScale:
					{
						FStructProperty* StructProperty = CastField<FStructProperty>(CallerProperty);
						if(ensure(StructProperty))
						{
							if(ensure(StructProperty->Struct == TBaseStructure<FTransform>::Get()))
							{
								const FTransform& SrcTransform = *(FTransform*)SrcPtr;  
								Value = FTLLRN_RigControlValue::Make<FTransformNoScale>(SrcTransform);
								bIsValid = true;
							}
						}
						break;
					}
					case ETLLRN_RigControlType::EulerTransform:
					{
						FStructProperty* StructProperty = CastField<FStructProperty>(CallerProperty);
						if(ensure(StructProperty))
						{
							if(ensure(StructProperty->Struct == TBaseStructure<FTransform>::Get()))
							{
								const FTransform& SrcTransform = *(FTransform*)SrcPtr;  
								Value = FTLLRN_RigControlValue::Make<FEulerTransform>(FEulerTransform(SrcTransform));
								bIsValid = true;
							}
						}
						break;
					}
					default:
					{
						checkNoEntry();
					}
				}

				if(bIsValid)
				{
					TargetHierarchy->SetControlValue(ControlElement, Value, ETLLRN_RigControlValueType::Current);
				}
				continue;
			}

			FRigVMExternalVariable Variable = TargetTLLRN_ControlRig->GetPublicVariableByName(DestPropertyNames[PropIdx]);
			if(Variable.IsValid())
			{
				if (Variable.bIsReadOnly)
				{
					continue;
				}

				const uint8* SrcPtr = CallerProperty->ContainerPtrToValuePtr<uint8>(InSourceInstance);

				if (CastField<FBoolProperty>(CallerProperty) != nullptr && Variable.TypeName == RigVMTypeUtils::BoolTypeName)
				{
					const bool Value = *(const bool*)SrcPtr;
					Variable.SetValue<bool>(Value);
				}
				else if (CastField<FFloatProperty>(CallerProperty) != nullptr && (Variable.TypeName == RigVMTypeUtils::FloatTypeName ||  Variable.TypeName == RigVMTypeUtils::DoubleTypeName))
				{
					const float Value = *(const float*)SrcPtr;
					if(Variable.TypeName == RigVMTypeUtils::FloatTypeName)
					{
						Variable.SetValue<float>(Value);
					}
					else
					{
						Variable.SetValue<double>(Value);
					}
				}
				else if (CastField<FDoubleProperty>(CallerProperty) != nullptr && (Variable.TypeName == RigVMTypeUtils::FloatTypeName ||  Variable.TypeName == RigVMTypeUtils::DoubleTypeName))
				{
					const double Value = *(const double*)SrcPtr;
					if(Variable.TypeName == RigVMTypeUtils::FloatTypeName)
					{
						Variable.SetValue<float>((float)Value);
					}
					else
					{
						Variable.SetValue<double>(Value);
					}
				}
				else if (CastField<FIntProperty>(CallerProperty) != nullptr && Variable.TypeName == RigVMTypeUtils::Int32TypeName)
				{
					const int32 Value = *(const int32*)SrcPtr;
					Variable.SetValue<int32>(Value);
				}
				else if (CastField<FNameProperty>(CallerProperty) != nullptr && Variable.TypeName == RigVMTypeUtils::FNameTypeName)
				{
					const FName Value = *(const FName*)SrcPtr;
					Variable.SetValue<FName>(Value);
				}
				else if (CastField<FNameProperty>(CallerProperty) != nullptr && Variable.TypeName == RigVMTypeUtils::FStringTypeName)
				{
					const FString Value = *(const FString*)SrcPtr;
					Variable.SetValue<FString>(Value);
				}
				else if (FStructProperty* StructProperty = CastField<FStructProperty>(CallerProperty))
				{
					if (StructProperty->Struct == Variable.TypeObject)
					{
						StructProperty->Struct->CopyScriptStruct(Variable.Memory, SrcPtr, 1);
					}
				}
				else if(FArrayProperty* ArrayProperty = CastField<FArrayProperty>(CallerProperty))
				{
					if(ensure(ArrayProperty->SameType(Variable.Property)))
					{
						ArrayProperty->CopyCompleteValue(Variable.Memory, SrcPtr);
					}
				}
				else if(FObjectProperty* ObjectProperty = CastField<FObjectProperty>(CallerProperty))
				{
					if(ensure(ObjectProperty->SameType(Variable.Property)))
					{
						ObjectProperty->CopyCompleteValue(Variable.Memory, SrcPtr);
					}
				}
				else if(FEnumProperty* EnumProperty = CastField<FEnumProperty>(CallerProperty))
				{
					if(ensure(EnumProperty->SameType(Variable.Property)))
					{
						EnumProperty->CopyCompleteValue(Variable.Memory, SrcPtr);
					}
				}
				else
				{
					ensureMsgf(false, TEXT("Property %s type %s not recognized"), *CallerProperty->GetName(), *CallerProperty->GetCPPType());
				}
			}
		}
	}
}

#if WITH_EDITOR

void FAnimNode_TLLRN_ControlRig::HandleObjectsReinstanced_Impl(UObject* InSourceObject, UObject* InTargetObject, const TMap<UObject*, UObject*>& OldToNewInstanceMap)
{
	Super::HandleObjectsReinstanced_Impl(InSourceObject, InTargetObject, OldToNewInstanceMap);
	
	if(TLLRN_ControlRig)
	{
		TLLRN_ControlRig->OnInitialized_AnyThread().RemoveAll(this);
		TLLRN_ControlRig->OnInitialized_AnyThread().AddRaw(this, &FAnimNode_TLLRN_ControlRig::HandleOnInitialized_AnyThread);
	}
}

#endif