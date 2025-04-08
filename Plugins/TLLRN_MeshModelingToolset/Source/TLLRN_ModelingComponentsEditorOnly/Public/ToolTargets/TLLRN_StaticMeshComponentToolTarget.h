// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "TargetInterfaces/TLLRN_DynamicMeshCommitter.h"
#include "TargetInterfaces/TLLRN_DynamicMeshProvider.h"
#include "TargetInterfaces/MaterialProvider.h"
#include "TargetInterfaces/MeshDescriptionCommitter.h"
#include "TargetInterfaces/MeshDescriptionProvider.h"
#include "TargetInterfaces/StaticMeshBackedTarget.h"
#include "TargetInterfaces/PhysicsDataSource.h"
#include "ToolTargets/PrimitiveComponentToolTarget.h"
#include "ToolTargets/TLLRN_StaticMeshToolTarget.h" // for FMeshDescriptionCache
#include "ComponentSourceInterfaces.h"  // for EMeshLODIdentifier

#include "TLLRN_StaticMeshComponentToolTarget.generated.h"

class UStaticMesh;

/**
 * A tool target backed by a static mesh component that can provide and take a mesh
 * description.
 */
UCLASS(Transient)
class TLLRN_MODELINGCOMPONENTSEDITORONLY_API UTLLRN_StaticMeshComponentToolTarget : public UPrimitiveComponentToolTarget,
	public IMeshDescriptionCommitter, public IMeshDescriptionProvider, public IMaterialProvider, public IStaticMeshBackedTarget,
	public ITLLRN_DynamicMeshProvider, public ITLLRN_DynamicMeshCommitter, public IPhysicsDataSource
{
	GENERATED_BODY()

public:
	/**
	 * Configure active LOD to edit. Can only call this after Component is configured in base UPrimitiveComponentToolTarget.
	 * If requested LOD does not exist, fall back to one that does.
	 */
	virtual void SetEditingLOD(EMeshLODIdentifier RequestedEditingLOD);

	/** @return current editing LOD */
	virtual EMeshLODIdentifier GetEditingLOD() const { return EditingLOD; }

public:
	virtual bool IsValid() const override;


public:
	// IMeshDescriptionProvider implementation
	virtual const FMeshDescription* GetMeshDescription(const FGetMeshParameters& GetMeshParams = FGetMeshParameters()) override;
	virtual FMeshDescription GetMeshDescriptionCopy(const FGetMeshParameters& GetMeshParams) override;
	virtual FMeshDescription GetEmptyMeshDescription() override;
	virtual bool SupportsLODs() const override { return true; }
	virtual TArray<EMeshLODIdentifier> GetAvailableLODs(bool bSkipAutoGenerated = true) const override;
	virtual EMeshLODIdentifier GetMeshDescriptionLOD() const override { return EditingLOD; }
	virtual TArray<int32> GetPolygonGroupToMaterialIndexMap() const override;

	// IMeshDescriptionCommitter implementation
	virtual void CommitMeshDescription(const FCommitter& Committer, const FCommitMeshParameters& CommitParams = FCommitMeshParameters()) override;
	using IMeshDescriptionCommitter::CommitMeshDescription; // unhide the other overload

	// ITLLRN_DynamicMeshProvider
	virtual UE::Geometry::FDynamicMesh3 GetDynamicMesh() override;
	virtual UE::Geometry::FDynamicMesh3 GetDynamicMesh(const FGetMeshParameters& InGetMeshParams) override;

	// ITLLRN_DynamicMeshCommitter
	virtual void CommitDynamicMesh(const UE::Geometry::FDynamicMesh3& Mesh, const FDynamicMeshCommitInfo& CommitInfo) override;
	using ITLLRN_DynamicMeshCommitter::CommitDynamicMesh; // unhide the other overload

	// IMaterialProvider implementation
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
	virtual void GetMaterialSet(FComponentMaterialSet& MaterialSetOut, bool bPreferAssetMaterials) const override;
	virtual bool CommitMaterialSetUpdate(const FComponentMaterialSet& MaterialSet, bool bApplyToAsset) override;	

	// IStaticMeshBackedTarget
	virtual UStaticMesh* GetStaticMesh() const override;

	// IPhysicsDataSource
	virtual UBodySetup* GetBodySetup() const override;
	virtual IInterface_CollisionDataProvider* GetComplexCollisionProvider() const override;

	// Rest provided by parent class

public:

protected:
	EMeshLODIdentifier EditingLOD = EMeshLODIdentifier::LOD0;

	friend class UTLLRN_StaticMeshComponentToolTargetFactory;

private:
	UTLLRN_StaticMeshToolTarget::FMeshDescriptionCache CachedMeshDescriptions;
};


/** Factory for UTLLRN_StaticMeshComponentToolTarget to be used by the target manager. */
UCLASS(Transient)
class TLLRN_MODELINGCOMPONENTSEDITORONLY_API UTLLRN_StaticMeshComponentToolTargetFactory : public UToolTargetFactory
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTarget(UObject* SourceObject, const FToolTargetTypeRequirements& TargetTypeInfo) const override;

	virtual UToolTarget* BuildTarget(UObject* SourceObject, const FToolTargetTypeRequirements& TargetTypeInfo) override;

public:
	virtual EMeshLODIdentifier GetActiveEditingLOD() const { return EditingLOD; }
	virtual void SetActiveEditingLOD(EMeshLODIdentifier NewEditingLOD);

protected:
	// LOD to edit, default is to edit LOD0
	EMeshLODIdentifier EditingLOD = EMeshLODIdentifier::LOD0;
};