// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tools/TLLRN_ControlTLLRN_RigPoseMirrorTable.h"
#include "Tools/TLLRN_ControlTLLRN_RigPoseMirrorSettings.h"
#include "TLLRN_ControlRig.h"
#include "Tools/TLLRN_ControlTLLRN_RigPose.h"
#include "RigVMFunctions/Math/RigVMMathLibrary.h"

void FTLLRN_ControlTLLRN_RigPoseMirrorTable::SetUpMirrorTable(const UTLLRN_ControlRig* TLLRN_ControlRig)
{
	const UTLLRN_ControlTLLRN_RigPoseMirrorSettings* Settings = GetDefault<UTLLRN_ControlTLLRN_RigPoseMirrorSettings>();
	MatchedControls.Reset();
	if (Settings && TLLRN_ControlRig)
	{
		TArray<FTLLRN_RigControlElement*> CurrentControls = TLLRN_ControlRig->AvailableControls();
		for (FTLLRN_RigControlElement* ControlElement : CurrentControls)
		{
			FString CurrentString = ControlElement->GetName();
			if (CurrentString.Contains(Settings->RightSide))
			{
				FString NewString = CurrentString.Replace(*Settings->RightSide, *Settings->LeftSide);
				FName CurrentName(*CurrentString);
				FName NewName(*NewString);
				MatchedControls.Add(NewName, CurrentName);
			}
			else if (CurrentString.Contains(Settings->LeftSide))
			{
				FString NewString = CurrentString.Replace(*Settings->LeftSide, *Settings->RightSide);
				FName CurrentName(*CurrentString);
				FName NewName(*NewString);
				MatchedControls.Add(NewName, CurrentName);
			}
		}
	}
}

FTLLRN_RigControlCopy* FTLLRN_ControlTLLRN_RigPoseMirrorTable::GetControl(FTLLRN_ControlTLLRN_RigControlPose& Pose, FName Name)
{
	TArray<FTLLRN_RigControlCopy> CopyOfControls;
	
	if (MatchedControls.Num() > 0) 
	{
		if (const FName* MatchedName = MatchedControls.Find(Name))
		{
			int32* Index = Pose.CopyOfControlsNameToIndex.Find(*MatchedName);
			if (Index != nullptr && (*Index) >= 0 && (*Index) < Pose.CopyOfControls.Num())
			{
				return &(Pose.CopyOfControls[*Index]);
			}
		}
	}
	//okay not matched so just find it.
	int32* Index = Pose.CopyOfControlsNameToIndex.Find(Name);
	if (Index != nullptr && (*Index) >= 0 && (*Index) < Pose.CopyOfControls.Num())
	{
		return &(Pose.CopyOfControls[*Index]);
	}

	return nullptr;
}

bool FTLLRN_ControlTLLRN_RigPoseMirrorTable::IsMatched(const FName& Name) const
{
	if (MatchedControls.Num() > 0)
	{
		if (const FName* MatchedName = MatchedControls.Find(Name))
		{
			return true;
		}
	}
	return false;
}

//Now returns mirrored global and local unmirrored
void FTLLRN_ControlTLLRN_RigPoseMirrorTable::GetMirrorTransform(const FTLLRN_RigControlCopy& ControlCopy, bool bDoLocal, bool bIsMatched, FTransform& OutGlobalTransform,
	FTransform& OutLocalTransform) const
{
	const UTLLRN_ControlTLLRN_RigPoseMirrorSettings* Settings = GetDefault<UTLLRN_ControlTLLRN_RigPoseMirrorSettings>();
	FTransform GlobalTransform = ControlCopy.GlobalTransform;
	OutGlobalTransform = GlobalTransform;
	FTransform LocalTransform = ControlCopy.LocalTransform;
	OutLocalTransform = LocalTransform;
	if (Settings)
	{
		if (!bIsMatched  && (OutGlobalTransform.GetRotation().IsIdentity() == false || OutLocalTransform.GetRotation().IsIdentity() == false))
		{
			FRigVMMirrorSettings MirrorSettings;
			MirrorSettings.MirrorAxis = Settings->MirrorAxis;
			MirrorSettings.AxisToFlip = Settings->AxisToFlip;
			FTransform NewTransform = MirrorSettings.MirrorTransform(GlobalTransform);
			OutGlobalTransform.SetTranslation(NewTransform.GetTranslation());
			OutGlobalTransform.SetRotation(NewTransform.GetRotation());

			FTransform NewLocalTransform = MirrorSettings.MirrorTransform(LocalTransform);
			OutLocalTransform.SetTranslation(NewLocalTransform.GetTranslation());
			OutLocalTransform.SetRotation(NewLocalTransform.GetRotation());
			return;
		}
	}
}
