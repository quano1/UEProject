// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigTestData.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigObjectVersion.h"
#include "HAL/PlatformTime.h"
#if WITH_EDITOR
#include "AssetToolsModule.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigTestData)

bool FTLLRN_ControlRigTestDataFrame::Store(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bInitial)
{
	if(InTLLRN_ControlRig == nullptr)
	{
		return false;
	}

	AbsoluteTime = InTLLRN_ControlRig->GetAbsoluteTime();
	DeltaTime = InTLLRN_ControlRig->GetDeltaTime();
	Pose = InTLLRN_ControlRig->GetHierarchy()->GetPose(bInitial);
	Variables.Reset();

	const TArray<FRigVMExternalVariable> ExternalVariables = InTLLRN_ControlRig->GetExternalVariables(); 
	for(const FRigVMExternalVariable& ExternalVariable : ExternalVariables)
	{
		FTLLRN_ControlRigTestDataVariable VariableData;
		VariableData.Name = ExternalVariable.Name;
		VariableData.CPPType = ExternalVariable.TypeName;

		if(ExternalVariable.Property && ExternalVariable.Memory)
		{
			ExternalVariable.Property->ExportText_Direct(
				VariableData.Value,
				ExternalVariable.Memory,
				ExternalVariable.Memory,
				nullptr,
				PPF_None,
				nullptr
			);
		}

		Variables.Add(VariableData);
	}

	return true;
}

bool FTLLRN_ControlRigTestDataFrame::Restore(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bInitial) const
{
	if(InTLLRN_ControlRig == nullptr)
	{
		return false;
	}

	UTLLRN_RigHierarchy* Hierarchy = InTLLRN_ControlRig->GetHierarchy();

	// check if the pose can be applied
	for(const FTLLRN_RigPoseElement& PoseElement : Pose.Elements)
	{
		const FTLLRN_RigElementKey& Key = PoseElement.Index.GetKey(); 
		if(!Hierarchy->Contains(Key))
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("TLLRN Control Rig does not contain hierarchy element '%s'. Please re-create the test data asset."), *Key.ToString());
			return false;
		}
	}

	Hierarchy->SetPose(Pose, bInitial ? ETLLRN_RigTransformType::InitialLocal : ETLLRN_RigTransformType::CurrentLocal);

	return RestoreVariables(InTLLRN_ControlRig);
}

bool FTLLRN_ControlRigTestDataFrame::RestoreVariables(UTLLRN_ControlRig* InTLLRN_ControlRig) const
{
	class FTLLRN_ControlRigTestDataFrame_ErrorPipe : public FOutputDevice
	{
	public:

		TArray<FString> Errors;

		FTLLRN_ControlRigTestDataFrame_ErrorPipe()
			: FOutputDevice()
		{
		}

		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
		{
			Errors.Add(FString::Printf(TEXT("Error convert to string: %s"), V));
		}
	};

	const TArray<FRigVMExternalVariable> ExternalVariables = InTLLRN_ControlRig->GetExternalVariables();

	if(ExternalVariables.Num() != Variables.Num())
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Variable data does not match the Rig. Please re-create the test data asset."));
		return false;
	}
	
	for(const FRigVMExternalVariable& ExternalVariable : ExternalVariables)
	{
		if(ExternalVariable.Memory == nullptr || ExternalVariable.Property == nullptr)
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Variable '%s' is not valid."), *ExternalVariable.Name.ToString());
			return false;
		}
		
		const FTLLRN_ControlRigTestDataVariable* VariableData = Variables.FindByPredicate(
			[ExternalVariable](const FTLLRN_ControlRigTestDataVariable& InVariable) -> bool
			{
				return InVariable.Name == ExternalVariable.Name &&
					InVariable.CPPType == ExternalVariable.TypeName;
			}
		);

		if(VariableData)
		{
			FTLLRN_ControlRigTestDataFrame_ErrorPipe ErrorPipe;
			ExternalVariable.Property->ImportText_Direct(
				*VariableData->Value,
				ExternalVariable.Memory,
				nullptr,
				PPF_None,
				&ErrorPipe
			);

			if(!ErrorPipe.Errors.IsEmpty())
			{
				for(const FString& ImportError : ErrorPipe.Errors)
				{
					UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Import Error for Variable '%s': %s"), *ExternalVariable.Name.ToString(), *ImportError);
				}
				return false;
			}
		}
		else
		{
			UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Variable data for '%s' is not part of the test file. Please re-create the test data asset."), *ExternalVariable.Name.ToString());
			return false;
		}
	}

	return true;
}

void UTLLRN_ControlRigTestData::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FTLLRN_ControlRigObjectVersion::GUID);	
	UObject::Serialize(Ar);
	LastFrameIndex = INDEX_NONE;

	// If pose is older than TLLRN_RigPoseWithParentKey, set the active parent of all poses to invalid key
	if (GetLinkerCustomVersion(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::TLLRN_RigPoseWithParentKey)
	{
		for (FTLLRN_RigPoseElement& Element : Initial.Pose)
		{
			Element.ActiveParent = FTLLRN_RigElementKey();
		}
		for (FTLLRN_ControlRigTestDataFrame& Frame : InputFrames)
		{
			for (FTLLRN_RigPoseElement& Element : Frame.Pose)
			{
				Element.ActiveParent = FTLLRN_RigElementKey();
			}
		}
		for (FTLLRN_ControlRigTestDataFrame& Frame : OutputFrames)
		{
			for (FTLLRN_RigPoseElement& Element : Frame.Pose)
			{
				Element.ActiveParent = FTLLRN_RigElementKey();
			}
		}
	}
}

UTLLRN_ControlRigTestData* UTLLRN_ControlRigTestData::CreateNewAsset(FString InDesiredPackagePath, FString InBlueprintPathName)
{
#if WITH_EDITOR
	const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(InDesiredPackagePath, TEXT(""), UniquePackageName, UniqueAssetName);

	if (UniquePackageName.EndsWith(UniqueAssetName))
	{
		UniquePackageName = UniquePackageName.LeftChop(UniqueAssetName.Len() + 1);
	}

	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(*UniqueAssetName, *UniquePackageName, UTLLRN_ControlRigTestData::StaticClass(), nullptr);
	if(NewAsset)
	{
		// make sure the package is never cooked.
		UPackage* Package = NewAsset->GetOutermost();
		Package->SetPackageFlags(Package->GetPackageFlags() | PKG_EditorOnly);
			
		if(UTLLRN_ControlRigTestData* TestData = Cast<UTLLRN_ControlRigTestData>(NewAsset))
		{
			TestData->TLLRN_ControlRigObjectPath = InBlueprintPathName;
			return TestData;
		}
	}
#endif
	return nullptr;
}

FVector2D UTLLRN_ControlRigTestData::GetTimeRange(bool bInput) const
{
	const TArray<FTLLRN_ControlRigTestDataFrame>& Frames = bInput ? InputFrames : OutputFrames;
	if(Frames.IsEmpty())
	{
		return FVector2D::ZeroVector;
	}
	return FVector2D(Frames[0].AbsoluteTime, Frames.Last().AbsoluteTime);
}

int32 UTLLRN_ControlRigTestData::GetFrameIndexForTime(double InSeconds, bool bInput) const
{
	const TArray<FTLLRN_ControlRigTestDataFrame>& Frames = bInput ? InputFrames : OutputFrames;

	if(LastFrameIndex == INDEX_NONE)
	{
		LastFrameIndex = 0;
	}

	while(Frames.IsValidIndex(LastFrameIndex) && Frames[LastFrameIndex].AbsoluteTime < InSeconds)
	{
		LastFrameIndex++;
	}

	while(Frames.IsValidIndex(LastFrameIndex) && Frames[LastFrameIndex].AbsoluteTime > InSeconds)
	{
		LastFrameIndex--;
	}

	LastFrameIndex = FMath::Clamp(LastFrameIndex, 0, Frames.Num() - 1);

	return LastFrameIndex;
}

bool UTLLRN_ControlRigTestData::Record(UTLLRN_ControlRig* InTLLRN_ControlRig, double InRecordingDuration)
{
	if(InTLLRN_ControlRig == nullptr)
	{
		return false;
	}

	ReleaseReplay();
	ClearDelegates(InTLLRN_ControlRig);

	DesiredRecordingDuration = InRecordingDuration;
	TimeAtStartOfRecording = FPlatformTime::Seconds();

	// if this is the first frame
	if(InputFrames.IsEmpty())
	{
		InTLLRN_ControlRig->RequestInit();
		PreConstructionHandle = InTLLRN_ControlRig->OnPreConstruction_AnyThread().AddLambda(
			[this](UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
			{
				Initial.Store(InTLLRN_ControlRig, true);
			}
		);
	}

	PreForwardHandle = InTLLRN_ControlRig->OnPreForwardsSolve_AnyThread().AddLambda(
		[this](UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
		{
			FTLLRN_ControlRigTestDataFrame Frame;
			Frame.Store(InTLLRN_ControlRig);

			// reapply the variable data. we are doing this to make sure that
			// the results in the rig are the same during a recording and replay.
			Frame.RestoreVariables(InTLLRN_ControlRig);
			
			InputFrames.Add(Frame);
		}
	);

	PostForwardHandle = InTLLRN_ControlRig->OnPostForwardsSolve_AnyThread().AddLambda(
		[this](UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
		{
			FTLLRN_ControlRigTestDataFrame Frame;
			Frame.Store(InTLLRN_ControlRig);
			OutputFrames.Add(Frame);
			LastFrameIndex = INDEX_NONE;
			(void)MarkPackageDirty();

			const double TimeNow = FPlatformTime::Seconds();
			const double TimeDelta = TimeNow - TimeAtStartOfRecording;
			if(DesiredRecordingDuration <= TimeDelta)
			{
				DesiredRecordingDuration = 0.0;

				// Once clear delegates is called, we no longer have access to this pointer
				ClearDelegates(InTLLRN_ControlRig);
			}
		}
	);

	return true;
}

bool UTLLRN_ControlRigTestData::SetupReplay(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bGroundTruth)
{
	ReleaseReplay();
	ClearDelegates(InTLLRN_ControlRig);

	if(InTLLRN_ControlRig == nullptr)
	{
		return false;
	}
	
	bIsApplyingOutputs = bGroundTruth;

	if(InputFrames.IsEmpty() || OutputFrames.IsEmpty())
	{
		return false;
	}

	// reset the control rig's absolute time
	InTLLRN_ControlRig->SetAbsoluteAndDeltaTime(InputFrames[0].AbsoluteTime, InputFrames[0].DeltaTime);
	
	InTLLRN_ControlRig->RequestInit();
	PreConstructionHandle = InTLLRN_ControlRig->OnPreConstruction_AnyThread().AddLambda(
		[this](UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
		{
			Initial.Restore(InTLLRN_ControlRig, true);
		}
	);

	PreForwardHandle = InTLLRN_ControlRig->OnPreForwardsSolve_AnyThread().AddLambda(
		[this](UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
		{
			// loop the animation data
			if(InTLLRN_ControlRig->GetAbsoluteTime() < GetTimeRange().X - SMALL_NUMBER ||
				InTLLRN_ControlRig->GetAbsoluteTime() > GetTimeRange().Y + SMALL_NUMBER)
			{
				InTLLRN_ControlRig->SetAbsoluteAndDeltaTime(GetTimeRange().X, InTLLRN_ControlRig->GetDeltaTime());
			}
			
			const int32 FrameIndex = GetFrameIndexForTime(InTLLRN_ControlRig->GetAbsoluteTime(), true);
			const FTLLRN_ControlRigTestDataFrame& Frame = InputFrames[FrameIndex];
			Frame.Restore(InTLLRN_ControlRig, false);

			if(Frame.DeltaTime > SMALL_NUMBER)
			{
				InTLLRN_ControlRig->SetDeltaTime(Frame.DeltaTime);
			}
		}
	);

	PostForwardHandle = InTLLRN_ControlRig->OnPostForwardsSolve_AnyThread().AddLambda(
		[this](UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
		{
			const FTLLRN_RigPose CurrentPose = InTLLRN_ControlRig->GetHierarchy()->GetPose();

			const FTLLRN_ControlRigTestDataFrame& Frame = OutputFrames[LastFrameIndex];
			if(bIsApplyingOutputs)
			{
				Frame.Restore(InTLLRN_ControlRig, false);
			}

			const FTLLRN_RigPose& ExpectedPose = Frame.Pose;

			// draw differences of the pose result of the rig onto the screen
			FRigVMDrawInterface& DrawInterface = InTLLRN_ControlRig->GetDrawInterface();
			for(const FTLLRN_RigPoseElement& ExpectedPoseElement : ExpectedPose)
			{
				const int32 CurrentPoseIndex = CurrentPose.GetIndex(ExpectedPoseElement.Index.GetKey());
				if(CurrentPoseIndex != INDEX_NONE)
				{
					const FTLLRN_RigPoseElement& CurrentPoseElement = CurrentPose[CurrentPoseIndex];

					if(!CurrentPoseElement.LocalTransform.Equals(ExpectedPoseElement.LocalTransform, 0.001f))
					{
						DrawInterface.DrawAxes(
							FTransform::Identity,
							bIsApplyingOutputs ? CurrentPoseElement.GlobalTransform : ExpectedPoseElement.GlobalTransform,
							bIsApplyingOutputs ? FLinearColor::Red : FLinearColor::Green,
							15.0f,
							1.0f
						);
					}
				}
			}
		}
	);

	ReplayTLLRN_ControlRig = InTLLRN_ControlRig;
	return true;
}

void UTLLRN_ControlRigTestData::ReleaseReplay()
{
	if(UTLLRN_ControlRig* TLLRN_ControlRig = ReplayTLLRN_ControlRig.Get())
	{
		ClearDelegates(TLLRN_ControlRig);
		ReplayTLLRN_ControlRig.Reset();
	}
}

ETLLRN_ControlRigTestDataPlaybackMode UTLLRN_ControlRigTestData::GetPlaybackMode() const
{
	if(IsReplaying())
	{
		if(bIsApplyingOutputs)
		{
			return ETLLRN_ControlRigTestDataPlaybackMode::GroundTruth;
		}
		return ETLLRN_ControlRigTestDataPlaybackMode::ReplayInputs;
	}
	return ETLLRN_ControlRigTestDataPlaybackMode::Live;
}

bool UTLLRN_ControlRigTestData::IsReplaying() const
{
	return ReplayTLLRN_ControlRig.IsValid();
}

void UTLLRN_ControlRigTestData::ClearDelegates(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	if(InTLLRN_ControlRig)
	{
		if(PreConstructionHandle.IsValid())
		{
			InTLLRN_ControlRig->OnPreConstruction_AnyThread().Remove(PreConstructionHandle);
			PreConstructionHandle.Reset();
		}
		if(PreForwardHandle.IsValid())
		{
			InTLLRN_ControlRig->OnPreForwardsSolve_AnyThread().Remove(PreForwardHandle);
			PreForwardHandle.Reset();
		}
		if(PostForwardHandle.IsValid())
		{
			InTLLRN_ControlRig->OnPostForwardsSolve_AnyThread().Remove(PostForwardHandle);
			PostForwardHandle.Reset();
		}
	}
}
