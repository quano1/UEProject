// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TransformableHandle.h"
#include "Rigs/TLLRN_RigHierarchyDefines.h"

#include "TLLRN_ControlTLLRN_RigTransformableHandle.generated.h"

struct FTLLRN_RigBaseElement;
struct FTLLRN_RigControlElement;

class UTLLRN_ControlRig;
class USkeletalMeshComponent;
class UTLLRN_ControlRigComponent;
class UTLLRN_RigHierarchy;

/**
 * FControlEvaluationGraphBinding
 */
struct TLLRN_CONTROLRIG_API FControlEvaluationGraphBinding
{
	void HandleControlModified(
		UTLLRN_ControlRig* InTLLRN_ControlRig,
		FTLLRN_RigControlElement* InControl,
		const FTLLRN_RigControlModifiedContext& InContext);
	bool bPendingFlush = false;
};

/**
 * UTLLRN_TransformableControlHandle
 */

UCLASS(Blueprintable)
class TLLRN_CONTROLRIG_API UTLLRN_TransformableControlHandle : public UTransformableHandle 
{
	GENERATED_BODY()
	
public:
	
	virtual ~UTLLRN_TransformableControlHandle();

	virtual void PostLoad() override;
	
	/** Sanity check to ensure that TLLRN_ControlRig and ControlName are safe to use. */
	virtual bool IsValid(const bool bDeepCheck = true) const override;

	/** Sets the global transform of the control. */
	virtual void SetGlobalTransform(const FTransform& InGlobal) const override;
	/** Sets the local transform of the control. */
	virtual void SetLocalTransform(const FTransform& InLocal) const override;
	/** Gets the global transform of the control. */
	virtual FTransform GetGlobalTransform() const  override;
	/** Sets the local transform of the control. */
	virtual FTransform GetLocalTransform() const  override;

	/** Returns the target object containing the tick function (e.i. SkeletalComponent bound to TLLRN_ControlRig). */
	virtual UObject* GetPrerequisiteObject() const override;
	/** Returns ths SkeletalComponent tick function. */
	virtual FTickFunction* GetTickFunction() const override;

	/** Generates a hash value based on TLLRN_ControlRig and ControlName. */
	static uint32 ComputeHash(const UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InControlName);
	virtual uint32 GetHash() const override;
	
	/** Returns the underlying targeted object. */
	virtual TWeakObjectPtr<UObject> GetTarget() const override;

	/** Get the array of float channels for the specified section*/
	virtual TArrayView<FMovieSceneFloatChannel*>  GetFloatChannels(const UMovieSceneSection* InSection) const override;
	/** Get the array of double channels for the specified section*/
	virtual TArrayView<FMovieSceneDoubleChannel*>  GetDoubleChannels(const UMovieSceneSection* InSection) const override;
	virtual bool AddTransformKeys(const TArray<FFrameNumber>& InFrames,
		const TArray<FTransform>& InTransforms,
		const EMovieSceneTransformChannel& InChannels,
		const FFrameRate& InTickResolution,
		UMovieSceneSection* InSection,
		const bool bLocal = true) const override;

	/** Resolve the bound objects so that any object it references are resolved and correctly set up*/
	virtual void ResolveBoundObjects(FMovieSceneSequenceID LocalSequenceID, TSharedRef<UE::MovieScene::FSharedPlaybackState> SharedPlaybackState, UObject* SubObject = nullptr) override;

	/** Make a duplicate of myself with this outer*/
	virtual UTransformableHandle* Duplicate(UObject* NewOuter) const override;

	/** Tick any skeletal mesh related to the bound component. */ 
	virtual void TickTarget() const override;

	/**
	 * Perform any pre-evaluation of the handle to ensure that the transform data is up to date.
	 * @param bTick to force any pre-evaluation ticking. The rig will still be pre-evaluated even
	 * if bTick is false (it just won't tick the bound skeletal meshes) Default is false.
	*/
	virtual void PreEvaluate(const bool bTick = false) const override;

	/** Registers/Unregisters useful delegates to track changes in the control's transform. */
	void UnregisterDelegates() const;
	void RegisterDelegates();

	/** Check for direct dependencies (ie hierarchy + skeletal mesh) with InOther. */
	virtual bool HasDirectDependencyWith(const UTransformableHandle& InOther) const override;

	/** Look for a possible tick function that can be used as a prerequisite. */
	virtual FTickPrerequisite GetPrimaryPrerequisite() const override;

#if WITH_EDITOR
	/** Returns labels used for UI. */
	virtual FString GetLabel() const override;
	virtual FString GetFullLabel() const override;
#endif

	/** The TLLRN_ControlRig that this handle is pointing at. */
	UPROPERTY(BlueprintReadOnly, Category = "Object")
	TSoftObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRig;

	/** The ControlName of the control that this handle is pointing at. */
	UPROPERTY(BlueprintReadOnly, Category = "Object")
	FName ControlName;

	/** @todo document */
	void OnControlModified(
		UTLLRN_ControlRig* InTLLRN_ControlRig,
		FTLLRN_RigControlElement* InControl,
		const FTLLRN_RigControlModifiedContext& InContext);
	
private:

	/** Returns the component bounded to TLLRN_ControlRig. */
	USceneComponent* GetBoundComponent() const;
	USkeletalMeshComponent* GetSkeletalMesh() const;
	UTLLRN_ControlRigComponent* GetTLLRN_ControlRigComponent() const;
	
	/** Handles notifications coming from the TLLRN_ControlRig's hierarchy */
	void OnHierarchyModified(
		ETLLRN_RigHierarchyNotification InNotif,
		UTLLRN_RigHierarchy* InHierarchy,
		const FTLLRN_RigBaseElement* InElement);

	void OnTLLRN_ControlRigBound(UTLLRN_ControlRig* InTLLRN_ControlRig);
	void OnObjectBoundToTLLRN_ControlRig(UObject* InObject);
	
	static FControlEvaluationGraphBinding& GetEvaluationBinding();

#if WITH_EDITOR
	/** @todo document */
	void OnObjectsReplaced(const TMap<UObject*, UObject*>& InOldToNewInstances);
#endif

	/** @todo document */
	FTLLRN_RigControlElement* GetControlElement() const;
};
