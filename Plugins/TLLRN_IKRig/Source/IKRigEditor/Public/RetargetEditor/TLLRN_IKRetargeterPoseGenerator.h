// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "UObject/WeakObjectPtr.h"

#include "TLLRN_IKRetargeterPoseGenerator.generated.h"

class UTLLRN_IKRetargeterController;
class FTLLRN_IKRetargetEditorController;
class UTLLRN_IKRetargetProcessor;
enum class ETLLRN_RetargetSourceOrTarget : uint8;

// A Note on Determining the "Direction" of a Bone...
//
// What are we using to determine the "direction" of the source and target bones to align?
// If chain only has one bone OR this bone is located at the end of a chain, then we cannot compute a tangent!
// So we fallback to an analysis of the geometry... this is necessary because we cannot rely solely on the shape of the skeleton.
// Any assumptions we make could be invalidated in different circumstances. For example,
// 1. we cannot assume the local axes are aligned in any meaningful way, and even if they are we cannot assume WHICH axis is the correct one
// 2. we cannot assume that any child bones are located in a position that would create a meaningful vector (ie twist bones in arbitrary locations)
// 3. we cannot assume that extrapolating the direction from the parent is meaningful. For example, feet are usually perpendicular to the knee.
//
// Since the skeleton cannot be relied upon to provide robust directional information for individual and/or leaf bones, we must instead fallback
// on looking at the mesh that is skinned to it and try to discern a direction vector from that. If there is no skinning data, then we simply
// do NOT auto-align that bone.
UENUM()
enum class ETLLRN_RetargetAutoAlignMethod : uint8
{
	// use the chain to determine the source and target directions to align
	ChainToChain,
	// use mesh direction as source, and mesh direction as the target to align
	MeshToMesh,
};

// a sub-system used by the TLLRN_IKRetargetEditorController for automatically generating a retarget pose
struct FTLLRN_RetargetAutoPoseGenerator : public FGCObject
{
	// created and owned by the retarget editor controller
	FTLLRN_RetargetAutoPoseGenerator(const TWeakObjectPtr<UTLLRN_IKRetargeterController> InController);

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FTLLRN_RetargetAutoPoseGenerator"); }
	// End FGCObject interface

	// align all bones bones on the specified skeleton to the OTHER skeleton
	void AlignAllBones(const ETLLRN_RetargetSourceOrTarget SourceOrTarget, const bool bSuppressWarnings) const;
	
	// align the specified bones on the current skeleton to the OTHER skeleton, using the specified method
	void AlignBones(
		const TArray<FName>& BonesToAlign,
		const ETLLRN_RetargetAutoAlignMethod Method,
		const ETLLRN_RetargetSourceOrTarget SourceOrTarget,
		const bool bSuppressWarnings) const;

	// adjust the vertical height of the retarget root to place the character back on the ground
	// if BoneToPutOnGround is not specified (NAME_None), then will find the lowest bone and use that
	void SnapToGround(const FName BoneToPutOnGround, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;

private:
	
	// generates a local rotation offset to align BoneToAlign to the opposite skeleton, and stores it in the current retarget pose
	void AlignBone(
		const FName BoneToAlign,
		const ETLLRN_RetargetAutoAlignMethod Method,
		const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;

	// generates a local rotation offset to align the retarget root based on the spine orientation
	void AlignRetargetRootBone(const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;

	// get the chain most likely to be the spine
	bool GetSpineChain(const ETLLRN_RetargetSourceOrTarget SourceOrTarget, FName& OutChainName) const;

	// get the chain most likely to be the spine
	FVector GetChainDirection(const FName ChainName, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	
	// takes a world space rotation offset, converts it to local space and stores it in the current retarget pose
	void ApplyWorldRotationToRetargetPose(
		const FQuat& WorldRotationOffset,
		const FName BoneToAffect,
		const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;

	// get the tangent vector of a retarget chain at a given param (0-1)
	bool GetChainTangent(
		const FName ChainName,
		const float Param,
		const ETLLRN_RetargetSourceOrTarget SourceOrTarget,
		FVector& OutDirection) const;

	// discern the direction of a bone based on the mesh it is skinned to
	bool GetChainTangentFromMesh(
		const FName ChainName,
		const float Param,
		const ETLLRN_RetargetSourceOrTarget SourceOrTarget,
		FVector& OutDirection) const;

	// get the locations of vertices skinned to a sub-set of bones
	void GetPointsWeightedToBones(
		const TArray<FName>& Bones,
		const ETLLRN_RetargetSourceOrTarget SourceOrTarget,
		TArray<FVector>& OutVertexPositions) const;

	// get all children (recursive) of the given bone
	void GetAllChildrenOfBone(
		const FName Bone,
		const ETLLRN_RetargetSourceOrTarget SourceOrTarget,
		TArray<FName>& OutAllChildren) const;

	// get the vertex ref pose and skinning weights for a given mesh (source or target)
	void GetRefPoseVerticesAndWeights(
		const ETLLRN_RetargetSourceOrTarget SourceOrTarget,
		TArray<TMap<FName, float>>& OutWeights,
		TArray<FVector>& OutPositions) const;

	// calculates the skinned vertex positions for a subset of the mesh using the supplied bone transforms
	void GetDeformedVertexPositions(
		const TArray<int32>& VerticesToDeform,
		const TArray<FVector>& VertexRefPose,
		const TArray<FTransform>& RefPoseBoneTransforms,
		const TArray<FTransform>& CurrentBoneTransforms,
		const TArray<TMap<FName, float>>& SkinWeights,
		const ETLLRN_RetargetSourceOrTarget SourceOrTarget,
		TArray<FVector>& OutDeformedPositions) const;

	// gets the bone at this point in the chain, returns true if non of it's children are retargeted 
	bool IsARetargetLeaf(
		const FName ChainName,
		const float Param,
		ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;

	// get the processor from the editor and return true if it's initialized
	bool CheckReadyToAlignBones() const;

	TWeakObjectPtr<UTLLRN_IKRetargeterController> Controller;
	TObjectPtr<UTLLRN_IKRetargetProcessor> Processor;
	static constexpr float ParamStepSize = 0.1f;
};
