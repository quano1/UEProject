// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "TargetInterfaces/TLLRN_DynamicMeshCommitter.h"
#include "TargetInterfaces/TLLRN_DynamicMeshProvider.h"
#include "TargetInterfaces/MaterialProvider.h"
#include "TargetInterfaces/MeshDescriptionCommitter.h"
#include "TargetInterfaces/MeshDescriptionProvider.h"
#include "TargetInterfaces/TLLRN_DynamicMeshSource.h"
#include "TargetInterfaces/PhysicsDataSource.h"
#include "ToolTargets/PrimitiveComponentToolTarget.h"

#include "TLLRN_DynamicMeshComponentToolTarget.generated.h"

class UDynamicMesh;

/**
 * A ToolTarget backed by a DynamicMeshComponent
 */
UCLASS(Transient)
class TLLRN_MODELINGCOMPONENTSEDITORONLY_API UTLLRN_DynamicMeshComponentToolTarget : 
	public UPrimitiveComponentToolTarget,
	public IMeshDescriptionCommitter, 
	public IMeshDescriptionProvider, 
	public ITLLRN_DynamicMeshProvider,
	public ITLLRN_DynamicMeshCommitter,
	public IMaterialProvider,
	public IPersistentTLLRN_DynamicMeshSource,
	public IPhysicsDataSource
{
	GENERATED_BODY()

public:
	virtual bool IsValid() const override;


public:
	// IMeshDescriptionProvider implementation
	const FMeshDescription* GetMeshDescription(const FGetMeshParameters& GetMeshParams = FGetMeshParameters()) override;
	virtual FMeshDescription GetEmptyMeshDescription() override;

	// IMeshDescritpionCommitter implementation
	virtual void CommitMeshDescription(const FCommitter& Committer, const FCommitMeshParameters& CommitMeshParams = FCommitMeshParameters()) override;
	using IMeshDescriptionCommitter::CommitMeshDescription; // unhide the other overload

	// ITLLRN_DynamicMeshProvider implementation
	virtual UE::Geometry::FDynamicMesh3 GetDynamicMesh() override;

	// ITLLRN_DynamicMeshCommitter implementation
	virtual void CommitDynamicMesh(const UE::Geometry::FDynamicMesh3& Mesh, const FDynamicMeshCommitInfo& CommitInfo) override;
	using ITLLRN_DynamicMeshCommitter::CommitDynamicMesh; // unhide the other overload

	// IMaterialProvider implementation
	int32 GetNumMaterials() const override;
	UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
	void GetMaterialSet(FComponentMaterialSet& MaterialSetOut, bool bPreferAssetMaterials) const override;
	virtual bool CommitMaterialSetUpdate(const FComponentMaterialSet& MaterialSet, bool bApplyToAsset) override;	

	// IPersistentTLLRN_DynamicMeshSource implementation
	virtual UDynamicMesh* GetDynamicMeshContainer() override;
	virtual void CommitDynamicMeshChange(TUniquePtr<FToolCommandChange> Change, const FText& ChangeMessage) override;
	virtual bool HasDynamicMeshComponent() const override;
	virtual UDynamicMeshComponent* GetDynamicMeshComponent() override;

	// IPhysicsDataSource implementation
	virtual UBodySetup* GetBodySetup() const override;
	virtual IInterface_CollisionDataProvider* GetComplexCollisionProvider() const override;

	// Rest provided by parent class

protected:
	TUniquePtr<FMeshDescription> ConvertedMeshDescription;
	bool bHaveMeshDescription = false;

protected:
	friend class UTLLRN_DynamicMeshComponentToolTargetFactory;
};


/** Factory for UTLLRN_DynamicMeshComponentToolTarget to be used by the target manager. */
UCLASS(Transient)
class TLLRN_MODELINGCOMPONENTSEDITORONLY_API UTLLRN_DynamicMeshComponentToolTargetFactory : public UToolTargetFactory
{
	GENERATED_BODY()

public:

	virtual bool CanBuildTarget(UObject* SourceObject, const FToolTargetTypeRequirements& TargetTypeInfo) const override;

	virtual UToolTarget* BuildTarget(UObject* SourceObject, const FToolTargetTypeRequirements& TargetTypeInfo) override;
};