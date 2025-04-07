// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_IKRigDataTypes.h"
#include "TLLRN_IKRigLogger.h"
#include "TLLRN_IKRigSkeleton.h"

#include "TLLRN_IKRigProcessor.generated.h"

class UTLLRN_IKRigDefinition;
class UTLLRN_IKRigSolver;

USTRUCT()
struct FTLLRN_GoalBone
{
	GENERATED_BODY()
	
	FName BoneName;
	int32 BoneIndex;
	int32 OptSourceIndex = INDEX_NONE;
};

UCLASS()
class TLLRN_IKRIG_API UTLLRN_IKRigProcessor : public UObject
{
	GENERATED_BODY()
	
public:
	
	/** the runtime for an IKRig to convert an input pose into
	*   a solved output pose given a set of IK Rig Goals:
	*   
	* 1. Create a new TLLRN_IKRigProcessor once using MakeNewTLLRN_IKRigProcessor()
	* 2. Initialize() with an TLLRN_IKRigDefinition asset
	* 3. each tick, call SetIKGoal() and SetInputPoseGlobal()
	* 4. Call Solve()
	* 5. Copy output transforms with CopyOutputGlobalPoseToArray()
	* 
	*/

	UTLLRN_IKRigProcessor();
	
	/** setup a new processor to run the given IKRig asset
	 *  NOTE!! this function creates new UObjects and consequently MUST be called from the main thread!!
	 *  @param InRigAsset - the IK Rig defining the collection of solvers to execute and all the rig settings
	 *  @param SkeletalMesh - the skeletal mesh you want to solve the IK on
	 *  @param ExcludedGoals - a list of goal names to exclude from this processor instance (support per-instance removal of goals)
	 */
	void Initialize(
		const UTLLRN_IKRigDefinition* InRigAsset,
		const USkeletalMesh* SkeletalMesh,
		const TArray<FName>& ExcludedGoals = TArray<FName>());

	//
	// BEGIN UPDATE SEQUENCE FUNCTIONS
	//
	// This is the general sequence of function calls to run a typical IK solve:
	//
	
	/** Set all bone transforms in global space. This is the pose the IK solve will start from */
	void SetInputPoseGlobal(const TArray<FTransform>& InGlobalBoneTransforms);

	/** Optionally can be called before Solve() to use the reference pose as start pose */
	void SetInputPoseToRefPose();

	/** Set a named IK goal to go to a specific location, rotation and space, blended by separate position/rotation alpha (0-1)*/
	void SetIKGoal(const FTLLRN_IKRigGoal& Goal);

	/** Set a named IK goal to go to a specific location, rotation and space, blended by separate position/rotation alpha (0-1)*/
	void SetIKGoal(const UTLLRN_IKRigEffectorGoal* Goal);

	/** Run entire stack of solvers.
	 * If any Goals were supplied in World Space, a valid WorldToComponent transform must be provided.  */
	void Solve(const FTransform& WorldToComponent = FTransform::Identity);

	/** Get the results after calling Solve() */
	void CopyOutputGlobalPoseToArray(TArray<FTransform>& OutputPoseGlobal) const;

	/** Reset all internal data structures. Will require re-initialization before solving again. */
	void Reset();

	//
	// END UPDATE SEQUENCE FUNCTIONS
	//

	/** Get access to the internal goal data (read only) */
	const FTLLRN_IKRigGoalContainer& GetGoalContainer() const;
	/** Get the bone associated with a goal */
	const FTLLRN_GoalBone* GetGoalBone(const FName& GoalName) const;
	
	/** Get read/write access to the internal skeleton data */
	FTLLRN_IKRigSkeleton& GetSkeletonWriteable();
	/** Get read-only access to the internal skeleton data */
	const FTLLRN_IKRigSkeleton& GetSkeleton() const;

	/** Used to determine if the IK Rig asset is compatible with a given skeleton. */
	static bool IsIKRigCompatibleWithSkeleton(
		const UTLLRN_IKRigDefinition* InRigAsset,
		const FTLLRN_IKRigInputSkeleton& InputSkeleton,
		const FTLLRN_IKRigLogger* Log);

	bool IsInitialized() const { return bInitialized; };

	void SetNeedsInitialized();

	/** logging system */
	FTLLRN_IKRigLogger Log;

	/** Used to propagate setting values from the source asset at runtime (settings that do not require re-initialization) */
	void CopyAllInputsFromSourceAssetAtRuntime(const UTLLRN_IKRigDefinition* SourceAsset);

	/** Get read-only access to currently running solvers */
	const TArray<TObjectPtr<UTLLRN_IKRigSolver>>& GetSolvers(){ return Solvers; };
	
private:

	/** Update the final pos/rot of all the goals based on their alpha values and their space settings. */
	void ResolveFinalGoalTransforms(const FTransform& WorldToComponent);

	/** the stack of solvers to run in order */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTLLRN_IKRigSolver>> Solvers;

	/** the named transforms that solvers use as end effectors */
	FTLLRN_IKRigGoalContainer GoalContainer;

	/** map of goal names to bone names/indices */
	TMap<FName, FTLLRN_GoalBone> GoalBones;

	/** storage for hierarchy and bone transforms */
	FTLLRN_IKRigSkeleton Skeleton;

	/** solving disabled until this flag is true */
	bool bInitialized = false;
	bool bTriedToInitialize = false;
};