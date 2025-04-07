// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ConstraintsManager.h"
#include "Sections/MovieSceneParameterSection.h"
#include "Sections/MovieSceneConstrainedSection.h"
#include "UObject/ObjectMacros.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Sections/MovieSceneSubSection.h"
#include "TLLRN_ControlRig.h"
#include "MovieSceneSequencePlayer.h"
#include "Animation/AnimData/BoneMaskFilter.h"
#include "MovieSceneObjectBindingID.h"
#include "Compilation/MovieSceneTemplateInterrogation.h"
#include "Channels/MovieSceneIntegerChannel.h"
#include "Channels/MovieSceneByteChannel.h"
#include "Channels/MovieSceneBoolChannel.h"
#include "Sequencer/MovieSceneTLLRN_ControlTLLRN_RigSpaceChannel.h"
#include "ConstraintChannel.h"
#include "KeyParams.h"
#include "MovieSceneTLLRN_ControlRigParameterSection.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;
struct FKeyDataOptimizationParams;

struct TLLRN_CONTROLRIG_API FTLLRN_ControlRigBindingHelper
{
	static void BindToSequencerInstance(UTLLRN_ControlRig* TLLRN_ControlRig);
	static void UnBindFromSequencerInstance(UTLLRN_ControlRig* TLLRN_ControlRig);
};

struct FEnumParameterNameAndValue //uses uint8
{
	FEnumParameterNameAndValue(FName InParameterName, uint8 InValue)
	{
		ParameterName = InParameterName;
		Value = InValue;
	}

	FName ParameterName;

	uint8 Value;
};

struct FIntegerParameterNameAndValue
{
	FIntegerParameterNameAndValue(FName InParameterName, int32 InValue)
	{
		ParameterName = InParameterName;
		Value = InValue;
	}

	FName ParameterName;

	int32 Value;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_EnumParameterNameAndCurve
{
	GENERATED_USTRUCT_BODY()

	FTLLRN_EnumParameterNameAndCurve()
	{}

	FTLLRN_EnumParameterNameAndCurve(FName InParameterName);

	UPROPERTY()
	FName ParameterName;

	UPROPERTY()
	FMovieSceneByteChannel ParameterCurve;
};


USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_IntegerParameterNameAndCurve
{
	GENERATED_USTRUCT_BODY()

	FTLLRN_IntegerParameterNameAndCurve()
	{}
	FTLLRN_IntegerParameterNameAndCurve(FName InParameterName);

	UPROPERTY()
	FName ParameterName;

	UPROPERTY()
	FMovieSceneIntegerChannel ParameterCurve;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_SpaceControlNameAndChannel
{
	GENERATED_USTRUCT_BODY()

	FTLLRN_SpaceControlNameAndChannel(){}
	FTLLRN_SpaceControlNameAndChannel(FName InControlName) : ControlName(InControlName) {};

	UPROPERTY()
	FName ControlName;

	UPROPERTY()
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel SpaceCurve;
};


/**
*  Data that's queried during an interrogation
*/
struct FFloatInterrogationData
{
	float Val;
	FName ParameterName;
};

struct FVector2DInterrogationData
{
	FVector2D Val;
	FName ParameterName;
};

struct FVectorInterrogationData
{
	FVector Val;
	FName ParameterName;
};

struct FEulerTransformInterrogationData
{
	FEulerTransform Val;
	FName ParameterName;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_ChannelMapInfo
{
	GENERATED_USTRUCT_BODY()

	FTLLRN_ChannelMapInfo() = default;

	FTLLRN_ChannelMapInfo(int32 InControlIndex, int32 InTotalChannelIndex, int32 InChannelIndex, int32 InParentControlIndex = INDEX_NONE, FName InChannelTypeName = NAME_None,
		int32 InMaskIndex = INDEX_NONE, int32 InCategoryIndex = INDEX_NONE) :
		ControlIndex(InControlIndex),TotalChannelIndex(InTotalChannelIndex), ChannelIndex(InChannelIndex), ParentControlIndex(InParentControlIndex), ChannelTypeName(InChannelTypeName),MaskIndex(InMaskIndex),CategoryIndex(InCategoryIndex) {};
	UPROPERTY()
	int32 ControlIndex = 0; 
	UPROPERTY()
	int32 TotalChannelIndex = 0;
	UPROPERTY()
	int32 ChannelIndex = 0; //channel index for it's type.. (e.g  float, int, bool).
	UPROPERTY()
	int32 ParentControlIndex = 0;
	UPROPERTY()
	FName ChannelTypeName; 
	UPROPERTY()
	bool bDoesHaveSpace = false;
	UPROPERTY()
	int32 SpaceChannelIndex = -1; //if it has space what's the space channel index
	UPROPERTY()
	int32 MaskIndex = -1; //index for the mask
	UPROPERTY()
	int32 CategoryIndex = -1; //index for the Sequencer category node

	int32 GeneratedKeyIndex = -1; //temp index set by the TLLRN_ControlRigParameterTrack, not saved

	UPROPERTY()
	TArray<uint32> ConstraintsIndex; //constraints data
};


struct FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel;

/**
 * Movie scene section that controls animation controller animation
 */
UCLASS()
class TLLRN_CONTROLRIG_API UMovieSceneTLLRN_ControlRigParameterSection : public UMovieSceneParameterSection, public IMovieSceneConstrainedSection
{
	GENERATED_BODY()

public:

	/** Bindable events for when we add space or constraint channels. */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FSpaceChannelAddedEvent, UMovieSceneTLLRN_ControlRigParameterSection*, const FName&, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel*);

	void AddEnumParameterKey(FName InParameterName, FFrameNumber InTime, uint8 InValue);
	void AddIntegerParameterKey(FName InParameterName, FFrameNumber InTime, int32 InValue);

	bool RemoveEnumParameter(FName InParameterName);
	bool RemoveIntegerParameter(FName InParameterName);

	TArray<FTLLRN_EnumParameterNameAndCurve>& GetEnumParameterNamesAndCurves();
	const TArray<FTLLRN_EnumParameterNameAndCurve>& GetEnumParameterNamesAndCurves() const;

	TArray<FTLLRN_IntegerParameterNameAndCurve>& GetIntegerParameterNamesAndCurves();
	const TArray<FTLLRN_IntegerParameterNameAndCurve>& GetIntegerParameterNamesAndCurves() const;

	void FixRotationWinding(const FName& ControlName, FFrameNumber StartFrame, FFrameNumber EndFrame);
	void OptimizeSection(const FName& ControlName, const FKeyDataOptimizationParams& InParams);
	void AutoSetTangents(const FName& ControlName);

	TArray<FTLLRN_SpaceControlNameAndChannel>& GetSpaceChannels();
	const TArray< FTLLRN_SpaceControlNameAndChannel>& GetSpaceChannels() const;
	FName FindControlNameFromSpaceChannel(const FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* SpaceChannel) const;
	
	FSpaceChannelAddedEvent& SpaceChannelAdded() { return OnSpaceChannelAdded; }

	const FName& FindControlNameFromConstraintChannel(const FMovieSceneConstraintChannel* InConstraintChannel) const;
		
	bool RenameParameterName(const FName& OldParameterName, const FName& NewParameterName);

	void ChangeTLLRN_ControlRotationOrder(const FName& InControlName, const TOptional<EEulerRotationOrder>& CurrentOrder, 
		const  TOptional<EEulerRotationOrder>& NewOrder, EMovieSceneKeyInterpolation Interpolation);

private:

	FSpaceChannelAddedEvent OnSpaceChannelAdded;
	/** Control Rig that controls us*/
	UPROPERTY()
	TObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRig;

public:

	/** The class of control rig to instantiate */
	UPROPERTY(EditAnywhere, Category = "Animation")
	TSubclassOf<UTLLRN_ControlRig> TLLRN_ControlRigClass;

	/** Deprecrated, use ControlNameMask*/
	UPROPERTY()
	TArray<bool> ControlsMask;

	/** Names of Controls that are masked out on this section*/
	UPROPERTY()
	TSet<FName> ControlNameMask;

	/** Mask for Transform Mask*/
	UPROPERTY()
	FMovieSceneTransformMask TransformMask;

	/** The weight curve for this animation controller section */
	UPROPERTY()
	FMovieSceneFloatChannel Weight;

	/** Map from the control name to where it starts as a channel*/
	UPROPERTY()
	TMap<FName, FTLLRN_ChannelMapInfo> ControlChannelMap;


protected:
	/** Enum Curves*/
	UPROPERTY()
	TArray<FTLLRN_EnumParameterNameAndCurve> EnumParameterNamesAndCurves;

	/*Integer Curves*/
	UPROPERTY()
	TArray<FTLLRN_IntegerParameterNameAndCurve> IntegerParameterNamesAndCurves;

	/** Space Channels*/
	UPROPERTY()
	TArray<FTLLRN_SpaceControlNameAndChannel>  SpaceChannels;

	/** Space Channels*/
	UPROPERTY()
	TArray<FConstraintAndActiveChannel> ConstraintsChannels;

public:

	UMovieSceneTLLRN_ControlRigParameterSection();

	//UMovieSceneSection virtuals
	virtual void SetBlendType(EMovieSceneBlendType InBlendType) override;
	virtual UObject* GetImplicitObjectOwner() override;
	virtual EMovieSceneChannelProxyType CacheChannelProxy() override;

	// IMovieSceneConstrainedSection overrides
	/*
	* Whether it has that channel
	*/
	virtual bool HasConstraintChannel(const FGuid& InConstraintName) const override;

	/*
	* Get constraint with that name
	*/
	virtual FConstraintAndActiveChannel* GetConstraintChannel(const FGuid& InConstraintID) override;

	/*
	*  Add Constraint channel
	*/
	virtual void AddConstraintChannel(UTickableConstraint* InConstraint) override;

	/*
	*  Remove Constraint channel
	*/
	virtual void RemoveConstraintChannel(const UTickableConstraint* InConstraint) override;

	/*
	*  Get The channels
	*/
	virtual TArray<FConstraintAndActiveChannel>& GetConstraintsChannels()  override;

	/*
	*  Replace the constraint with the specified name with the new one
	*/
	virtual void ReplaceConstraint(const FName InConstraintName, UTickableConstraint* InConstraint)  override;

	/*
	*  What to do if the constraint object has been changed, for example by an undo or redo.
	*/
	virtual void OnConstraintsChanged() override;

	//not override but needed
	const TArray<FConstraintAndActiveChannel>& GetConstraintsChannels() const;

#if WITH_EDITOR
	//Function to save control rig key when recording.
	void RecordTLLRN_ControlRigKey(FFrameNumber FrameNumber, bool bSetDefault, EMovieSceneKeyInterpolation InInterpMode);

	//Function to load an Anim Sequence into this section. It will automatically resize to the section size.
	//Will return false if fails or is canceled
	virtual bool LoadAnimSequenceIntoThisSection(UAnimSequence* Sequence, const FFrameNumber& SequenceStart, UMovieScene* MovieScene, UObject* BoundObject, bool bKeyReduce, float Tolerance, bool bResetControls, const FFrameNumber& InStartFrame, EMovieSceneKeyInterpolation InInterpolation);

	UE_DEPRECATED(5.5, "LoadAnimSequenceIntoThisSection without taking a sequence start frame is deprecated, use version that takes start frame instead")
	virtual bool LoadAnimSequenceIntoThisSection(UAnimSequence* Sequence, UMovieScene* MovieScene, UObject* BoundObject, bool bKeyReduce, float Tolerance, bool bResetControls, FFrameNumber InStartFrame , EMovieSceneKeyInterpolation InInterpolation);
#endif
	
	void FillControlNameMask(bool bValue);

	void SetControlNameMask(const FName& Name, bool bValue);

	bool GetControlNameMask(const FName& Name) const;

	UE_DEPRECATED(5.5, "Use GetControlNameMask")
	const TArray<bool>& GetControlsMask() const
	{
		return ControlsMask;
	}
	
	UE_DEPRECATED(5.5, "Use GetControlNameMask")
	const TArray<bool>& GetControlsMask() 
	{
		if (ChannelProxy.IsValid() == false)
		{
			CacheChannelProxy();
		}
		return ControlsMask;
	}

	UE_DEPRECATED(5.5, "Use GetControlNameMask")
	bool GetControlsMask(int32 Index)  
	{
		if (ChannelProxy.IsValid() == false)
		{
			CacheChannelProxy();
		}
		if (Index >= 0 && Index < ControlsMask.Num())
		{
			return ControlsMask[Index];
		}
		return false;
	}

	UE_DEPRECATED(5.5, "Use SetControlNameMask")
	void SetControlsMask(const TArray<bool>& InMask)
	{
		ControlsMask = InMask;
		ReconstructChannelProxy();
	}

	UE_DEPRECATED(5.5, "Use SetControlNameMask")
	void SetControlsMask(int32 Index, bool Val)
	{
		if (Index >= 0 && Index < ControlsMask.Num())
		{
			ControlsMask[Index] = Val;
		}
		ReconstructChannelProxy();
	}

	UE_DEPRECATED(5.5, "Use FillControlNameMask")
	void FillControlsMask(bool Val)
	{
		ControlsMask.Init(Val, ControlsMask.Num());
		ReconstructChannelProxy();
	}
	
	/**
	* This function returns the active category index of the control, based upon what controls are active/masked or not
	* If itself is masked it returns INDEX_NONE
	*/
	int32 GetActiveCategoryIndex(FName ControlName) const;
	/**
	* Access the transform mask that defines which channels this track should animate
	*/
	FMovieSceneTransformMask GetTransformMask() const
	{
		return TransformMask;
	}

	/**
	 * Set the transform mask that defines which channels this track should animate
	 */
	void SetTransformMask(FMovieSceneTransformMask NewMask)
	{
		TransformMask = NewMask;
		ReconstructChannelProxy();
	}

public:

	/** Recreate with this Control Rig*/
	void RecreateWithThisTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bSetDefault);

	/* Set the control rig for this section */
	void SetTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig);
	/* Get the control rig for this section, by default in non-game world */
	UTLLRN_ControlRig* GetTLLRN_ControlRig(UWorld* InGameWorld = nullptr) const;

	/** Whether or not to key currently, maybe evaluating so don't*/
	void  SetDoNotKey(bool bIn) const { bDoNotKey = bIn; }
	/** Get Whether to key or not*/
	bool GetDoNotKey() const { return bDoNotKey; }

	/**  Whether or not this section has scalar*/
	bool HasScalarParameter(FName InParameterName) const;

	/**  Whether or not this section has bool*/
	bool HasBoolParameter(FName InParameterName) const;

	/**  Whether or not this section has enum*/
	bool HasEnumParameter(FName InParameterName) const;

	/**  Whether or not this section has int*/
	bool HasIntegerParameter(FName InParameterName) const;

	/**  Whether or not this section has scalar*/
	bool HasVector2DParameter(FName InParameterName) const;

	/**  Whether or not this section has scalar*/
	bool HasVectorParameter(FName InParameterName) const;

	/**  Whether or not this section has scalar*/
	bool HasColorParameter(FName InParameterName) const;

	/**  Whether or not this section has scalar*/
	bool HasTransformParameter(FName InParameterName) const;

	/**  Whether or not this section has space*/
	bool HasSpaceChannel(FName InParameterName) const;

	/** Get The Space Channel for the Control*/
	FTLLRN_SpaceControlNameAndChannel* GetSpaceChannel(FName InParameterName);

	/** Adds specified scalar parameter. */
	void AddScalarParameter(FName InParameterName,  TOptional<float> DefaultValue, bool bReconstructChannel);

	/** Adds specified bool parameter. */
	void AddBoolParameter(FName InParameterName, TOptional<bool> DefaultValue, bool bReconstructChannel);

	/** Adds specified enum parameter. */
	void AddEnumParameter(FName InParameterName, UEnum* Enum,TOptional<uint8> DefaultValue, bool bReconstructChannel);

	/** Adds specified int parameter. */
	void AddIntegerParameter(FName InParameterName, TOptional<int32> DefaultValue, bool bReconstructChannel);

	/** Adds a a key for a specific vector parameter. */
	void AddVectorParameter(FName InParameterName, TOptional<FVector> DefaultValue, bool bReconstructChannel);

	/** Adds a a key for a specific vector2D parameter. */
	void AddVector2DParameter(FName InParameterName, TOptional<FVector2D> DefaultValue, bool bReconstructChannel);

	/** Adds a a key for a specific color parameter. */
	void AddColorParameter(FName InParameterName, TOptional<FLinearColor> DefaultValue, bool bReconstructChannel);

	/** Adds a a key for a specific transform parameter*/
	void AddTransformParameter(FName InParameterName, TOptional<FEulerTransform> DefaultValue, bool bReconstructChannel);

	/** Add Space Parameter for a specified Control, no Default since that is Parent space*/
	void AddSpaceChannel(FName InControlName, bool bReconstructChannel);


	/** Clear Everything Out*/
	void ClearAllParameters();

	/** Evaluates specified scalar parameter. Will not get set if not found */
	TOptional<float> EvaluateScalarParameter(const  FFrameTime& InTime, FName InParameterName);

	/** Evaluates specified bool parameter. Will not get set if not found */
	TOptional<bool> EvaluateBoolParameter(const  FFrameTime& InTime, FName InParameterName);

	/** Evaluates specified enum parameter. Will not get set if not found */
	TOptional<uint8> EvaluateEnumParameter(const  FFrameTime& InTime, FName InParameterName);

	/** Evaluates specified int parameter. Will not get set if not found */
	TOptional<int32> EvaluateIntegerParameter(const  FFrameTime& InTime, FName InParameterName);

	/** Evaluates a a key for a specific vector parameter. Will not get set if not found */
	TOptional<FVector> EvaluateVectorParameter(const FFrameTime& InTime, FName InParameterName);

	/** Evaluates a a key for a specific vector2D parameter. Will not get set if not found */
	TOptional<FVector2D> EvaluateVector2DParameter(const  FFrameTime& InTime, FName InParameterName);

	/** Evaluates a a key for a specific color parameter. Will not get set if not found */
	TOptional<FLinearColor> EvaluateColorParameter(const  FFrameTime& InTime, FName InParameterName);

	/** Evaluates a a key for a specific transform parameter. Will not get set if not found */
	TOptional<FEulerTransform> EvaluateTransformParameter(const  FFrameTime& InTime, FName InParameterName);
	

	/** Evaluates a a key for a specific space parameter. Will not get set if not found */
	TOptional<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey> EvaluateSpaceChannel(const  FFrameTime& InTime, FName InParameterName);

	/** Key Zero Values on all or just selected controls in these section at the specified time */
	void KeyZeroValue(FFrameNumber InFrame, EMovieSceneKeyInterpolation DefaultInterpolation, bool bSelected);

	/** Key the Weights to the specified value */
	void KeyWeightValue(FFrameNumber InFrame, EMovieSceneKeyInterpolation DefaultInterpolation, float InVal);
;
	/** Remove All Keys, but maybe not space keys if bIncludeSpaceKeys is false */
	void RemoveAllKeys(bool bIncludeSpaceKeys);

	/** Whether or not create a space channel for a particular control */
	bool CanCreateSpaceChannel(FName InControlName) const;

public:
	/**
	* Access the interrogation key for control rig data 
	*/
	 static FMovieSceneInterrogationKey GetFloatInterrogationKey();
	 static FMovieSceneInterrogationKey GetVector2DInterrogationKey();
	 static FMovieSceneInterrogationKey GetVector4InterrogationKey();
	 static FMovieSceneInterrogationKey GetVectorInterrogationKey();
	 static FMovieSceneInterrogationKey GetTransformInterrogationKey();

	virtual void ReconstructChannelProxy() override;

protected:

	void ConvertMaskArrayToNameSet();

	//~ UMovieSceneSection interface
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostEditImport() override;
	virtual void PostLoad() override;
	virtual float GetTotalWeightValue(FFrameTime InTime) const override;
	virtual void OnBindingIDsUpdated(const TMap<UE::MovieScene::FFixedObjectBindingID, UE::MovieScene::FFixedObjectBindingID>& OldFixedToNewFixedMap, FMovieSceneSequenceID LocalSequenceID, TSharedRef<UE::MovieScene::FSharedPlaybackState> SharedPlaybackState) override;
	virtual void GetReferencedBindings(TArray<FGuid>& OutBindings) override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

#if WITH_EDITOR
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
#endif


	// When true we do not set a key on the section, since it will be set because we changed the value
	// We need this because control rig notifications are set on every change even when just changing sequencer time
	// which forces a sequencer eval, not like the editor where changes are only set on UI changes(changing time doesn't send change delegate)
	mutable bool bDoNotKey;

public:
	// Special list of Names that we should only Modify. Needed to handle Interaction (FK/IK) since Control Rig expecting only changed value to be set
	//not all Controls
	mutable TSet<FName> ControlsToSet;


public:
	//Test Controls really are new
	bool IsDifferentThanLastControlsUsedToReconstruct(const TArray<FTLLRN_RigControlElement*>& NewControls) const;

private:
	void StoreLastControlsUsedToReconstruct(const TArray<FTLLRN_RigControlElement*>& NewControls);
	//Last set of Controls used to reconstruct the channel proxies, used to make sure controls really changed if we want to reconstruct
	//only care to check name and type
	TArray<TPair<FName, ETLLRN_RigControlType>> LastControlsUsedToReconstruct;
};
