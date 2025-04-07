// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMesh.h"
#include "IViewportSelectableObject.h"
#include "Materials/Material.h"
#include "Rigs/TLLRN_TLLRN_RigControlHierarchy.h"
#include "TLLRN_ControlRigGizmoActor.generated.h"

class UTLLRN_ControlRig;
struct FActorSpawnParameters;

USTRUCT()
struct FTLLRN_ControlShapeActorCreationParam
{
	GENERATED_BODY()

	FTLLRN_ControlShapeActorCreationParam()
		: ManipObj(nullptr)
		, TLLRN_ControlRigIndex(INDEX_NONE)
		, TLLRN_ControlRig(nullptr)
		, ControlName(NAME_None)
		, ShapeName(NAME_None)
		, SpawnTransform(FTransform::Identity)
		, ShapeTransform(FTransform::Identity)
		, MeshTransform(FTransform::Identity)
		, StaticMesh(nullptr)
		, Material(nullptr)
		, ColorParameterName(NAME_None)
		, Color(FLinearColor::Red)
		, bSelectable(true)
	{
	}

	UObject*	ManipObj;
	int32		TLLRN_ControlRigIndex;
	UTLLRN_ControlRig* TLLRN_ControlRig;
	FName		ControlName;
	FName		ShapeName;
	FTransform	SpawnTransform;
	FTransform  ShapeTransform;
	FTransform  MeshTransform;
	TSoftObjectPtr<UStaticMesh> StaticMesh;
	TSoftObjectPtr<UMaterial> Material;
	FName ColorParameterName;
	FLinearColor Color;
	bool bSelectable;
};

/** An actor used to represent a rig control */
UCLASS(NotPlaceable, Transient)
class TLLRN_CONTROLRIG_API ATLLRN_ControlRigShapeActor : public AActor, public IViewportSelectableObject
{
	GENERATED_BODY()

public:
	ATLLRN_ControlRigShapeActor(const FObjectInitializer& ObjectInitializer);

	// this is the one holding transform for the controls
	UPROPERTY()
	TObjectPtr<class USceneComponent> ActorRootComponent;

	// this is visual representation of the transform
	UPROPERTY(Category = StaticMesh, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|StaticMesh", AllowPrivateAccess = "true"))
	TObjectPtr<class UStaticMeshComponent> StaticMeshComponent;

	// the name of the control this actor is referencing
	UPROPERTY()
	uint32 TLLRN_ControlRigIndex;

	//  control rig this actor is referencing we can have multiple control rig's visible
	UPROPERTY()
	TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRig;

	// the name of the control this actor is referencing
	UPROPERTY()
	FName ControlName;

	// the name of the shape to use on this actor
   	UPROPERTY()
   	FName ShapeName;

	// the name of the color parameter on the material
	UPROPERTY()
	FName ColorParameterName;

	FLinearColor OverrideColor;

	FTransform OffsetTransform;

	UFUNCTION(BlueprintSetter)
	/** Set the control to be enabled/disabled */
	virtual void SetEnabled(bool bInEnabled) { SetSelectable(bInEnabled); }

	UFUNCTION(BlueprintGetter)
	/** Get whether the control is enabled/disabled */
	virtual bool IsEnabled() const { return IsSelectable(); }

	UFUNCTION(BlueprintSetter)
	/** Set the control to be selected/unselected */
	virtual void SetSelected(bool bInSelected);

	/** Get whether the control is selected/unselected */
	UFUNCTION(BlueprintGetter)
	virtual bool IsSelectedInEditor() const;

	/** Get wether the control is selectable/unselectable */
	virtual bool IsSelectable() const;

	UFUNCTION(BlueprintSetter)
	/** Set the control to be selected/unselected */
	virtual void SetSelectable(bool bInSelectable);

	UFUNCTION(BlueprintSetter)
	/** Set the control to be hovered */
	virtual void SetHovered(bool bInHovered);

	UFUNCTION(BlueprintGetter)
	/** Get whether the control is hovered */
	virtual bool IsHovered() const;

	/* Returns the key of the control element this shape actor represents */
	FTLLRN_RigElementKey GetElementKey() const { return FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control); }

	/** Called from the edit mode each tick */
	virtual void TickControl() {};

	/** changes the shape color */
	virtual void SetShapeColor(const FLinearColor& InColor);

	/** Event called when the transform of this control has changed */
	UFUNCTION(BlueprintImplementableEvent)
	void OnTransformChanged(const FTransform& NewTransform);

	/** Event called when the enabled state of this control has changed */
	UFUNCTION(BlueprintImplementableEvent)
	void OnEnabledChanged(bool bIsEnabled);

	/** Event called when the selection state of this control has changed */
	UFUNCTION(BlueprintImplementableEvent)
	void OnSelectionChanged(bool bIsSelected);

	/** Event called when the hovered state of this control has changed */
	UFUNCTION(BlueprintImplementableEvent)
	void OnHoveredChanged(bool bIsSelected);

	/** Event called when the manipulating state of this control has changed */
	UFUNCTION(BlueprintImplementableEvent)
	void OnManipulatingChanged(bool bIsManipulating);

	// this returns root component transform based on attach
	// when there is no attach, it is based on 0
	UFUNCTION(BlueprintCallable, Category = "TLLRN_ControlRig|Shape")
	void SetGlobalTransform(const FTransform& InTransform);

	UFUNCTION(BlueprintPure, Category = "TLLRN_ControlRig|Shape")
	FTransform GetGlobalTransform() const;

	// try to update the actor with the latest settings
	bool UpdateControlSettings(
		ETLLRN_RigHierarchyNotification InNotif,
		UTLLRN_ControlRig* InTLLRN_ControlRig,
		const FTLLRN_RigControlElement* InControlElement,
		bool bHideManipulators,
		bool bIsInLevelEditor);
	
private:

	/** Whether this control is selected */
	UPROPERTY(BlueprintGetter = IsSelectedInEditor, BlueprintSetter = SetSelected, Category = "TLLRN_ControlRig|Shape")
	uint8 bSelected : 1;

	/** Whether this control is hovered */
	UPROPERTY(BlueprintGetter = IsHovered, BlueprintSetter = SetHovered, Category = "TLLRN_ControlRig|Shape")
	uint8 bHovered : 1;

	/** Whether or not this control is selectable*/
	uint8 bSelectable : 1;

};

/**
 * Creating Shape Param helper functions
 */
namespace FTLLRN_ControlRigShapeHelper
{
	extern TLLRN_CONTROLRIG_API ATLLRN_ControlRigShapeActor* CreateShapeActor(UWorld* InWorld, UStaticMesh* InStaticMesh, const FTLLRN_ControlShapeActorCreationParam& CreationParam);
	ATLLRN_ControlRigShapeActor* CreateShapeActor(UWorld* InWorld, TSubclassOf<ATLLRN_ControlRigShapeActor> InClass, const FTLLRN_ControlShapeActorCreationParam& CreationParam);
	extern TLLRN_CONTROLRIG_API ATLLRN_ControlRigShapeActor* CreateDefaultShapeActor(UWorld* InWorld, const FTLLRN_ControlShapeActorCreationParam& CreationParam);

	FActorSpawnParameters GetDefaultSpawnParameter();
}