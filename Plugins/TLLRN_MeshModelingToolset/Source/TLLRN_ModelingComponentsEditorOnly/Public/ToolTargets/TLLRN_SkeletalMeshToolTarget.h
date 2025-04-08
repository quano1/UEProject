// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "TargetInterfaces/TLLRN_DynamicMeshCommitter.h"
#include "TargetInterfaces/TLLRN_DynamicMeshProvider.h"
#include "TargetInterfaces/MaterialProvider.h"
#include "TargetInterfaces/MeshDescriptionCommitter.h"
#include "TargetInterfaces/MeshDescriptionProvider.h"
#include "TargetInterfaces/SkeletalMeshBackedTarget.h"
#include "ToolTargets/PrimitiveComponentToolTarget.h"

#include "TLLRN_SkeletalMeshToolTarget.generated.h"


class USkeletalMesh;

/**
 * A tool target backed by a read-only skeletal mesh.
 * 
 * This is a special tool target that refers to the underlying asset (in this case a skeletal mesh), rather than indirectly through a component.
 * This type of target is used in cases, such as opening an asset through the content browser, when there is no component available.
 */
UCLASS(Transient)
class TLLRN_MODELINGCOMPONENTSEDITORONLY_API UTLLRN_SkeletalMeshReadOnlyToolTarget :
	public UToolTarget,
	public IMeshDescriptionProvider,
	public ITLLRN_DynamicMeshProvider,
	public IMaterialProvider,
	public ISkeletalMeshBackedTarget
{
	GENERATED_BODY()

public:
	// UToolTarget
	virtual bool IsValid() const override;

	// IMeshDescriptionProvider implementation
	virtual const FMeshDescription* GetMeshDescription(const FGetMeshParameters& GetMeshParams = FGetMeshParameters()) override;
	virtual FMeshDescription GetEmptyMeshDescription() override;
	virtual bool SupportsLODs() const override { return true; }
	virtual TArray<EMeshLODIdentifier> GetAvailableLODs(bool bSkipAutoGenerated = true) const override;
	virtual EMeshLODIdentifier GetMeshDescriptionLOD() const override { return static_cast<EMeshLODIdentifier>(DefaultEditingLOD); }

	// IMaterialProvider implementation
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
	virtual void GetMaterialSet(FComponentMaterialSet& MaterialSetOut, bool bPreferAssetMaterials) const override;
	virtual bool CommitMaterialSetUpdate(const FComponentMaterialSet& MaterialSet, bool bApplyToAsset) override;	

	// ITLLRN_DynamicMeshProvider
	virtual UE::Geometry::FDynamicMesh3 GetDynamicMesh() override;
	virtual UE::Geometry::FDynamicMesh3 GetDynamicMesh(const FGetMeshParameters& InGetMeshParams) override;

	// ISkeletalMeshBackedTarget implementation
	virtual USkeletalMesh* GetSkeletalMesh() const override;

protected:
	TWeakObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;

	// The LOD to edit if requested to get/commit at EMeshLODIdentifier::Default or if no specific LOD is requested.
	int32 DefaultEditingLOD = 0;	
	
	// So that the tool target factory can poke into Component.
	friend class UTLLRN_SkeletalMeshReadOnlyToolTargetFactory;
	friend class UTLLRN_SkeletalMeshComponentReadOnlyToolTarget;
	friend class UTLLRN_SkeletalMeshComponentToolTarget;

	UE_DEPRECATED(5.3, "This function doesn't check anything skeletal mesh specific, only (badly) checks to see if the object is alive")
	static bool IsValid(const USkeletalMesh* USkeletalMesh);

	static TArray<EMeshLODIdentifier> GetAvailableLODs(const USkeletalMesh* SkeletalMesh, bool bSkipAutoGenerated);	
	static void GetMaterialSet(const USkeletalMesh* SkeletalMesh, FComponentMaterialSet& MaterialSetOut,
		bool bPreferAssetMaterials);
	static bool CommitMaterialSetUpdate(USkeletalMesh* SkeletalMesh,
		const FComponentMaterialSet& MaterialSet, bool bApplyToAsset);
	static int32 GetValidEditingLOD(const USkeletalMesh* SkeletalMesh, int32 DefaultEditingLOD, bool bHaveRequestLOD, EMeshLODIdentifier RequestedEditingLOD);	
};


/**
 * A tool target backed by a skeletal mesh.
 */
UCLASS(Transient)
class TLLRN_MODELINGCOMPONENTSEDITORONLY_API UTLLRN_SkeletalMeshToolTarget :
	public UTLLRN_SkeletalMeshReadOnlyToolTarget,
	public IMeshDescriptionCommitter,
	public ITLLRN_DynamicMeshCommitter
{
	GENERATED_BODY()

public:
	// IMeshDescriptionCommitter implementation
	virtual void CommitMeshDescription(const FCommitter& Committer, const FCommitMeshParameters& CommitMeshParams = FCommitMeshParameters()) override;
	using IMeshDescriptionCommitter::CommitMeshDescription; // unhide the other overload

	// ITLLRN_DynamicMeshCommitter
	virtual void CommitDynamicMesh(const UE::Geometry::FDynamicMesh3& Mesh, 
		const FDynamicMeshCommitInfo& CommitInfo) override;
	using ITLLRN_DynamicMeshCommitter::CommitDynamicMesh; // unhide the other overload

protected:
	// So that the tool target factory can poke into Component.
	friend class UTLLRN_SkeletalMeshToolTargetFactory;
	friend class UTLLRN_SkeletalMeshComponentToolTarget;

	static void CommitMeshDescription(USkeletalMesh* SkeletalMesh, const FCommitter& Committer, int32 LODIndex);
};


/** Factory for UTLLRN_SkeletalMeshReadOnlyToolTarget to be used by the target manager. */
UCLASS(Transient)
class TLLRN_MODELINGCOMPONENTSEDITORONLY_API UTLLRN_SkeletalMeshReadOnlyToolTargetFactory : public UToolTargetFactory
{
	GENERATED_BODY()

public:

	virtual bool CanBuildTarget(UObject* SourceObject, const FToolTargetTypeRequirements& TargetTypeInfo) const override;

	virtual UToolTarget* BuildTarget(UObject* SourceObject, const FToolTargetTypeRequirements& TargetTypeInfo) override;
};


/** Factory for UTLLRN_SkeletalMeshToolTarget to be used by the target manager. */
UCLASS(Transient)
class TLLRN_MODELINGCOMPONENTSEDITORONLY_API UTLLRN_SkeletalMeshToolTargetFactory : public UToolTargetFactory
{
	GENERATED_BODY()

public:

	virtual bool CanBuildTarget(UObject* SourceObject, const FToolTargetTypeRequirements& TargetTypeInfo) const override;

	virtual UToolTarget* BuildTarget(UObject* SourceObject, const FToolTargetTypeRequirements& TargetTypeInfo) override;
};
