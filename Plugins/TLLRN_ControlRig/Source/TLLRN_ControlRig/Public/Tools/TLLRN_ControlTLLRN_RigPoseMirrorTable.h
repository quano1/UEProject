// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

/**
* Class to hold information on how a Pose May be Mirrored
*
*/

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

class UTLLRN_ControlRig;
struct FTLLRN_RigControl;
struct FTLLRN_RigControlCopy;
struct FTLLRN_ControlTLLRN_RigControlPose;

struct FTLLRN_ControlTLLRN_RigPoseMirrorTable
{
public:
	FTLLRN_ControlTLLRN_RigPoseMirrorTable() {};
	~FTLLRN_ControlTLLRN_RigPoseMirrorTable() {};

	/*Set up the Mirror Table*/
	void SetUpMirrorTable(const UTLLRN_ControlRig* TLLRN_ControlRig);

	/*Get the matched control with the given name*/
	FTLLRN_RigControlCopy* GetControl(FTLLRN_ControlTLLRN_RigControlPose& Pose, FName ControlrigName);

	/*Whether or not the Control with this name is matched*/
	bool IsMatched(const FName& ControlName) const;

	/*Return the Mirrored Global(In Control Rig Space) Translation and Mirrored Local Rotation*/
	void GetMirrorTransform(const FTLLRN_RigControlCopy& ControlCopy, bool bDoLocal, bool bIsMatched,FTransform& OutGlobalTransform, FTransform& OutLocalTransform) const;

private:

	TMap<FName, FName>  MatchedControls;

};