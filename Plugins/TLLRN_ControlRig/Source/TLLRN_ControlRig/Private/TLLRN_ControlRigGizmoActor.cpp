// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigGizmoActor.h"
#include "TLLRN_ControlRig.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/CollisionProfile.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigGizmoActor)

ATLLRN_ControlRigShapeActor::ATLLRN_ControlRigShapeActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TLLRN_ControlRigIndex(INDEX_NONE)
	, TLLRN_ControlRig(nullptr)
	, ControlName(NAME_None)
	, ShapeName(NAME_None)
	, OverrideColor(0, 0, 0, 0)
	, OffsetTransform(FTransform::Identity)
	, bSelected(false)
	, bHovered(false)
	, bSelectable(true)
{

	ActorRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent0"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent0"));
	StaticMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StaticMeshComponent->Mobility = EComponentMobility::Movable;
	StaticMeshComponent->SetGenerateOverlapEvents(false);
	StaticMeshComponent->bUseDefaultCollision = false;
#if WITH_EDITORONLY_DATA
	StaticMeshComponent->HitProxyPriority = HPP_Wireframe;
#endif

#if WITH_EDITOR
	StaticMeshComponent->bAlwaysAllowTranslucentSelect = true;
#endif

	RootComponent = ActorRootComponent;
	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->bCastStaticShadow = false;
	StaticMeshComponent->bCastDynamicShadow = false;
	StaticMeshComponent->bSelectable = true;
}

//client should check to see if it's seletable based upon how the selection occurs (viewport or outliner,etc..)
void ATLLRN_ControlRigShapeActor::SetSelected(bool bInSelected)
{
	if(bSelected != bInSelected)
	{
		bSelected = bInSelected;
		FEditorScriptExecutionGuard Guard;
		OnSelectionChanged(bSelected);
	}
}

bool ATLLRN_ControlRigShapeActor::IsSelectedInEditor() const
{
	return bSelected;
}

bool ATLLRN_ControlRigShapeActor::IsSelectable() const
{
	return bSelectable;
}

//we no longer set the StaticMeshComponent bSelectable flag since that drives if it can move with a gizmo or not
void ATLLRN_ControlRigShapeActor::SetSelectable(bool bInSelectable)
{
	if (bSelectable != bInSelectable)
	{
		bSelectable = bInSelectable;
		if (!bSelectable)
		{
			SetSelected(false);
		}

		FEditorScriptExecutionGuard Guard;
		OnEnabledChanged(bInSelectable);
	}
}

void ATLLRN_ControlRigShapeActor::SetHovered(bool bInHovered)
{
	bool bOldHovered = bHovered;

	bHovered = bInHovered;

	if(bHovered != bOldHovered)
	{
		FEditorScriptExecutionGuard Guard;
		OnHoveredChanged(bHovered);
	}
}

bool ATLLRN_ControlRigShapeActor::IsHovered() const
{
	return bHovered;
}


void ATLLRN_ControlRigShapeActor::SetShapeColor(const FLinearColor& InColor)
{
	if (StaticMeshComponent && !ColorParameterName.IsNone())
	{
		if (UMaterialInstanceDynamic* MaterialInstance = Cast<UMaterialInstanceDynamic>(StaticMeshComponent->GetMaterial(0)))
		{
			MaterialInstance->SetVectorParameterValue(ColorParameterName, FVector(InColor));
		}
	}
}

bool ATLLRN_ControlRigShapeActor::UpdateControlSettings(
	ETLLRN_RigHierarchyNotification InNotif,
	UTLLRN_ControlRig* InTLLRN_ControlRig,
	const FTLLRN_RigControlElement* InControlElement,
	bool bHideManipulators,
	bool bIsInLevelEditor)
{
	check(InControlElement);

	const FTLLRN_RigControlSettings& ControlSettings = InControlElement->Settings;
	
	// if this actor is not supposed to exist
	if(!ControlSettings.SupportsShape())
	{
		return false;
	}

	const bool bShapeNameUpdated = ShapeName != ControlSettings.ShapeName;
	bool bShapeTransformChanged = InNotif == ETLLRN_RigHierarchyNotification::ControlShapeTransformChanged;
	const bool bLookupShape = bShapeNameUpdated || bShapeTransformChanged;
	
	FTransform MeshTransform = FTransform::Identity;

	// update the shape used for the control
	if(bLookupShape)
	{
		const TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>> ShapeLibraries = InTLLRN_ControlRig->GetShapeLibraries();
		if (const FTLLRN_ControlRigShapeDefinition* ShapeDef = UTLLRN_ControlRigShapeLibrary::GetShapeByName(ControlSettings.ShapeName, ShapeLibraries, InTLLRN_ControlRig->ShapeLibraryNameMap))
		{
			MeshTransform = ShapeDef->Transform;

			if(bShapeNameUpdated)
			{
				if (!ShapeDef->StaticMesh.IsValid())
				{
					ShapeDef->StaticMesh.LoadSynchronous();
				}

				if(ShapeDef->StaticMesh.IsValid())
				{
					if(UStaticMesh* StaticMesh = ShapeDef->StaticMesh.Get())
					{
						if(StaticMesh != StaticMeshComponent->GetStaticMesh())
						{
							StaticMeshComponent->SetStaticMesh(StaticMesh);
							bShapeTransformChanged = true;
							ShapeName = ControlSettings.ShapeName;
						}
					}
					else
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}
		}
	}

	// update the shape transform
	if(bShapeTransformChanged)
	{
		const FTransform ShapeTransform = InControlElement->GetShapeTransform().Get(ETLLRN_RigTransformType::CurrentLocal);
		StaticMeshComponent->SetRelativeTransform(MeshTransform * ShapeTransform);
	}
	
	// update the shape color
	SetShapeColor(ControlSettings.ShapeColor);

	return true;
}

// FTLLRN_ControlRigShapeHelper START

namespace FTLLRN_ControlRigShapeHelper
{
	FActorSpawnParameters GetDefaultSpawnParameter()
	{
		FActorSpawnParameters ActorSpawnParameters;
#if WITH_EDITOR
		ActorSpawnParameters.bTemporaryEditorActor = true;
		ActorSpawnParameters.bHideFromSceneOutliner = true;
#endif
		ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActorSpawnParameters.ObjectFlags = RF_Transient;
		return ActorSpawnParameters;
	}

	// create shape from custom staticmesh, may deprecate this unless we come up with better usage
	ATLLRN_ControlRigShapeActor* CreateShapeActor(UWorld* InWorld, UStaticMesh* InStaticMesh, const FTLLRN_ControlShapeActorCreationParam& CreationParam)
	{
		if (InWorld)
		{
			ATLLRN_ControlRigShapeActor* ShapeActor = CreateDefaultShapeActor(InWorld, CreationParam);

			if (ShapeActor)
			{
				if (InStaticMesh)
				{
					ShapeActor->StaticMeshComponent->SetStaticMesh(InStaticMesh);
				}

				return ShapeActor;
			}
		}

		return nullptr;
	}

	ATLLRN_ControlRigShapeActor* CreateShapeActor(UWorld* InWorld, TSubclassOf<ATLLRN_ControlRigShapeActor> InClass, const FTLLRN_ControlShapeActorCreationParam& CreationParam)
	{
		ATLLRN_ControlRigShapeActor* ShapeActor = InWorld->SpawnActor<ATLLRN_ControlRigShapeActor>(InClass, GetDefaultSpawnParameter());
		if (ShapeActor)
		{
			// set transform
			ShapeActor->SetActorTransform(CreationParam.SpawnTransform);
			return ShapeActor;
		}

		return nullptr;
	}

	ATLLRN_ControlRigShapeActor* CreateDefaultShapeActor(UWorld* InWorld, const FTLLRN_ControlShapeActorCreationParam& CreationParam)
	{
		ATLLRN_ControlRigShapeActor* ShapeActor = InWorld->SpawnActor<ATLLRN_ControlRigShapeActor>(ATLLRN_ControlRigShapeActor::StaticClass(), GetDefaultSpawnParameter());
		if (ShapeActor)
		{
			ShapeActor->TLLRN_ControlRigIndex = CreationParam.TLLRN_ControlRigIndex;
			ShapeActor->TLLRN_ControlRig = CreationParam.TLLRN_ControlRig;
			ShapeActor->ControlName = CreationParam.ControlName;
			ShapeActor->ShapeName = CreationParam.ShapeName;
			ShapeActor->SetSelectable(CreationParam.bSelectable);
			ShapeActor->SetActorTransform(CreationParam.SpawnTransform);
#if WITH_EDITOR
			ShapeActor->SetActorLabel(CreationParam.ControlName.ToString(), false);
#endif // WITH_EDITOR

			UStaticMeshComponent* MeshComponent = ShapeActor->StaticMeshComponent;

			if (!CreationParam.StaticMesh.IsValid())
			{
				CreationParam.StaticMesh.LoadSynchronous();
			}
			if (CreationParam.StaticMesh.IsValid())
			{
				MeshComponent->SetStaticMesh(CreationParam.StaticMesh.Get());
				MeshComponent->SetRelativeTransform(CreationParam.MeshTransform * CreationParam.ShapeTransform);
			}

			if (!CreationParam.Material.IsValid())
			{
				CreationParam.Material.LoadSynchronous();
			}
			if (CreationParam.StaticMesh.IsValid())
			{
				ShapeActor->ColorParameterName = CreationParam.ColorParameterName;
				UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(CreationParam.Material.Get(), ShapeActor);
				MaterialInstance->SetVectorParameterValue(CreationParam.ColorParameterName, FVector(CreationParam.Color));

				for (int32 i=0; i<MeshComponent->GetNumMaterials(); ++i)
				{
					MeshComponent->SetMaterial(i, MaterialInstance);
				}
			}
			return ShapeActor;
		}

		return nullptr;
	}
}

void ATLLRN_ControlRigShapeActor::SetGlobalTransform(const FTransform& InTransform)
{
	if (RootComponent)
	{
		RootComponent->SetRelativeTransform(InTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

FTransform ATLLRN_ControlRigShapeActor::GetGlobalTransform() const
{
	if (RootComponent)
	{
		return RootComponent->GetRelativeTransform();
	}

	return FTransform::Identity;
}

// FTLLRN_ControlRigShapeHelper END

