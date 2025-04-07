// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BoneContainer.h"
#include "TLLRN_IKRigLogger.h"
#include "UObject/Object.h"
#include "Interfaces/Interface_PreviewMeshProvider.h"
#include "TLLRN_IKRigSkeleton.h"
#if WITH_EDITOR
#include "SAdvancedTransformInputBox.h"
#endif

#include "TLLRN_IKRigDefinition.generated.h"

class IPropertyHandle;
class UTLLRN_IKRigSolver;

#if WITH_EDITORONLY_DATA
UENUM(BlueprintType)
enum class ETLLRN_IKRigGoalPreviewMode : uint8
{
	Additive		UMETA(DisplayName = "Additive"),
	Absolute		UMETA(DisplayName = "Absolute"),
};

namespace ETLLRN_IKRigTransformType
{
	enum Type : int8
	{
		Current,
		Reference,
	};
}
#endif

UCLASS(BlueprintType)
class TLLRN_IKRIG_API UTLLRN_IKRigEffectorGoal : public UObject
{
	GENERATED_BODY()

public:

	/** The name used to refer to this goal from outside systems.
	 * This is the name to use when referring to this Goal from Blueprint, Anim Graph, Control Rig or IK Retargeter.*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Goal Settings")
	FName GoalName;

	/** The name of the bone that this Goal is located at.*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Goal Settings")
	FName BoneName;

	/** Range 0-1, default is 1. Blend between the input bone position (0.0) and the current goal position (1.0).*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Goal Settings", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float PositionAlpha = 1.0f;
	
	/** Range 0-1, default is 1. Blend between the input bone rotation (0.0) and the current goal rotation (1.0).*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Goal Settings", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float RotationAlpha = 1.0f;

	/** The current transform of this Goal, in the Global Space of the character.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Goal Settings")
	FTransform CurrentTransform;

	/** The initial transform of this Goal, as defined by the initial transform of the Goal's bone in the retarget pose.*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Goal Settings")
	FTransform InitialTransform;

	bool operator==(const UTLLRN_IKRigEffectorGoal& Other) const { return GoalName == Other.GoalName; }

#if WITH_EDITORONLY_DATA

	/** Effects how this Goal transform is previewed in the IK Rig editor.
	* "Additive" interprets the Goal transform as being relative to the input pose. Useful for previewing animations.
	* "Absolute" pins the Goal transform to the Gizmo in the viewport.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Goal Settings")
	ETLLRN_IKRigGoalPreviewMode PreviewMode = ETLLRN_IKRigGoalPreviewMode::Additive;
	
	/**The size of the Goal gizmo drawing in the editor viewport.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Viewport Goal Settings", meta = (ClampMin = "0.1", ClampMax = "1000.0", UIMin = "0.1", UIMax = "100.0"))
	float SizeMultiplier = 1.0f;

	/**The thickness of the Goal gizmo drawing in the editor viewport.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Viewport Goal Settings",  meta = (ClampMin = "0.0", ClampMax = "10.0", UIMin = "0.0", UIMax = "5.0"))
	float ThicknessMultiplier = 0.7f;

	/** Should position data be exposed in Blueprint */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Exposure")
	bool bExposePosition = false;

	/** Should rotation data be exposed in Blueprint */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Exposure")
	bool bExposeRotation = false;
	
	virtual void PostLoad() override
	{
		Super::PostLoad();
		SetFlags(RF_Transactional);
	}
	
#endif

#if WITH_EDITOR

	TOptional<FTransform::FReal> GetNumericValue(
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent,
		ETLLRN_IKRigTransformType::Type TransformType
	) const;

	TTuple<FTransform, FTransform> PrepareNumericValueChanged(
		ESlateTransformComponent::Type Component,
		ESlateRotationRepresentation::Type Representation,
		ESlateTransformSubComponent::Type SubComponent,
		FTransform::FReal Value,
		ETLLRN_IKRigTransformType::Type TransformType
	) const;

	void SetTransform( const FTransform& InTransform, ETLLRN_IKRigTransformType::Type InTransformType);

	void OnCopyToClipboard(ESlateTransformComponent::Type Component, ETLLRN_IKRigTransformType::Type TransformType) const;
	void OnPasteFromClipboard(ESlateTransformComponent::Type Component, ETLLRN_IKRigTransformType::Type TransformType);

	bool TransformDiffersFromDefault(ESlateTransformComponent::Type Component, TSharedPtr<IPropertyHandle> PropertyHandle) const;
	void ResetTransformToDefault(ESlateTransformComponent::Type Component, TSharedPtr<IPropertyHandle> PropertyHandle);
	
#endif
};

USTRUCT(BlueprintType)
struct TLLRN_IKRIG_API FTLLRN_BoneChain
{
	GENERATED_BODY()

	FTLLRN_BoneChain() = default;
	
	FTLLRN_BoneChain(
		FName InName, 
		const FName InStartBone,
		const FName InEndBone,
		const FName InGoalName = NAME_None)
		: ChainName(InName)
		, StartBone(InStartBone)
		, EndBone(InEndBone)
		, IKGoalName(InGoalName)
	{}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = BoneChain)
	FName ChainName;

	UPROPERTY(VisibleAnywhere, Category = BoneChain)
	FBoneReference StartBone;

	UPROPERTY(VisibleAnywhere, Category = BoneChain)
	FBoneReference EndBone;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = BoneChain)
	FName IKGoalName;
};

USTRUCT(Blueprintable)
struct TLLRN_IKRIG_API FTLLRN_RetargetDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=RetargetRoot)
	FName RootBone;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Chains)
	TArray<FTLLRN_BoneChain> BoneChains;

	// add a bone chain from start bone to end bone and store it in this retarget definition
	void AddBoneChain(
		const FName ChainName,
		const FName StartBone,
		const FName EndBone,
		const FName GoalName = NAME_None);
	
	FTLLRN_BoneChain* GetEditableBoneChainByName(FName ChainName);
};

UCLASS(Blueprintable)
class TLLRN_IKRIG_API UTLLRN_IKRigDefinition : public UObject, public IInterface_PreviewMeshProvider
{
	GENERATED_BODY()

public:
	
	/** The skeletal mesh to run the IK solve on (loaded into viewport).
	* NOTE: you can assign ANY Skeletal Mesh to apply the IK Rig to. Compatibility is determined when a new mesh is assigned
	* by comparing it's hierarchy with the goals, solvers and bone settings required by the rig. See output log for details. */
	UPROPERTY(AssetRegistrySearchable, EditAnywhere, Category = PreviewMesh)
	TSoftObjectPtr<USkeletalMesh> PreviewSkeletalMesh;

	/** runtime, read-only access to skeleton data */
	const FTLLRN_IKRigSkeleton& GetSkeleton() const { return Skeleton; };
	
	/** runtime, read-only access to array of Goals, all modifications must go through UTLLRN_IKRigController */
	const TArray<UTLLRN_IKRigEffectorGoal*>& GetGoalArray() const { return Goals; };

	/** runtime, read-only access to array of Solvers, all modifications must go through UTLLRN_IKRigController */
	const TArray<UTLLRN_IKRigSolver*>& GetSolverArray() const { return Solvers; };

	/** runtime, read-only access to Retarget Definition, all modifications must go through UTLLRN_IKRigController */
	const TArray<FTLLRN_BoneChain>& GetRetargetChains() const { return RetargetDefinition.BoneChains; };

	/** runtime, read-only access to Retarget Root, all modifications must go through UTLLRN_IKRigController */
	const FName& GetRetargetRoot() const { return RetargetDefinition.RootBone; };

	/** runtime, read-only access to read a bone chain, all modifications must go through UTLLRN_IKRigController */
	const FTLLRN_BoneChain* GetRetargetChainByName(FName ChainName) const;

	/** UObject */
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	/** END UObject */
	
	/** IInterface_PreviewMeshProvider interface */
	virtual void SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty = true) override;
	virtual USkeletalMesh* GetPreviewMesh() const override;
	/** END IInterface_PreviewMeshProvider interface */

#if WITH_EDITOR
	/* Get name of Preview Mesh property */
	static const FName GetPreviewMeshPropertyName();
#endif

#if WITH_EDITORONLY_DATA
	/**The size of the Bones in the editor viewport. This is set by callbacks from the viewport Character>Bones menu*/
	UPROPERTY()
	float BoneSize = 2.0f;

	/**Draw bones in the viewport.*/
	UPROPERTY(EditAnywhere, Category = "Viewport Goal Settings")
	bool DrawGoals = true;
	
	/**The size of the Goals in the editor viewport.*/
	UPROPERTY(EditAnywhere, Category = "Viewport Goal Settings", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "100.0"))
	float GoalSize = 7.0f;

	/** The thickness of the Goals in the editor viewport.*/
	UPROPERTY(EditAnywhere, Category = "Viewport Goal Settings",  meta = (ClampMin = "0.01", UIMin = "0.0", UIMax = "10.0"))
	float GoalThickness = 0.7f;

	/** The controller responsible for managing this asset's data (all editor mutation goes through this) */
	UPROPERTY(Transient, DuplicateTransient, NonTransactional )
	TObjectPtr<UObject> Controller;
#endif

private:
	
	/** hierarchy and bone-pose transforms */
	UPROPERTY()
	FTLLRN_IKRigSkeleton Skeleton;

	/** goals, used as effectors by solvers that support them */
	UPROPERTY()
	TArray<TObjectPtr<UTLLRN_IKRigEffectorGoal>> Goals;
	
	/** polymorphic stack of solvers, executed in serial fashion where output of prior solve is input to the next */
	UPROPERTY(instanced)
	TArray<TObjectPtr<UTLLRN_IKRigSolver>> Solvers;

	/** bone chains for IK Retargeter */
	UPROPERTY()
	FTLLRN_RetargetDefinition RetargetDefinition;

	friend class UTLLRN_IKRigController;
};
