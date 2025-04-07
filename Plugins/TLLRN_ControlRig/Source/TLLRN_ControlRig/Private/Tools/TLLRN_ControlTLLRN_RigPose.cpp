// Copyright Epic Games, Inc. All Rights Reserved.
#include "Tools/TLLRN_ControlTLLRN_RigPose.h"
#include "Tools/TLLRN_ControlTLLRN_RigPoseProjectSettings.h"
#include "ITLLRN_ControlRigObjectBinding.h"
#include "TLLRN_ControlRig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlTLLRN_RigPose)

#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

#define LOCTEXT_NAMESPACE "TLLRN_ControlTLLRN_RigPose"

void FTLLRN_ControlTLLRN_RigControlPose::SavePose(UTLLRN_ControlRig* TLLRN_ControlRig, bool bUseAll)
{
	TArray<FTLLRN_RigControlElement*> CurrentControls;
	TLLRN_ControlRig->GetControlsInOrder(CurrentControls);
	CopyOfControls.SetNum(0);
	
	for (FTLLRN_RigControlElement* ControlElement : CurrentControls)
	{
		if (TLLRN_ControlRig->GetHierarchy()->IsAnimatable(ControlElement) && (bUseAll || TLLRN_ControlRig->IsControlSelected(ControlElement->GetFName())))
		{
			//we store poses in default parent space so if not in that space we need to compensate it
			bool bHasNonDefaultParent = false;
			FTransform GlobalTransform;
			FTLLRN_RigElementKey SpaceKey = TLLRN_ControlRig->GetHierarchy()->GetActiveParent(ControlElement->GetKey());
			if (SpaceKey != TLLRN_ControlRig->GetHierarchy()->GetDefaultParentKey())
			{
				bHasNonDefaultParent = true;
				//to compensate we get the global, switch space, then reset global in that new space
				GlobalTransform = TLLRN_ControlRig->GetHierarchy()->GetGlobalTransform(ControlElement->GetKey());
				TLLRN_ControlRig->GetHierarchy()->SwitchToDefaultParent(ControlElement->GetKey());
				TLLRN_ControlRig->GetHierarchy()->SetGlobalTransform(ControlElement->GetKey(),GlobalTransform);

				TLLRN_ControlRig->Evaluate_AnyThread();
			}
			FTLLRN_RigControlCopy Copy(ControlElement, TLLRN_ControlRig->GetHierarchy());
			CopyOfControls.Add(Copy);
	
			if (bHasNonDefaultParent == true) 
			{
				TLLRN_ControlRig->GetHierarchy()->SwitchToParent(ControlElement->GetKey(),SpaceKey);
				TLLRN_ControlRig->GetHierarchy()->SetGlobalTransform(ControlElement->GetKey(), GlobalTransform);
			}
			
		}
	}
	SetUpControlMap();
}

void FTLLRN_ControlTLLRN_RigControlPose::PastePose(UTLLRN_ControlRig* TLLRN_ControlRig, bool bDoKey, bool bDoMirror)
{
	PastePoseInternal(TLLRN_ControlRig, bDoKey, bDoMirror, CopyOfControls);
	TLLRN_ControlRig->Evaluate_AnyThread();
	PastePoseInternal(TLLRN_ControlRig, bDoKey, bDoMirror, CopyOfControls);

}

void FTLLRN_ControlTLLRN_RigControlPose::SetControlMirrorTransform(bool bDoLocal, UTLLRN_ControlRig* TLLRN_ControlRig, const FName& Name, bool bIsMatched, 
	const FTransform& GlobalTransform, const FTransform& LocalTransform, bool bNotify, const  FTLLRN_RigControlModifiedContext&Context,bool bSetupUndo)
{
	if (bDoLocal || bIsMatched)
	{
		TLLRN_ControlRig->SetControlLocalTransform(Name, LocalTransform, bNotify,Context,bSetupUndo, true/* bFixEulerFlips*/);

	}
	else
	{
		TLLRN_ControlRig->SetControlGlobalTransform(Name, GlobalTransform, bNotify,Context,bSetupUndo, false /*bPrintPython*/, true/* bFixEulerFlips*/);
	}	
}

void FTLLRN_ControlTLLRN_RigControlPose::PastePoseInternal(UTLLRN_ControlRig* TLLRN_ControlRig, bool bDoKey, bool bDoMirror, const TArray<FTLLRN_RigControlCopy>& ControlsToPaste)
{
	FTLLRN_RigControlModifiedContext Context;
	Context.SetKey = bDoKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
	FTLLRN_ControlTLLRN_RigPoseMirrorTable MirrorTable;
	if (bDoMirror)
	{
		MirrorTable.SetUpMirrorTable(TLLRN_ControlRig);
	}

	TArray<FTLLRN_RigControlElement*> SortedControls;
	TLLRN_ControlRig->GetControlsInOrder(SortedControls);
	
	const bool bDoLocal = true;
	const bool bSetupUndo = false;

	for (FTLLRN_RigControlElement* ControlElement : SortedControls)
	{
		if (!TLLRN_ControlRig->IsControlSelected(ControlElement->GetFName()))
		{
			continue;
		}
		
		FTLLRN_RigControlCopy* CopyTLLRN_RigControl = MirrorTable.GetControl(*this, ControlElement->GetFName());
		if (CopyTLLRN_RigControl)
		{
			//if not in default parent space we need to move it to default parent space first and then reset the global transforms
			bool bHasNonDefaultParent = false;
			FTLLRN_RigElementKey SpaceKey = TLLRN_ControlRig->GetHierarchy()->GetActiveParent(ControlElement->GetKey());
			if (SpaceKey != TLLRN_ControlRig->GetHierarchy()->GetDefaultParentKey())
			{
				bHasNonDefaultParent = true;
				TLLRN_ControlRig->GetHierarchy()->SwitchToDefaultParent(ControlElement->GetKey());
				TLLRN_ControlRig->Evaluate_AnyThread();
			}

			switch (ControlElement->Settings.ControlType)
			{
			case ETLLRN_RigControlType::Transform:
			case ETLLRN_RigControlType::TransformNoScale:
			case ETLLRN_RigControlType::EulerTransform:
			case ETLLRN_RigControlType::Position:
			case ETLLRN_RigControlType::Scale:
			case ETLLRN_RigControlType::Rotator:
			{
				if (bDoMirror == false)
				{
					if (bDoLocal) // -V547  
					{
						TLLRN_ControlRig->SetControlLocalTransform(ControlElement->GetFName(), CopyTLLRN_RigControl->LocalTransform, true,Context,bSetupUndo, true/* bFixEulerFlips*/);
					}
					else
					{
						TLLRN_ControlRig->SetControlGlobalTransform(ControlElement->GetFName(), CopyTLLRN_RigControl->GlobalTransform, true, Context, bSetupUndo, false /*bPrintPython*/, true/* bFixEulerFlips*/);
					}
				}
				else
				{
					FTransform GlobalTransform;
					FTransform LocalTransform;
					bool bIsMatched = MirrorTable.IsMatched(CopyTLLRN_RigControl->Name);
					MirrorTable.GetMirrorTransform(*CopyTLLRN_RigControl, bDoLocal,bIsMatched,GlobalTransform,LocalTransform);
					SetControlMirrorTransform(bDoLocal,TLLRN_ControlRig, ControlElement->GetFName(), bIsMatched, GlobalTransform,LocalTransform,true,Context,bSetupUndo);
				}				
				break;
			}
			case ETLLRN_RigControlType::Float:
			case ETLLRN_RigControlType::ScaleFloat:
			{
				float Val = CopyTLLRN_RigControl->Value.Get<float>();
				TLLRN_ControlRig->SetControlValue<float>(ControlElement->GetFName(), Val, true, Context,bSetupUndo);
				break;
			}
			case ETLLRN_RigControlType::Bool:
			{
				bool Val = CopyTLLRN_RigControl->Value.Get<bool>();
				TLLRN_ControlRig->SetControlValue<bool>(ControlElement->GetFName(), Val, true, Context,bSetupUndo);
				break;
			}
			case ETLLRN_RigControlType::Integer:
			{
				int32 Val = CopyTLLRN_RigControl->Value.Get<int32>();
				TLLRN_ControlRig->SetControlValue<int32>(ControlElement->GetFName(), Val, true, Context,bSetupUndo);
				break;
			}
			case ETLLRN_RigControlType::Vector2D:
			{
				FVector3f Val = CopyTLLRN_RigControl->Value.Get<FVector3f>();
				TLLRN_ControlRig->SetControlValue<FVector3f>(ControlElement->GetFName(), Val, true, Context,bSetupUndo);
				break;
			}
			default:
				//TODO add log
				break;
			};

			if (bHasNonDefaultParent == true)
			{
				FTransform GlobalTransform = TLLRN_ControlRig->GetHierarchy()->GetGlobalTransform(ControlElement->GetKey());
				TLLRN_ControlRig->GetHierarchy()->SwitchToParent(ControlElement->GetKey(), SpaceKey);
				TLLRN_ControlRig->SetControlGlobalTransform(ControlElement->GetFName(), GlobalTransform, true, Context, bSetupUndo, false /*bPrintPython*/, true/* bFixEulerFlips*/);
			}
		}
	}
}

void FTLLRN_ControlTLLRN_RigControlPose::BlendWithInitialPoses(FTLLRN_ControlTLLRN_RigControlPose& InitialPose, UTLLRN_ControlRig* TLLRN_ControlRig, bool bDoKey, bool bDoMirror, float BlendValue)
{
	if (InitialPose.CopyOfControls.Num() == 0)
	{
		return;
	}

	//though can be n^2 should be okay, we search from current Index which in most cases will be the same
	//not run often anyway
	FTLLRN_RigControlModifiedContext Context;
	Context.SetKey = bDoKey ? ETLLRN_ControlRigSetKey::Always : ETLLRN_ControlRigSetKey::DoNotCare;
	FTLLRN_ControlTLLRN_RigPoseMirrorTable MirrorTable;
	if (bDoMirror)
	{
		MirrorTable.SetUpMirrorTable(TLLRN_ControlRig);
	}

	TArray<FTLLRN_RigControlElement*> SortedControls;
	TLLRN_ControlRig->GetControlsInOrder(SortedControls);
	
	const bool bDoLocal = true;
	const bool bSetupUndo = false;
	for (FTLLRN_RigControlElement* ControlElement : SortedControls)
	{
		if (!TLLRN_ControlRig->IsControlSelected(ControlElement->GetFName()))
		{
			continue;
		}
		FTLLRN_RigControlCopy* CopyTLLRN_RigControl = MirrorTable.GetControl(*this, ControlElement->GetFName());
		if (CopyTLLRN_RigControl)
		{
			FTLLRN_RigControlCopy* InitialFound = nullptr;
			int32* Index = InitialPose.CopyOfControlsNameToIndex.Find(CopyTLLRN_RigControl->Name);
			if (Index)
			{
				InitialFound = &(InitialPose.CopyOfControls[*Index]);
			}
			if (InitialFound && InitialFound->ControlType == CopyTLLRN_RigControl->ControlType)
			{
				if ((CopyTLLRN_RigControl->ControlType == ETLLRN_RigControlType::Transform || CopyTLLRN_RigControl->ControlType == ETLLRN_RigControlType::EulerTransform ||
					CopyTLLRN_RigControl->ControlType == ETLLRN_RigControlType::TransformNoScale || CopyTLLRN_RigControl->ControlType == ETLLRN_RigControlType::Position ||
					CopyTLLRN_RigControl->ControlType == ETLLRN_RigControlType::Rotator || CopyTLLRN_RigControl->ControlType == ETLLRN_RigControlType::Scale
					))
				{
					//if not in default parent space we need to move it to default parent space first and then reset the global transforms
					bool bHasNonDefaultParent = false;
					FTLLRN_RigElementKey SpaceKey = TLLRN_ControlRig->GetHierarchy()->GetActiveParent(ControlElement->GetKey());
					if (SpaceKey != TLLRN_ControlRig->GetHierarchy()->GetDefaultParentKey())
					{
						bHasNonDefaultParent = true;
						TLLRN_ControlRig->GetHierarchy()->SwitchToDefaultParent(ControlElement->GetKey());
						TLLRN_ControlRig->Evaluate_AnyThread();
					}
					if (bDoMirror == false)
					{
						if (bDoLocal == true)    // -V547  
						{
							FTransform Val = CopyTLLRN_RigControl->LocalTransform;
							FTransform InitialVal = InitialFound->LocalTransform;
							FVector Translation, Scale;
							FQuat Rotation;
							Translation = FMath::Lerp(InitialVal.GetTranslation(), Val.GetTranslation(), BlendValue);
							Rotation = FQuat::Slerp(InitialVal.GetRotation(), Val.GetRotation(), BlendValue); //doing slerp here not fast lerp, can be slow this is for content creation
							Scale = FMath::Lerp(InitialVal.GetScale3D(), Val.GetScale3D(), BlendValue);
							Val = FTransform(Rotation, Translation, Scale);
							TLLRN_ControlRig->SetControlLocalTransform(ControlElement->GetFName(), Val, bDoKey,Context,bSetupUndo, true/* bFixEulerFlips*/);
						}
						else
						{
							FTransform Val = CopyTLLRN_RigControl->GlobalTransform;
							FTransform InitialVal = InitialFound->GlobalTransform;
							FVector Translation, Scale;
							FQuat Rotation;
							Translation = FMath::Lerp(InitialVal.GetTranslation(), Val.GetTranslation(), BlendValue);
							Rotation = FQuat::Slerp(InitialVal.GetRotation(), Val.GetRotation(), BlendValue); //doing slerp here not fast lerp, can be slow this is for content creation
							Scale = FMath::Lerp(InitialVal.GetScale3D(), Val.GetScale3D(), BlendValue);
							Val = FTransform(Rotation, Translation, Scale);
							TLLRN_ControlRig->SetControlGlobalTransform(ControlElement->GetFName(), Val, bDoKey,Context,bSetupUndo, false /*bPrintPython*/, true/* bFixEulerFlips*/);
						}
					}
					else
					{
						FTransform GlobalTransform;
						FTransform LocalTransform;
						bool bIsMatched = MirrorTable.IsMatched(CopyTLLRN_RigControl->Name);
						MirrorTable.GetMirrorTransform(*CopyTLLRN_RigControl, bDoLocal, bIsMatched, GlobalTransform, LocalTransform);
						FVector InitialTranslation = InitialFound->GlobalTransform.GetTranslation();
						FQuat InitialGlobalRotation = InitialFound->GlobalTransform.GetRotation();
						FVector InitialGlobalScale = InitialFound->GlobalTransform.GetScale3D();

						FVector InitialLocalTranslation = InitialFound->LocalTransform.GetTranslation();
						FQuat InitialLocationRotation = InitialFound->LocalTransform.GetRotation();
						FVector InitialLocalScale = InitialFound->LocalTransform.GetScale3D();

						GlobalTransform.SetTranslation(FMath::Lerp(InitialTranslation, GlobalTransform.GetTranslation(), BlendValue));
						GlobalTransform.SetRotation(FQuat::Slerp(InitialGlobalRotation, GlobalTransform.GetRotation(), BlendValue)); //doing slerp here not fast lerp, can be slow this is for content creation
						GlobalTransform.SetScale3D(FMath::Lerp(InitialGlobalScale, GlobalTransform.GetScale3D(), BlendValue));

						LocalTransform.SetTranslation(FMath::Lerp(InitialLocalTranslation, LocalTransform.GetTranslation(), BlendValue));
						LocalTransform.SetRotation(FQuat::Slerp(InitialLocationRotation, LocalTransform.GetRotation(), BlendValue)); //doing slerp here not fast lerp, can be slow this is for content creation
						LocalTransform.SetScale3D(FMath::Lerp(InitialLocalScale, LocalTransform.GetScale3D(), BlendValue));

						SetControlMirrorTransform(bDoLocal,TLLRN_ControlRig, ControlElement->GetFName(), bIsMatched, GlobalTransform,LocalTransform,bDoKey,Context,bSetupUndo);							
					}
					if (bHasNonDefaultParent == true)
					{
						FTransform GlobalTransform = TLLRN_ControlRig->GetHierarchy()->GetGlobalTransform(ControlElement->GetKey());
						TLLRN_ControlRig->GetHierarchy()->SwitchToParent(ControlElement->GetKey(), SpaceKey);
						TLLRN_ControlRig->SetControlGlobalTransform(ControlElement->GetFName(), GlobalTransform, true, Context, bSetupUndo, false /*bPrintPython*/, true/* bFixEulerFlips*/);
					}
				}
				else if(CopyTLLRN_RigControl->ControlType == ETLLRN_RigControlType::Float || 
						CopyTLLRN_RigControl->ControlType == ETLLRN_RigControlType::ScaleFloat)
				{
					float InitialVal = InitialFound->Value.Get<float>();
					float Val = CopyTLLRN_RigControl->Value.Get<float>();
					Val = FMath::Lerp(InitialVal, Val, BlendValue);
					TLLRN_ControlRig->SetControlValue<float>(ControlElement->GetFName(), Val, true, Context, bSetupUndo);
				}
				else if (CopyTLLRN_RigControl->ControlType == ETLLRN_RigControlType::Vector2D)
				{
					FVector3f InitialVal = InitialFound->Value.Get<FVector3f>();
					FVector3f Val = CopyTLLRN_RigControl->Value.Get<FVector3f>();
					Val = FMath::Lerp(InitialVal, Val, BlendValue);
					TLLRN_ControlRig->SetControlValue<FVector3f>(ControlElement->GetFName(), Val, true, Context, bSetupUndo);
				}
			}
		}
	}
}

bool FTLLRN_ControlTLLRN_RigControlPose::ContainsName(const FName& Name) const
{
	const int32* Index = CopyOfControlsNameToIndex.Find(Name);
	return (Index && *Index >= 0);
}

void FTLLRN_ControlTLLRN_RigControlPose::ReplaceControlName(const FName& Name, const FName& NewName)
{
	int32* Index = CopyOfControlsNameToIndex.Find(Name);
	if (Index && *Index >= 0)
	{
		FTLLRN_RigControlCopy& Control = CopyOfControls[*Index];
		Control.Name = NewName;
		CopyOfControlsNameToIndex.Remove(Name);
		CopyOfControlsNameToIndex.Add(Control.Name, *Index);
	}
}

TArray<FName> FTLLRN_ControlTLLRN_RigControlPose::GetControlNames() const
{
	TArray<FName> Controls;
	Controls.Reserve(CopyOfControls.Num());
	for (const FTLLRN_RigControlCopy& Control : CopyOfControls)
	{
		Controls.Add(Control.Name);
	}
	return Controls;
}

void FTLLRN_ControlTLLRN_RigControlPose::SetUpControlMap()
{
	CopyOfControlsNameToIndex.Reset();

	for (int32 Index = 0; Index < CopyOfControls.Num(); ++Index)
	{
		const FTLLRN_RigControlCopy& Control = CopyOfControls[Index];
		CopyOfControlsNameToIndex.Add(Control.Name, Index);
	}
}


UTLLRN_ControlTLLRN_RigPoseAsset::UTLLRN_ControlTLLRN_RigPoseAsset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UTLLRN_ControlTLLRN_RigPoseAsset::PostLoad()
{
	Super::PostLoad();
	Pose.SetUpControlMap();
}

void UTLLRN_ControlTLLRN_RigPoseAsset::SavePose(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bUseAll)
{
	Pose.SavePose(InTLLRN_ControlRig,bUseAll);
}

void UTLLRN_ControlTLLRN_RigPoseAsset::PastePose(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bDoKey, bool bDoMirror)
{
#if WITH_EDITOR
	FScopedTransaction ScopedTransaction(LOCTEXT("PastePoseTransaction", "Paste Pose"));
	InTLLRN_ControlRig->Modify();
#endif
	Pose.PastePose(InTLLRN_ControlRig,bDoKey, bDoMirror);
}

void UTLLRN_ControlTLLRN_RigPoseAsset::SelectControls(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bDoMirror)
{
#if WITH_EDITOR
	FScopedTransaction ScopedTransaction(LOCTEXT("SelectControlTransaction", "Select Control"));
	InTLLRN_ControlRig->Modify();
#endif
	InTLLRN_ControlRig->ClearControlSelection();
	TArray<FName> Controls = Pose.GetControlNames();
	FTLLRN_ControlTLLRN_RigPoseMirrorTable MirrorTable;
	FTLLRN_ControlTLLRN_RigControlPose TempPose;
	if (bDoMirror)
	{
		MirrorTable.SetUpMirrorTable(InTLLRN_ControlRig);
		TempPose.SavePose(InTLLRN_ControlRig, true);
	}
	for (const FName& Name : Controls)
	{
		if (bDoMirror)
		{
			FTLLRN_RigControlCopy* CopyTLLRN_RigControl = MirrorTable.GetControl(TempPose, Name);
			if (CopyTLLRN_RigControl)
			{
				InTLLRN_ControlRig->SelectControl(CopyTLLRN_RigControl->Name, true);
			}
			else
			{
				InTLLRN_ControlRig->SelectControl(Name, true);
			}
		}
		else
		{
			InTLLRN_ControlRig->SelectControl(Name, true);
		}
	}
}

void UTLLRN_ControlTLLRN_RigPoseAsset::GetCurrentPose(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_ControlTLLRN_RigControlPose& OutPose)
{
	OutPose.SavePose(InTLLRN_ControlRig, true);
}


TArray<FTLLRN_RigControlCopy> UTLLRN_ControlTLLRN_RigPoseAsset::GetCurrentPose(UTLLRN_ControlRig* InTLLRN_ControlRig) 
{
	FTLLRN_ControlTLLRN_RigControlPose TempPose;
	TempPose.SavePose(InTLLRN_ControlRig,true);
	return TempPose.GetPoses();
}

void UTLLRN_ControlTLLRN_RigPoseAsset::BlendWithInitialPoses(FTLLRN_ControlTLLRN_RigControlPose& InitialPose, UTLLRN_ControlRig* InTLLRN_ControlRig, bool bDoKey, bool bDoMirror, float BlendValue)
{
	if (BlendValue > 0.0f)
	{
		Pose.BlendWithInitialPoses(InitialPose, InTLLRN_ControlRig, bDoKey, bDoMirror, BlendValue);
	}
}

TArray<FName> UTLLRN_ControlTLLRN_RigPoseAsset::GetControlNames() const
{
	return Pose.GetControlNames();
}

void UTLLRN_ControlTLLRN_RigPoseAsset::ReplaceControlName(const FName& CurrentName, const FName& NewName)
{
	Pose.ReplaceControlName(CurrentName, NewName);
}

bool UTLLRN_ControlTLLRN_RigPoseAsset::DoesMirrorMatch(UTLLRN_ControlRig* TLLRN_ControlRig, const FName& ControlName) const
{
	FTLLRN_ControlTLLRN_RigPoseMirrorTable MirrorTable;
	MirrorTable.SetUpMirrorTable(TLLRN_ControlRig);
	return (MirrorTable.IsMatched(ControlName));
}


#undef LOCTEXT_NAMESPACE


