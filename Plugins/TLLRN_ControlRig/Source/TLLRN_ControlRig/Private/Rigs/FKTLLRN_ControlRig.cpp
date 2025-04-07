// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/FKTLLRN_ControlRig.h"
#include "Animation/SmartName.h"
#include "Engine/SkeletalMesh.h"
#include "ITLLRN_ControlRigObjectBinding.h"
#include "Components/SkeletalMeshComponent.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"
#include "Settings/TLLRN_ControlRigSettings.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Execution/TLLRN_RigUnit_InverseExecution.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FKTLLRN_ControlRig)

#define LOCTEXT_NAMESPACE "OverrideTLLRN_ControlRig"

UFKTLLRN_ControlRig::UFKTLLRN_ControlRig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ApplyMode(ETLLRN_ControlRigFKRigExecuteMode::Replace)
	, CachedToggleApplyMode(ETLLRN_ControlRigFKRigExecuteMode::Replace)
{
	bCopyHierarchyBeforeConstruction = false;
	bResetInitialTransformsBeforeConstruction = false;
}

FName UFKTLLRN_ControlRig::GetControlName(const FName& InName, const ETLLRN_RigElementType& InType)
{
	if (InName != NAME_None)
	{
		if((InType == ETLLRN_RigElementType::Bone || InType == ETLLRN_RigElementType::Curve))
		{
			static thread_local TMap<FName, FName> NameToControlMappings[2];
			TMap<FName, FName>& NameToControlMapping = NameToControlMappings[InType == ETLLRN_RigElementType::Bone ? 0 : 1];
			if (const FName* CachedName = NameToControlMapping.Find(InName))
			{
				return *CachedName;
			}
			else
			{
				static thread_local FStringBuilderBase ScratchString;		

				// ToString performs ScratchString.Reset() internally
				InName.ToString(ScratchString);

				if (InType == ETLLRN_RigElementType::Curve)
				{
					static const TCHAR* CurvePostFix = TEXT("_CURVE");
					ScratchString.Append(CurvePostFix);
				}

				static const TCHAR* ControlPostFix = TEXT("_CONTROL");
				ScratchString.Append(ControlPostFix);
				return NameToControlMapping.Add(InName, FName(*ScratchString));
			}
		}
		else if (InType == ETLLRN_RigElementType::Control)
		{
			checkSlow(InName.ToString().Contains(TEXT("_CONTROL")));
			return InName;
		}	
	}

	// if control name is coming as none, we don't append the postfix
	return NAME_None;
}

FName UFKTLLRN_ControlRig::GetControlTargetName(const FName& InName, const ETLLRN_RigElementType& InType)
{
	if (InName != NAME_None)
	{
		check(InType == ETLLRN_RigElementType::Bone || InType == ETLLRN_RigElementType::Curve);
		static thread_local TMap<FName, FName> NameToTargetMappings[2];
		TMap<FName, FName>& NameToTargetMapping = NameToTargetMappings[InType == ETLLRN_RigElementType::Bone ? 0 : 1];
		if (const FName* CachedName = NameToTargetMapping.Find(InName))
		{
			return *CachedName;
		}
		else
		{
			static thread_local FString ScratchString;
			
			// ToString performs ScratchString.Reset() internally
			InName.ToString(ScratchString);

			int32 StartPostFix;
			if (InType == ETLLRN_RigElementType::Curve)
			{
				static const TCHAR* CurvePostFix = TEXT("_CURVE_CONTROL");
				StartPostFix = ScratchString.Find(CurvePostFix, ESearchCase::CaseSensitive);
			}
			else
			{
				static const TCHAR* ControlPostFix = TEXT("_CONTROL");
				StartPostFix = ScratchString.Find(ControlPostFix, ESearchCase::CaseSensitive);
			}

			const FStringView ControlTargetString(*ScratchString, StartPostFix != INDEX_NONE ? StartPostFix : ScratchString.Len());
			return NameToTargetMapping.Add(InName, FName(ControlTargetString));
		}
	}

	// If incoming name is NONE, or not correctly formatted according to expected post-fix return NAME_None
	return NAME_None;
}

bool UFKTLLRN_ControlRig::Execute_Internal(const FName& InEventName)
{
#if WITH_EDITOR
	if(UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		if(Hierarchy->IsTracingChanges())
		{
			Hierarchy->StorePoseForTrace(TEXT("UFKTLLRN_ControlRig::BeginExecuteUnits"));
		}
	}
#endif	

	if (InEventName == FTLLRN_RigUnit_BeginExecution::EventName)
	{
		GetHierarchy()->ForEach<FTLLRN_RigBoneElement>([&](FTLLRN_RigBoneElement* BoneElement) -> bool
        {
			const FName ControlName = GetControlName(BoneElement->GetFName(), BoneElement->GetType());
			const FTLLRN_RigElementKey ControlKey(ControlName, ETLLRN_RigElementType::Control);
			const int32 ControlIndex = GetHierarchy()->GetIndex(ControlKey);
			if (ControlIndex != INDEX_NONE && GetControlActive(ControlIndex))
			{
				FTLLRN_RigControlElement* Control = GetHierarchy()->Get<FTLLRN_RigControlElement>(ControlIndex);
				const FTransform LocalTransform = GetHierarchy()->GetLocalTransform(ControlIndex);
				switch (ApplyMode)
				{
					case ETLLRN_ControlRigFKRigExecuteMode::Replace:
					{
						const FTransform OffsetTransform = GetHierarchy()->GetControlOffsetTransform(Control, ETLLRN_RigTransformType::InitialLocal);
						FTransform Transform = LocalTransform * OffsetTransform;
						Transform.NormalizeRotation();
						GetHierarchy()->SetTransform(BoneElement, Transform, ETLLRN_RigTransformType::CurrentLocal, true, false);
						break;
					}
					case ETLLRN_ControlRigFKRigExecuteMode::Additive:
					{
						const FTransform PreviousTransform = GetHierarchy()->GetTransform(BoneElement, ETLLRN_RigTransformType::CurrentLocal);
						FTransform Transform = LocalTransform * PreviousTransform;
						Transform.NormalizeRotation();
						GetHierarchy()->SetTransform(BoneElement, Transform, ETLLRN_RigTransformType::CurrentLocal, true, false);
						break;
					}
					case ETLLRN_ControlRigFKRigExecuteMode::Direct:
					{
						GetHierarchy()->SetTransform(BoneElement, LocalTransform, ETLLRN_RigTransformType::CurrentLocal, true, false);
						break;
					}
				}
			}
			return true;
		});

		GetHierarchy()->ForEach<FTLLRN_RigCurveElement>([&](FTLLRN_RigCurveElement* CurveElement) -> bool
        {
			const FName ControlName = GetControlName(CurveElement->GetFName(), CurveElement->GetType());
			const FTLLRN_RigElementKey ControlKey(ControlName, ETLLRN_RigElementType::Control);
			const int32 ControlIndex = GetHierarchy()->GetIndex(ControlKey);
			
			if (ControlIndex != INDEX_NONE && GetControlActive(ControlIndex))
			{
				const float CurveValue = GetHierarchy()->GetControlValue(ControlIndex).Get<float>();

				switch (ApplyMode)
				{
					case ETLLRN_ControlRigFKRigExecuteMode::Replace:
					case ETLLRN_ControlRigFKRigExecuteMode::Direct:
					{
						GetHierarchy()->SetCurveValue(CurveElement, CurveValue, false /*bSetupUndo*/);
						break;
					}
					case ETLLRN_ControlRigFKRigExecuteMode::Additive:
					{
						const float PreviousValue = GetHierarchy()->GetCurveValue(CurveElement);
						GetHierarchy()->SetCurveValue(CurveElement, PreviousValue + CurveValue, false /*bSetupUndo*/);
						break;
					}
				}
			}
			return true;
		});
	}
	else if (InEventName == FTLLRN_RigUnit_InverseExecution::EventName)
	{
		const bool bNotify = true;
		const FTLLRN_RigControlModifiedContext Context = FTLLRN_RigControlModifiedContext();
		const bool bSetupUndo = false;

		GetHierarchy()->Traverse([&](FTLLRN_RigBaseElement* InElement, bool& bContinue)
		{
			if(!InElement->IsA<FTLLRN_RigBoneElement>())
			{
				bContinue = false;
				return;
			}

			FTLLRN_RigBoneElement* BoneElement = CastChecked<FTLLRN_RigBoneElement>(InElement);

			const FName ControlName = GetControlName(BoneElement->GetFName(), BoneElement->GetType());
			const FTLLRN_RigElementKey ControlKey(ControlName, ETLLRN_RigElementType::Control);
			const int32 ControlIndex = GetHierarchy()->GetIndex(ControlKey);
			
			switch (ApplyMode)
			{
				case ETLLRN_ControlRigFKRigExecuteMode::Replace:
				case ETLLRN_ControlRigFKRigExecuteMode::Additive:
				{
					// during inversion we assume Replace Mode
					FTLLRN_RigControlElement* Control = GetHierarchy()->GetChecked<FTLLRN_RigControlElement>(ControlIndex);
					const FTransform Offset = GetHierarchy()->GetControlOffsetTransform(Control, ETLLRN_RigTransformType::InitialLocal);
					const FTransform Current = GetHierarchy()->GetTransform(BoneElement, ETLLRN_RigTransformType::CurrentLocal);
			
					FTransform Transform = Current.GetRelativeTransform(Offset);
					Transform.NormalizeRotation();

					SetControlValue(ControlName, FTLLRN_RigControlValue::Make(FEulerTransform(Transform)), bNotify, Context, bSetupUndo, false, true);
					break;
				}

				case ETLLRN_ControlRigFKRigExecuteMode::Direct:
				{
					FTransform Transform = GetHierarchy()->GetTransform(BoneElement, ETLLRN_RigTransformType::CurrentLocal);
					Transform.NormalizeRotation();
					SetControlValue(ControlName, FTLLRN_RigControlValue::Make(FEulerTransform(Transform)), bNotify, Context, bSetupUndo, false, true);

					break;
				}
			}

		}, true);

		GetHierarchy()->ForEach<FTLLRN_RigCurveElement>([&](FTLLRN_RigCurveElement* CurveElement) -> bool
        {
			const FName ControlName = GetControlName(CurveElement->GetFName(), CurveElement->GetType());
			const FTLLRN_RigElementKey ControlKey(ControlName, ETLLRN_RigElementType::Control);
			const int32 ControlIndex = GetHierarchy()->GetIndex(ControlKey);

			const float CurveValue = GetHierarchy()->GetCurveValue(CurveElement);
			SetControlValue(ControlName, FTLLRN_RigControlValue::Make(CurveValue), bNotify, Context, bSetupUndo);

			return true;
		});
	}

#if WITH_EDITOR
	if(UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		if(Hierarchy->IsTracingChanges())
		{
			Hierarchy->StorePoseForTrace(TEXT("UFKTLLRN_ControlRig::EndExecuteUnits"));
			Hierarchy->DumpTransformStackToFile();
		}
	}
#endif
	
	return true;
}

void UFKTLLRN_ControlRig::SetBoneInitialTransformsFromSkeletalMeshComponent(USkeletalMeshComponent* InSkelMeshComp,
                                                                      bool bUseAnimInstance)
{
	Super::SetBoneInitialTransformsFromSkeletalMeshComponent(InSkelMeshComp, bUseAnimInstance);

#if WITH_EDITOR
	if(UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		if(Hierarchy->IsTracingChanges())
		{
			Hierarchy->StorePoseForTrace(TEXT("UFKTLLRN_ControlRig::SetBoneInitialTransformsFromSkeletalMeshComponent"));
		}
	}
#endif	

	SetControlOffsetsFromBoneInitials();
}

void UFKTLLRN_ControlRig::Initialize(bool bInitTLLRN_RigUnits /*= true*/)
{
	PostInitInstanceIfRequired();

	Super::Initialize(bInitTLLRN_RigUnits);

	if(const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = GetObjectBinding())
	{
		// we do this after Initialize because Initialize will copy from CDO. 
		// create hierarchy from the incoming skeleton
		if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Binding->GetBoundObject()))
		{
			CreateTLLRN_RigElements(SkeletalMeshComponent->GetSkeletalMeshAsset());
		}
		else if (USkeleton* Skeleton = Cast<USkeleton>(Binding->GetBoundObject()))
		{
			const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
			CreateTLLRN_RigElements(RefSkeleton, Skeleton);
		}
	}
}

TArray<FName> UFKTLLRN_ControlRig::GetControlNames()
{
	TArray<FTLLRN_RigControlElement*> Controls;
	GetControlsInOrder(Controls);

	TArray<FName> Names;
	for (FTLLRN_RigControlElement* ControlElement : Controls) 
	{
		Names.Add(ControlElement->GetFName());
	}
	return Names;
}

bool UFKTLLRN_ControlRig::GetControlActive(int32 Index) const
{
	if (Index >= 0 && Index < IsControlActive.Num())
	{
		return IsControlActive[Index];
	}
	return false;
}

void UFKTLLRN_ControlRig::SetControlActive(int32 Index, bool bActive)
{
	if (Index >= 0 && Index < IsControlActive.Num())
	{
		IsControlActive[Index] = bActive;
	}
}

void UFKTLLRN_ControlRig::SetControlActive(const TArray<FFKBoneCheckInfo>& BoneChecks)
{
	for (const FFKBoneCheckInfo& Info : BoneChecks)
	{
		SetControlActive(Info.BoneID, Info.bActive);
	}
}

void UFKTLLRN_ControlRig::CreateTLLRN_RigElements(const FReferenceSkeleton& InReferenceSkeleton, const USkeleton* InSkeleton)
{
	PostInitInstanceIfRequired();

	GetHierarchy()->Reset();
	if (UTLLRN_RigHierarchyController* Controller = GetHierarchy()->GetController(true))
	{
		Controller->ImportBones(InReferenceSkeleton, NAME_None, false, false, true, false);

		if (InitializationOptions.bGenerateCurveControls)
		{
			// try to add curves for manually specified names
			if (InitializationOptions.CurveNames.Num() > 0)
            {
            	for (int32 Index = 0; Index < InitializationOptions.CurveNames.Num(); ++Index)
            	{
            		Controller->AddCurve(InitializationOptions.CurveNames[Index], 0.f, false);
            	}
            }
			// add all curves found on the skeleton
            else if (InSkeleton && InitializationOptions.bImportCurves)
            {
            	InSkeleton->ForEachCurveMetaData([Controller](const FName& InCurveName, const FCurveMetaData& InMetaData)
				{
					Controller->AddCurve(InCurveName, 0.f, false);
				});
            }
		}

		// add control for all bone hierarchy 
		if (InitializationOptions.bGenerateBoneControls)
		{
			auto CreateControlForBoneElement = [this, Controller](const FTLLRN_RigBoneElement* BoneElement)
			{
				const FName BoneName = BoneElement->GetFName();
				const FName ControlName = GetControlName(BoneName, BoneElement->GetType());
				const FTLLRN_RigElementKey ParentKey = GetHierarchy()->GetFirstParent(BoneElement->GetKey());

				FTLLRN_RigControlSettings Settings;
				Settings.ControlType = ETLLRN_RigControlType::EulerTransform;
				Settings.DisplayName = BoneName;
					
				Controller->AddControl(ControlName, ParentKey, Settings, FTLLRN_RigControlValue::Make(FEulerTransform::Identity), FTransform::Identity, FTransform::Identity, false);
			};
			
			if (InitializationOptions.BoneNames.Num())
			{
				for (const FName& BoneName : InitializationOptions.BoneNames)
				{
					FTLLRN_RigElementKey BoneElementKey(BoneName, ETLLRN_RigElementType::Bone);
					if (const FTLLRN_RigBoneElement* BoneElement = GetHierarchy()->Find<FTLLRN_RigBoneElement>(BoneElementKey))
					{
						CreateControlForBoneElement(BoneElement);
					}
				}
			}
			else
			{
				GetHierarchy()->ForEach<FTLLRN_RigBoneElement>([&](const FTLLRN_RigBoneElement* BoneElement) -> bool
				{
					CreateControlForBoneElement(BoneElement);
					return true;
				});
			}

			SetControlOffsetsFromBoneInitials();
		}
		
		if (InitializationOptions.bGenerateCurveControls)
		{
			GetHierarchy()->ForEach<FTLLRN_RigCurveElement>([&](const FTLLRN_RigCurveElement* CurveElement) -> bool
			{
				const FName ControlName = GetControlName(CurveElement->GetFName(), CurveElement->GetType());

				FTLLRN_RigControlSettings Settings;
				Settings.ControlType = ETLLRN_RigControlType::Float;
				Settings.DisplayName = CurveElement->GetFName();

				const FName DisplayCurveControlName(*(CurveElement->GetFName().ToString() + TEXT(" Curve")));
				Settings.DisplayName = DisplayCurveControlName;
				Controller->AddControl(ControlName, FTLLRN_RigElementKey(), Settings, FTLLRN_RigControlValue::Make(CurveElement->Get()), FTransform::Identity, FTransform::Identity, false);
				
				return true;
			});
		}

		RefreshActiveControls();
	}
}

void UFKTLLRN_ControlRig::SetControlOffsetsFromBoneInitials()
{
	GetHierarchy()->Traverse([&](FTLLRN_RigBaseElement* InElement, bool& bContinue)
	{
		if(!InElement->IsA<FTLLRN_RigBoneElement>())
		{
			bContinue = false;
			return;
		}

		const FTLLRN_RigBoneElement* BoneElement = CastChecked<FTLLRN_RigBoneElement>(InElement);
		const FName BoneName = BoneElement->GetFName();
		const FName ControlName = GetControlName(BoneName, BoneElement->GetType());

		FTLLRN_RigControlElement* ControlElement = GetHierarchy()->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control));
		if(ControlElement == nullptr)
		{
			return;
		}
			
		const FTLLRN_RigElementKey ParentKey = GetHierarchy()->GetFirstParent(BoneElement->GetKey());

		FTransform OffsetTransform;
		if (ParentKey.IsValid())
		{
			FTransform GlobalTransform = GetHierarchy()->GetGlobalTransformByIndex(BoneElement->GetIndex(), true);
			FTransform ParentTransform = GetHierarchy()->GetGlobalTransform(ParentKey, true);
			OffsetTransform = GlobalTransform.GetRelativeTransform(ParentTransform);
		}
		else
		{
			OffsetTransform = GetHierarchy()->GetLocalTransformByIndex(BoneElement->GetIndex(), true);
		}

		OffsetTransform.NormalizeRotation();

		GetHierarchy()->SetControlOffsetTransform(ControlElement, OffsetTransform, ETLLRN_RigTransformType::InitialLocal, false, false, true);

	}, true);
}

void UFKTLLRN_ControlRig::RefreshActiveControls()
{
	if (const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		const int32 NumControls = Hierarchy->Num();
		if (IsControlActive.Num() != NumControls)
		{
			IsControlActive.Empty(NumControls);
			IsControlActive.SetNum(NumControls);
			for (bool& bIsActive : IsControlActive)
			{
				bIsActive = true;
			}
		}
	}
}

void UFKTLLRN_ControlRig::CreateTLLRN_RigElements(const USkeletalMesh* InReferenceMesh)
{
	if (InReferenceMesh)
	{
		const USkeleton* Skeleton = InReferenceMesh->GetSkeleton();
		CreateTLLRN_RigElements(InReferenceMesh->GetRefSkeleton(), Skeleton);
	}
}

void UFKTLLRN_ControlRig::SetApplyMode(ETLLRN_ControlRigFKRigExecuteMode InMode)
{
	if (ApplyMode == InMode)
	{
		return;
	}
	
	ApplyMode = InMode;

	if (UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		if (ApplyMode == ETLLRN_ControlRigFKRigExecuteMode::Additive)
		{
			FTLLRN_RigControlModifiedContext Context;
			Context.SetKey = ETLLRN_ControlRigSetKey::Never;
			const bool bSetupUndo = false;

			Hierarchy->ForEach<FTLLRN_RigControlElement>([&](FTLLRN_RigControlElement* ControlElement) -> bool
			{
				if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
				{
					SetControlValue<FEulerTransform>(ControlElement->GetFName(), FEulerTransform::Identity, true, Context, bSetupUndo);
				}
				else if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Float)
				{
					SetControlValue<float>(ControlElement->GetFName(), 0.f, true, Context, bSetupUndo);
				}

				return true;
			});
		}
		else
		{
			FTLLRN_RigControlModifiedContext Context;
			Context.SetKey = ETLLRN_ControlRigSetKey::Never;
			const bool bSetupUndo = false;

			Hierarchy->ForEach<FTLLRN_RigControlElement>([&](FTLLRN_RigControlElement* ControlElement) -> bool
			{
				if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
				{
					const FTLLRN_RigControlValue::FEulerTransform_Float InitValue =
						GetHierarchy()->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Initial).Get<FTLLRN_RigControlValue::FEulerTransform_Float>();
					SetControlValue<FTLLRN_RigControlValue::FEulerTransform_Float>(ControlElement->GetFName(), InitValue, true, Context, bSetupUndo);
				}
				else if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Float)
				{
					const float InitValue = GetHierarchy()->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Initial).Get<float>();
					SetControlValue<float>(ControlElement->GetFName(), InitValue, true, Context, bSetupUndo);
				}

				return true;
			});
		}
	}
}

void UFKTLLRN_ControlRig::ToggleApplyMode()
{
	ETLLRN_ControlRigFKRigExecuteMode ModeToApply = ApplyMode;
	if (ApplyMode == ETLLRN_ControlRigFKRigExecuteMode::Additive)
	{
		ModeToApply = CachedToggleApplyMode;
	}
	else
	{
		CachedToggleApplyMode = ApplyMode;
		ModeToApply = ETLLRN_ControlRigFKRigExecuteMode::Additive;
	}
	SetApplyMode(ModeToApply);
}

#undef LOCTEXT_NAMESPACE



