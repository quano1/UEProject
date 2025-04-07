// Copyright Epic Games, Inc. All Rights Reserved.

/**
 *
 * Control Rig Sequencer Exposure
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MovieSceneBindingProxy.h"
#include "Tools/TLLRN_ControlRigSnapper.h"
#include "TransformNoScale.h"
#include "EulerTransform.h"
#include "MovieSceneToolsUserSettings.h"
#include "Tools/TLLRN_ControlRigSnapSettings.h"
#include "MovieSceneTimeUnit.h"
#include "TLLRN_TLLRN_RigSpacePickerBakeSettings.h"
#include "Filters/CurveEditorSmartReduceFilter.h"
#include "TLLRN_ControlRigSequencerEditorLibrary.generated.h"

class ULevelSequence;
class UTickableConstraint;
class UTickableTransformConstraint;
class UTransformableHandle;
struct FBakingAnimationKeySettings;
class UTLLRN_ControlRig;
class UTLLRN_AnimLayer;

USTRUCT(BlueprintType)
struct FTLLRN_ControlRigSequencerBindingProxy
{
	GENERATED_BODY()

	FTLLRN_ControlRigSequencerBindingProxy()
		: TLLRN_ControlRig(nullptr)
		, Track(nullptr)
	{}

	FTLLRN_ControlRigSequencerBindingProxy(const FMovieSceneBindingProxy& InProxy, UTLLRN_ControlRig* InTLLRN_ControlRig, UMovieSceneTLLRN_ControlRigParameterTrack* InTrack)
		: Proxy(InProxy)
		, TLLRN_ControlRig(InTLLRN_ControlRig)
		, Track(InTrack)
	{}

	UPROPERTY(BlueprintReadOnly, Category = TLLRN_ControlRig)
	FMovieSceneBindingProxy Proxy;

	UPROPERTY(BlueprintReadOnly, Category = TLLRN_ControlRig)
	TObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRig;

	UPROPERTY(BlueprintReadOnly, Category = TLLRN_ControlRig)
	TObjectPtr<UMovieSceneTLLRN_ControlRigParameterTrack> Track;
};

UENUM(BlueprintType)
enum class ETLLRN_AnimToolBlendOperation : uint8
{
	Tween,
	BlendToNeighbor,
	PushPull,
	BlendRelative,
	BlendToEase,
	SmoothRough,
};

/**
* This is a set of helper functions to access various parts of the Sequencer and Control Rig API via Python and Blueprints.
*/
UCLASS(meta=(Transient, ScriptName="TLLRN_ControlRigSequencerLibrary"))
class TLLRN_CONTROLRIGEDITOR_API UTLLRN_ControlRigSequencerEditorLibrary : public UBlueprintFunctionLibrary
{

	public:
	GENERATED_BODY()

public:

	/**
	* Get all of the visible control rigs in the level
	* @return returns list of visible Control Rigs
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<UTLLRN_ControlRig*> GetVisibleTLLRN_ControlRigs();

	/**
	* Get all of the control rigs and their bindings in the level sequence
	* @param LevelSequence The movie scene sequence to look for Control Rigs
	* @return returns list of Control Rigs in the level sequence.
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<FTLLRN_ControlRigSequencerBindingProxy> GetTLLRN_ControlRigs(ULevelSequence* LevelSequence);

	/**
	* Find or create a Control Rig track of a specific class based upon the binding
	* @param World The world used to spawn into temporarily if binding is a spawnable
	* @param LevelSequence The LevelSequence to find or create
	* @param TLLRN_ControlRigClass The class of the Control Rig
	* @param InBinding The binding (actor or component binding) to find or create the Control Rig track
	* @return returns Return the found or created track
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static UMovieSceneTrack* FindOrCreateTLLRN_ControlRigTrack(UWorld* World, ULevelSequence* LevelSequence, const UClass* TLLRN_ControlRigClass, const FMovieSceneBindingProxy& InBinding, bool bIsLayeredTLLRN_ControlRig = false);

	/**
	* Find or create a Control Rig Component
	* @param World The world used to spawn into temporarily if binding is a spawnable
	* @param LevelSequence The LevelSequence to find or create
	* @param InBinding The binding (actor or component binding) to find or create the Control Rig tracks
	* @return returns Find array of component Control Rigs that were found or created
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<UMovieSceneTrack*> FindOrCreateTLLRN_ControlRigComponentTrack(UWorld* World, ULevelSequence* LevelSequence, const FMovieSceneBindingProxy& InBinding);
	
	/**
	* Load anim sequence into this control rig section
	* @param MovieSceneSection The MovieSceneSectionto load into
	* @param AnimSequence The Sequence to load
	* @param MovieScene The MovieScene getting loaded into
	* @param SkelMeshComponent The Skeletal Mesh component getting loaded into.
	* @param InStartFrame Frame to insert the animation
	* @param TimeUnit Unit for all frame and time values, either in display rate or tick resolution
	* @param bKeyReduce If true do key reduction based upon Tolerance, if false don't
	* @param Tolerance If reducing keys, tolerance about which keys will be removed, smaller tolerance, more keys usually.
	* @param Interpolation The key interpolation type to set the keys, defaults to EMovieSceneKeyInterpolation::SmartAuto
	* @param bResetControls If true will reset all controls to initial value on every frame
	* @return returns True if successful, False otherwise
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool LoadAnimSequenceIntoTLLRN_ControlRigSection(UMovieSceneSection* MovieSceneSection, UAnimSequence* AnimSequence, USkeletalMeshComponent* SkelMeshComp,
		FFrameNumber InStartFrame, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate, bool bKeyReduce = false, float Tolerance = 0.001f,
		EMovieSceneKeyInterpolation Interpolation = EMovieSceneKeyInterpolation::SmartAuto, bool bResetControls = true);

	/**
	* Bake the current animation in the binding to a Control Rig track
	* @param World The active world
	* @param LevelSequence The LevelSequence we are baking
	* @param TLLRN_ControlRigClass The class of the Control Rig
	* @param ExportOptions Export options for creating an animation sequence
	* @param bKeyReduce If true do key reduction based upon Tolerance, if false don't
	* @param Tolerance If reducing keys, tolerance about which keys will be removed, smaller tolerance, more keys usually.
	* @param Binding The binding upon which to bake
	* @param bResetControls If true will reset all controls to initial value on every frame
	* @return returns True if successful, False otherwise
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool BakeToTLLRN_ControlRig(UWorld* World, ULevelSequence* LevelSequence, UClass* TLLRN_ControlRigClass, UAnimSeqExportOption* ExportOptions, bool bReduceKeys, float Tolerance,
			const FMovieSceneBindingProxy& Binding, bool bResetControls = true);

	/**
	* Peform new Smart Reduce filter over the specified control rig section in the current open level sequence. Note existing
	* functions like LoadAnimSequenceIntoTLLRN_ControlRigSection and BakeToTLLRN_ControlRig, will still use the old key reduction algorithm,
	* so if you want to bake and then key reduce with the new function, set the bKeyReduce param as false with those functions,
	* but then call this function after.
	* @param ReduceParams Key reduction parameters
	* @param MovieSceneSection The Control rig section we want to reduce
	* @return returns True if successful, False otherwise
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool SmartReduce(FSmartReduceParams& ReduceParams, UMovieSceneSection* MovieSceneSection);


	/**
	* Bake the constraint to keys based on the passed in frames. This will use the open sequencer to bake. See ConstraintsScriptingLibrary to get the list of available constraints
	* @param World The active world
	* @param Constraint The Constraint to bake. After baking it will be keyed to be inactive of the range of frames that are baked
	* @param Frames The frames to bake, if the array is empty it will use the active time ranges of the constraint to determine where it should bake
	* @param TimeUnit Unit for all frame and time values, either in display rate or tick resolution
	* @return Returns True if successful, False otherwise
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool BakeConstraint(UWorld* World, UTickableConstraint* Constraint, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Bake the constraint to keys based on the passed in settings. This will use the open sequencer to bake. See ConstraintsScriptingLibrary to get the list of available constraints
	* @param World The active world
	* @param InConstraints The Constraints tobake.  After baking they will be keyed to be inactive of the range of frames that are baked
	* @param InSettings Settings to use for baking
	* @return Returns True if successful, False otherwise
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool BakeConstraints(UWorld* World, TArray<UTickableConstraint*>& InConstraints,const FBakingAnimationKeySettings& InSettings);
	/**
	* Add a constraint possibly adding to sequencer also if one is open.
	* @param World The active world
	* @param InType Type of constraint to create
	* @param InChild The handle to the transormable to be constrainted
	* @param InParent The handle to the parent of the constraint
	* @param bMaintainOffset Whether to maintain offset between child and parent when setting the constraint
	* @return Returns the constraint if created all nullptr if not
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static UTickableConstraint* AddConstraint(UWorld* World, ETransformConstraintType InType, UTransformableHandle* InChild, UTransformableHandle* InParent, const bool bMaintainOffset);
	
	/**
	* Get the constraint keys for the specified constraint
	* @param InConstraint The constraint to get
	* @param ConstraintSection Section containing Cosntraint Key
	* @param OutBools Array of whether or not it's active at the specified times
	* @param OutFrames The Times for the keys
	* @param TimeUnit Unit for the time params, either in display rate or tick resolution
	* @return Returns true if we got the keys from this constraint
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool GetConstraintKeys(UTickableConstraint* InConstraint, UMovieSceneSection* ConstraintSection,TArray<bool>& OutBools, TArray<FFrameNumber>& OutFrames, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);
	
	/**
	* Set the constraint active key in the current open Sequencer
	* @param InConstraint The constraint to set the key
	* @param bActive Whether or not it's active
	* @param FrameTime Time to set the value
	* @param TimeUnit Unit for the time params, either in display rate or tick resolution
	* @return Returns true if we set the constraint to be the passed in value, false if not. We may not do so if the value is the same.
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool SetConstraintActiveKey(UTickableConstraint* InConstraint, bool bActive, FFrameNumber InFrame, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get all constraints for this object, which is described by a transformable handle
	* @param InChild The handle to look for constraints controlling it
	* @return Returns array of Constraints this handle is constrained to.
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray <UTickableConstraint*> GetConstraintsForHandle(UWorld* InWorld, const UTransformableHandle* InChild);

	/** Move the constraint active key in the current open Sequencer
	* @param InConstraint The constraint whose key to move
	* @param ConstraintSection Section containing Cosntraint Key
	* @param InTime Original time of the constraint key
	* @param InNewTime New time for the constraint key
	* @param TimeUnit Unit for the time params, either in display rate or tick resolution
	* @return Will return false if function fails, for example if there is no key at this time it will fail, or if the new time is invalid it could fail also
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool MoveConstraintKey(UTickableConstraint* Constraint, UMovieSceneSection* ConstraintSection, FFrameNumber InTime, FFrameNumber InNewTime, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/** Delete the Key for the Constraint at the specified time. 
	* @param InConstraint The constraint whose key to move
	* @param ConstraintSection Section containing Cosntraint Key
	* @param InTime Time to delete the constraint.
	* @param TimeUnit Unit for the InTime, either in display rate or tick resolution
	* @return Will return false if function fails,  for example if there is no key at this time it will fail.
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool DeleteConstraintKey(UTickableConstraint* Constraint, UMovieSceneSection* ConstraintSection, FFrameNumber InTime, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);
	/**
	* Compensate constraint at the specfied time 
	* @param InConstraint The constraint to compensate
	* @param InTime Time to compensate
	* @param TimeUnit Unit for the InTime, either in display rate or tick resolution
	* @return Returns true if it can compensate
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool Compensate(UTickableConstraint* InConstraint,  FFrameNumber InTime, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Compensate constraint at all keys
	* @param InConstraint The constraint to compensate
	* @return Returns true if it can compensate
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool CompensateAll(UTickableConstraint* InConstraint);

	/**
	* Peform a Tween operation on the current active sequencer time(must be visible).
	* @param LevelSequence The LevelSequence that's loaded in the editor
	* @param TLLRN_ControlRig The Control Rig to tween.
	* @param TweenValue The tween value to use, range from -1(blend to previous) to 1(blend to next)
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool TweenTLLRN_ControlRig(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, float TweenValue);
	
	/**
	* Peform specified blend operation based upon selected keys in the curve editor or selected control rig controls
	* @param LevelSequence The LevelSequence that's loaded in the editor
	* @param ETLLRN_AnimToolBlendOperation The operation to perform
	* @param BlendValue The blend value to use, range from -1(blend to previous) to 1(blend to next)
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool BlendValuesOnSelected(ULevelSequence* LevelSequence, ETLLRN_AnimToolBlendOperation BlendOperation, 
		float BlendValue);

	/**
	* Peform a Snap operation to snap the children to the parent. 
	* @param LevelSequence Active Sequence to snap
	* @param StartFrame Beginning of the snap
	* @param EndFrame End of the snap
	* @param ChildrenToSnap The children objects that snap and get keys set onto. They need to live in an active Sequencer in the level editor
	* @param ParentToSnap The parent object to snap relative to. If animated, it needs to live in an active Sequencer in the level editor
	* @param SnapSettings Settings to use
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param Returns True if successful
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool SnapTLLRN_ControlRig(ULevelSequence* LevelSequence, FFrameNumber StartFrame, FFrameNumber EndFrame, const FTLLRN_ControlRigSnapperSelection& ChildrenToSnap,
		const FTLLRN_ControlRigSnapperSelection& ParentToSnap, const UTLLRN_ControlRigSnapSettings* SnapSettings, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);
	
	/**
	* Get Actors World Transform at a specific time
	* @param LevelSequence Active Sequence to get transform for
	* @param Actor The actor
	* @param Frame Time to get the transform
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns World Transform
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static FTransform GetActorWorldTransform(ULevelSequence* LevelSequence,AActor* Actor, FFrameNumber Frame, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get Actors World Transforms at specific times
	* @param LevelSequence Active Sequence to get transform for
	* @param Actor The actor
	* @param Frames Times to get the transform
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns World Transforms
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<FTransform> GetActorWorldTransforms(ULevelSequence* LevelSequence, AActor* Actor, const TArray<FFrameNumber>& Frames, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get SkeletalMeshComponent World Transform at a specific time
	* @param LevelSequence Active Sequence to get transform for
	* @param SkeletalMeshComponent The SkeletalMeshComponent
	* @param Frame Time to get the transform
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param ReferenceName Optional name of the referencer
	* @return Returns World Transform
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static FTransform GetSkeletalMeshComponentWorldTransform(ULevelSequence* LevelSequence, USkeletalMeshComponent* SkeletalMeshComponent, FFrameNumber Frame,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate,FName ReferenceName = NAME_None);

	/**
	* Get SkeletalMeshComponents World Transforms at specific times
	* @param LevelSequence Active Sequence to get transform for
	* @param SkeletalMeshComponent The SkeletalMeshComponent
	* @param Frames Times to get the transform
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param ReferenceName Optional name of the referencer
	* @return Returns World Transforms
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<FTransform> GetSkeletalMeshComponentWorldTransforms(ULevelSequence* LevelSequence, USkeletalMeshComponent* SkeletalMeshComponent, const TArray<FFrameNumber>& Frames, 
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate,FName ReferenceName = NAME_None);

	/**
	* Get TLLRN_ControlRig Control's World Transform at a specific time
	* @param LevelSequence Active Sequence to get transform for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control
	* @param Frame Time to get the transform
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns World Transform
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static FTransform GetTLLRN_ControlRigWorldTransform(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's World Transforms at specific times
	* @param LevelSequence Active Sequence to get transform for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control 
	* @param Frames Times to get the transform
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns World Transforms
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<FTransform> GetTLLRN_ControlRigWorldTransforms(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Set TLLRN_ControlRig Control's World Transform at a specific time
	* @param LevelSequence Active Sequence to set transforms for. Must be loaded in Level Editor.
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control
	* @param Frame Time to set the transform
	* @param WorldTransform World Transform to set
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param bSetKey Whether or not to set a key.
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetTLLRN_ControlRigWorldTransform(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, const FTransform& WorldTransform,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate, bool bSetKey = true);

	/**
	* Set TLLRN_ControlRig Control's World Transforms at a specific times.
	* @param LevelSequence Active Sequence to set transforms for. Must be loaded in Level Editor.
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control
	* @param Frames Times to set the transform
	* @param WorldTransform World Transform to set
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetTLLRN_ControlRigWorldTransforms(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, 
		 const TArray<FTransform>& WorldTransforms, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);
	
	/**
	* Get TLLRN_ControlRig Control's float value at a specific time
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a float control
	* @param Frame Time to get the value
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Value at that time
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static float GetLocalTLLRN_ControlTLLRN_RigFloat(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's float values at specific times
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a float control
	* @param Frames Times to get the values
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Values at those times
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<float> GetLocalTLLRN_ControlTLLRN_RigFloats(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Set TLLRN_ControlRig Control's float value at specific time
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a float control
	* @param Frame Time to set the value
	* @param Value The value to set
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param bSetKey If True set a key, if not just set the value
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlTLLRN_RigFloat(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, float Value,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate,bool bSetKey = true);

	/**
	* Set TLLRN_ControlRig Control's float values at specific times
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a float control
	* @param Frames Times to set the values
	* @param Values The values to set at those times
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlTLLRN_RigFloats(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, const TArray<float> Values,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's bool value at a specific time
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a bool control
	* @param Frame Time to get the value
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Value at that time
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool GetLocalTLLRN_ControlTLLRN_RigBool(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's bool values at specific times
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a bool control
	* @param Frames Times to get the values
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Values at those times
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<bool> GetLocalTLLRN_ControlTLLRN_RigBools(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Set TLLRN_ControlRig Control's bool value at specific time
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a bool control
	* @param Frame Time to set the value
	* @param Value The value to set
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param bSetKey If True set a key, if not just set the value
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlTLLRN_RigBool(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, bool Value,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate, bool bSetKey = true);

	/**
	* Set TLLRN_ControlRig Control's bool values at specific times
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a bool control
	* @param Frames Times to set the values
	* @param Values The values to set at those times
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlTLLRN_RigBools(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
		const TArray<bool> Values, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's integer value at a specific time
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a integer control
	* @param Frame Time to get the value
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Value at that time
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static int32 GetLocalTLLRN_ControlRigInt(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, 
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's integer values at specific times
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a intteger control
	* @param Frames Times to get the values
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Values at those times
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<int32> GetLocalTLLRN_ControlRigInts(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Set TLLRN_ControlRig Control's int value at specific time
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a int control
	* @param Frame Time to set the value
	* @param Value The value to set
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param bSetKey If True set a key, if not just set the value
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlRigInt(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, int32 Value,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate,bool bSetKey = true);


	/**
	* Set TLLRN_ControlRig Control's int values at specific times
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a int control
	* @param Frames Times to set the values
	* @param Values The values to set at those times
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlRigInts(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, const TArray<int32> Values,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's Vector2D value at a specific time
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Vector2D control
	* @param Frame Time to get the value
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Value at that time
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static FVector2D GetLocalTLLRN_ControlTLLRN_RigVector2D(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's Vector2D values at specific times
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Vector2D control
	* @param Frames Times to get the values
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Values at those times
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<FVector2D> GetLocalTLLRN_ControlTLLRN_RigVector2Ds(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Set TLLRN_ControlRig Control's Vector2D value at specific time
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Vector2D control
	* @param Frame Time to set the value
	* @param Value The value to set
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param bSetKey If True set a key, if not just set the value
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlTLLRN_RigVector2D(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FVector2D Value,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate,bool bSetKey = true);


	/**
	* Set TLLRN_ControlRig Control's Vector2D values at specific times
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Vector2D control
	* @param Frames Times to set the values
	* @param Values The values to set at those times
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlTLLRN_RigVector2Ds(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, const TArray<FVector2D> Values,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's Position value at a specific time
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Position control
	* @param Frame Time to get the value
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Value at that time
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static FVector GetLocalTLLRN_ControlRigPosition(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's Position values at specific times
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Position control
	* @param Frames Times to get the values
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Values at those times
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<FVector> GetLocalTLLRN_ControlRigPositions(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Set TLLRN_ControlRig Control's Position value at specific time
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Position control
	* @param Frame Time to set the value
	* @param Value The value to set
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param bSetKey If True set a key, if not just set the value
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlRigPosition(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FVector Value,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate, bool bSetKey = true);

	/**
	* Set TLLRN_ControlRig Control's Position values at specific times
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Position control
	* @param Frames Times to set the values
	* @param Values The values to set at those times
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlRigPositions(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, const TArray<FVector> Values,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's Rotator value at a specific time
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Rotator control
	* @param Frame Time to get the value
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Value at that time
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static FRotator GetLocalTLLRN_ControlTLLRN_RigRotator(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's Rotator values at specific times
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Rotator control
	* @param Frames Times to get the values
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Values at those times
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<FRotator> GetLocalTLLRN_ControlTLLRN_RigRotators(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Set TLLRN_ControlRig Control's Rotator value at specific time
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Rotator control
	* @param Frame Time to set the value
	* @param Value The value to set
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param bSetKey If True set a key, if not just set the value
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlTLLRN_RigRotator(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FRotator Value, 
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate, bool bSetKey = true);

	/**
	* Set TLLRN_ControlRig Control's Rotator values at specific times
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Rotator control
	* @param Frames Times to set the values
	* @param Values The values to set at those times
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlTLLRN_RigRotators(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, const TArray<FRotator> Values,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's Scale value at a specific time
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Scale control
	* @param Frame Time to get the value
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Value at that time
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static FVector GetLocalTLLRN_ControlRigScale(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's Scale values at specific times
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Scale control
	* @param Frames Times to get the values
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Values at those times
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<FVector> GetLocalTLLRN_ControlRigScales(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Set TLLRN_ControlRig Control's Scale value at specific time
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Scale control
	* @param Frame Time to set the value
	* @param Value The value to set
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param bSetKey If True set a key, if not just set the value
*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlRigScale(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FVector Value, 
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate, bool bSetKey = true);

	/**
	* Set TLLRN_ControlRig Control's Scale values at specific times
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Scale control
	* @param Frames Times to set the values
	* @param Values The values to set at those times
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlRigScales(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, const TArray<FVector> Values,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);


	/**
	* Get TLLRN_ControlRig Control's EulerTransform value at a specific time
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a EulerTransfom control
	* @param Frame Time to get the value
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Value at that time
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static FEulerTransform GetLocalTLLRN_ControlRigEulerTransform(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's EulerTransform values at specific times
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a EulerTransform control
	* @param Frames Times to get the values
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Values at those times
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<FEulerTransform> GetLocalTLLRN_ControlRigEulerTransforms(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Set TLLRN_ControlRig Control's EulerTransform value at specific time
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a EulerTransform control
	* @param Frame Time to set the value
	* @param Value The value to set
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param bSetKey If True set a key, if not just set the value
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlRigEulerTransform(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FEulerTransform Value,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate, bool bSetKey = true);

	/**
	* Set TLLRN_ControlRig Control's EulerTransform values at specific times
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a EulerTransform control
	* @param Frames Times to set the values
	* @param Values The values to set at those times
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlRigEulerTransforms(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, const TArray<FEulerTransform> Values,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's TransformNoScale value at a specific time
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a TransformNoScale control
	* @param Frame Time to get the value
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Value at that time
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static FTransformNoScale GetLocalTLLRN_ControlTLLRN_RigTransformNoScale(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's TransformNoScale values at specific times
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a TransformNoScale control
	* @param Frames Times to get the values
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Values at those times
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<FTransformNoScale> GetLocalTLLRN_ControlTLLRN_RigTransformNoScales(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Set TLLRN_ControlRig Control's TransformNoScale value at specific time
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a TransformNoScale control
	* @param Frame Time to set the value
	* @param Value The value to set
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param bSetKey If True set a key, if not just set the value
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlTLLRN_RigTransformNoScale(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FTransformNoScale Value, 
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate, bool bSetKey = true);

	/**
	* Set TLLRN_ControlRig Control's TransformNoScale values at specific times
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a TransformNoScale control
	* @param Frames Times to set the values
	* @param Values The values to set at those times
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlTLLRN_RigTransformNoScales(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, const TArray<FTransformNoScale> Values,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's Transform value at a specific time
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Transform control
	* @param Frame Time to get the value
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Value at that time
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static FTransform GetLocalTLLRN_ControlTLLRN_RigTransform(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, 
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Get TLLRN_ControlRig Control's Transform values at specific times
	* @param LevelSequence Active Sequence to get value for
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Transform control
	* @param Frames Times to get the values
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @return Returns Values at those times
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static TArray<FTransform> GetLocalTLLRN_ControlTLLRN_RigTransforms(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/**
	* Set TLLRN_ControlRig Control's Transform value at specific time
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Transform control
	* @param Frame Time to set the value
	* @param Value The value to set 
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	* @param bSetKey If True set a key, if not just set the value
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlTLLRN_RigTransform(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, FFrameNumber Frame, FTransform Value, 
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate, bool bSetKey = true);

	/**
	* Set TLLRN_ControlRig Control's Transform values at specific times
	* @param LevelSequence Active Sequence to set value on
	* @param TLLRN_ControlRig The TLLRN_ControlRig
	* @param ControlName Name of the Control, should be a Transform control
	* @param Frames Times to set the values
	* @param Values The values to set at those times
	* @param TimeUnit Unit for frame values, either in display rate or tick resolution
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetLocalTLLRN_ControlTLLRN_RigTransforms(ULevelSequence* LevelSequence, UTLLRN_ControlRig* TLLRN_ControlRig, FName ControlName, const TArray<FFrameNumber>& Frames, const TArray<FTransform> Values,
		EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/*
	 * Import FBX onto a control rig with the specified track and section
	 *
	 * @param InWorld World to import to
	 * @param InSequence Sequence to import
	 * @param InTrack Track to import onto
	 * @param InSection Section to import onto, may be null in which case we use the track's section to key
	 * @param SelectedTLLRN_ControlTLLRN_RigNames  List of selected control rig names. Will use them if  ImportFBXTLLRN_ControlRigSettings->bImportOntoSelectedControls is true
	 * @param ImportFBXTLLRN_ControlRigSettings Settings to control import.
	 * @param InImportFileName Path to fbx file to create
	 */
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | FBX")
	static bool ImportFBXToTLLRN_ControlRigTrack(UWorld* World, ULevelSequence* InSequence, UMovieSceneTLLRN_ControlRigParameterTrack* InTrack, UMovieSceneTLLRN_ControlRigParameterSection* InSection,
			const TArray<FString>& SelectedTLLRN_ControlTLLRN_RigNames,
			UMovieSceneUserImportFBXControlRigSettings* ImportFBXTLLRN_ControlRigSettings,
			const FString& ImportFilename);

	/** Exports an FBX from the given control rig section. */
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | FBX")
	static bool ExportFBXFromTLLRN_ControlRigSection(ULevelSequence* Sequence, const UMovieSceneTLLRN_ControlRigParameterSection* Section,
	                                    const UMovieSceneUserExportFBXControlRigSettings* ExportFBXTLLRN_ControlRigSettings);

	/*
	 * Collapse and bake all sections and layers on a control rig track to just one section.
	 *
	 * @param InSequence Sequence that has track to collapse
	 * @param InTrack Track for layers to collapse
	 * @param bKeyReduce If true do key reduction based upon Tolerance, if false don't
	 * @param Tolerance If reducing keys, tolerance about which keys will be removed, smaller tolerance, more keys usually.
	 */
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool CollapseTLLRN_ControlRigTLLRN_TLLRN_AnimLayers(ULevelSequence* InSequence,UMovieSceneTLLRN_ControlRigParameterTrack* InTrack, bool bKeyReduce = false, float Tolerance = 0.001f);

	/*
	 * Collapse and bake all sections and layers on a control rig track to just one section using passed in settings.
	 *
	 * @param InSequence Sequence that has track to collapse
	 * @param InTrack Track for layers to collapse
	 * @param InSettings Settings that determine how to collapse
	 */
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool CollapseTLLRN_ControlRigTLLRN_TLLRN_AnimLayersWithSettings(ULevelSequence* InSequence, UMovieSceneTLLRN_ControlRigParameterTrack* InTrack, const FBakingAnimationKeySettings& InSettings);

	/*
	 * Get the default parent key, can be used a parent space.
	 *
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static FTLLRN_RigElementKey GetDefaultParentKey();

	/*
	 * Get the default world space key, can be used a world space.
	 *
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static FTLLRN_RigElementKey GetWorldSpaceReferenceKey();

	/*
	 * Set the a key for the Control Rig Space for the Control at the specified time. If space is the same as the current no key witll be set.
	 *
	 * @param InSequence Sequence to set the space
	 * @param InTLLRN_ControlRig TLLRN_ControlRig with the Control
	 * @param InControlName The name of the Control
	 * @param InSpaceKey  The new space for the Control
	 * @param InTime Time to change the space.
	 * @param TimeUnit Unit for the InTime, either in display rate or tick resolution
	 */
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool SetTLLRN_ControlTLLRN_RigSpace(ULevelSequence* InSequence, UTLLRN_ControlRig* InTLLRN_ControlRig, FName InControlName, const FTLLRN_RigElementKey& InSpaceKey, FFrameNumber InTime,  EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/** Bake specified Control Rig Controls to a specified Space based upon the current settings
	* @param InSequence Sequence to bake
	* @param InTLLRN_ControlRig TLLRN_ControlRig to bake
	* @param InControlNames The name of the Controls to bake
	* @param InSettings  The settings for the bake, e.g, how long to bake, to key reduce etc.
	* @param TimeUnit Unit for the start and end times in the InSettings parameter.
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool BakeTLLRN_ControlTLLRN_RigSpace(ULevelSequence* InSequence, UTLLRN_ControlRig* InTLLRN_ControlRig, const TArray<FName>& InControlNames, FTLLRN_TLLRN_RigSpacePickerBakeSettings InSettings, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);
	
	/** Perform compensation for any spaces at the specified time for the specified control rig
	* @param InTLLRN_ControlRig Control Rig to compensate
	* @param InTime  The time to look for a space key to compensate
	* @param TimeUnit Unit for the InTime
	* @return Will return false if function fails
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool SpaceCompensate(UTLLRN_ControlRig* InTLLRN_ControlRig, FFrameNumber InTime, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/** Perform compensation for all spaces for the specified control rig
	* @param InTLLRN_ControlRig Control Rig to compensate
	* @return Will return false if function fails
	*/
	static bool SpaceCompensateAll(UTLLRN_ControlRig* InTLLRN_ControlRig);

	/** Delete the Control Rig Space Key for the Control at the specified time. This will delete any attached Control Rig keys at this time and will perform any needed compensation to the new space.
	*
	* @param InSequence Sequence to set the space
	* @param InTLLRN_ControlRig TLLRN_ControlRig with the Control
	* @param InControlName The name of the Control
	* @param InTime Time to delete the space.
	* @param TimeUnit Unit for the InTime, either in display rate or tick resolution
	* @return Will return false if function fails,  for example if there is no key at this time it will fail.
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool DeleteTLLRN_ControlTLLRN_RigSpace(ULevelSequence* InSequence, UTLLRN_ControlRig* InTLLRN_ControlRig, FName InControlName, FFrameNumber InTime, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/** Move the Control Rig Space Key for the Control at the specified time to the new time. This will also move any Control Rig keys at this space switch boundary.
	*
	* @param InSequence Sequence to set the space
	* @param InTLLRN_ControlRig TLLRN_ControlRig with the Control
	* @param InControlName The name of the Control
	* @param InTime Original time of the space key
	* @param InNewTime New time for the space key
	* @param TimeUnit Unit for the time params, either in display rate or tick resolution
	* @return Will return false if function fails, for example if there is no key at this time it will fail, or if the new time is invalid it could fail also
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool MoveTLLRN_ControlTLLRN_RigSpace(ULevelSequence* InSequence, UTLLRN_ControlRig* InTLLRN_ControlRig, FName InControlName, FFrameNumber InTime, FFrameNumber InNewTime, EMovieSceneTimeUnit TimeUnit = EMovieSceneTimeUnit::DisplayRate);

	/** Rename the Control Rig Channels in Sequencer to the specified new control names, which should be present on the Control Rig
	* @param InSequence Sequence to rename controls
	* @param InTLLRN_ControlRig TLLRN_ControlRig to rename controls
	* @param InOldControlNames The name of the old Control Rig Control Channels to change. Will be replaced by the corresponding name in the InNewControlNames array
	* @param InNewControlNames  The name of the new Control Rig Channels 
	* @return Return true if the function succeeds, false if it doesn't which can happen if the name arrays don't match in size or any of the new Control Names aren't valid
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool RenameTLLRN_ControlTLLRN_RigControlChannels(ULevelSequence* InSequence, UTLLRN_ControlRig* InTLLRN_ControlRig, const TArray<FName>& InOldControlNames, const TArray<FName>& InNewControlNames);

	/** Get the controls mask for the given ControlName */
	UFUNCTION(BlueprintPure, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool GetControlsMask(UMovieSceneSection* InSection, FName ControlName);

	/** Set the controls mask for the given ControlNames */
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetControlsMask(UMovieSceneSection* InSection, const TArray<FName>& ControlNames, bool bVisible);

	/** Shows all of the controls for the given section */
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void ShowAllControls(UMovieSceneSection* InSection);

	/** Hides all of the controls for the given section */
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void HideAllControls(UMovieSceneSection* InSection);

	/** Set Control Rig priority order */
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static void SetTLLRN_ControlRigPriorityOrder(UMovieSceneTrack* InSection,int32 PriorityOrder);

	/** Get Control Rig prirority order */
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static int32 GetTLLRN_ControlRigPriorityOrder(UMovieSceneTrack* InSection);

	/**	Whether or not the control rig is an FK Control Rig.
	@param InTLLRN_ControlRig Rig to test to see if FK Control Rig
	**/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool IsFKTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig);

	/**	Whether or not the control rig is an Layered Control Rig.
	@param InTLLRN_ControlRig Rig to test to see if Layered Control Rig
	**/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool IsLayeredTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig);

	/**
	 * Mark the layered mode of a control rig on the color and display name of a track
	 * @param InTrack The track to modify the color and display name if necessary
	 */
	static bool MarkLayeredModeOnTrackDisplay(UMovieSceneTLLRN_ControlRigParameterTrack* InTrack);

	/*
	 * Convert the control rig track into absolute or layered rig
	 *
	 * @param InTrack Control rig track to convert 
	 * @param bSetIsLayered Convert to layered rig if true, or absolute if false
	 */
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool SetTLLRN_ControlRigLayeredMode(UMovieSceneTLLRN_ControlRigParameterTrack* InTrack, bool bSetIsLayered);

	/**	Get FKTLLRN_ControlRig Apply Mode.
	@param InTLLRN_ControlRig Rig to test
	@return The ETLLRN_ControlRigFKRigExecuteMode mode it is in, either Replace,Additive or Direct
	**/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static ETLLRN_ControlRigFKRigExecuteMode  GetFKTLLRN_ControlRigApplyMode(UTLLRN_ControlRig* InTLLRN_ControlRig);

	/**	Set the FK Control Rig to apply mode
	@param InTLLRN_ControlRig Rig to set 
	@param InApplyMode Set the ETLLRN_ControlRigFKRigExecuteMode mode (Replace,Addtiive or Direct)
	@return returns True if the mode was set, may not be set if the Control Rig doesn't support these modes currently only FKTLLRN_ControlRig's do.
	**/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig")
	static bool SetTLLRN_ControlRigApplyMode(UTLLRN_ControlRig* InTLLRN_ControlRig, ETLLRN_ControlRigFKRigExecuteMode InApplyMode);

	/**
	* Delete anim layer at specified index
	* @param Index The index where the anim layer exists
	* @return Returns true if successful, false otherwise
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig | Animation Layers")
	static bool DeleteTLLRN_AnimLayer(int32 Index);

	/**
	* Duplicate anim layer at specified index
	* @param Index The index where the anim layer exists
	* @return Returns index of new layer, -1 if none created
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig | Animation Layers")
	static int32 DuplicateTLLRN_AnimLayer(int32 Index);

	/**
	* Add anim layer from objects selected in Sequencer
	* @return Returns Index of created anim layer
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig | Animation Layers")
	static int32 AddTLLRN_AnimLayerFromSelection();

	/**
	* Merge specified anim layers into one layer. Will merge onto the anim layer with the lowest index
	* @param Indices The indices to merge
	* @return Returns true if successful, false otherwise
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig | Animation Layers")
	static bool MergeTLLRN_TLLRN_AnimLayers(const TArray<int32>& Indices);

	/**
	* Get the animation layer objects
	* @return Returns array of anim layer objects if they exist on active Sequencer
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig | Animation Layers")
	static TArray<UTLLRN_AnimLayer*> GetTLLRN_TLLRN_AnimLayers();
	
	/**
	* Helper function to get the index in the anim layer array from the anim layer
	* @param Anim Layer to get the index for
	* @return Returns index for the anim layer or INDEX_NONE(-1) if it doesn't exist
	*/
	UFUNCTION(BlueprintCallable, Category = "Editor Scripting | Sequencer Tools | Control Rig | Animation Layers")
	static int32 GetTLLRN_AnimLayerIndex(UTLLRN_AnimLayer* TLLRN_AnimLayer);

};
