// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigGizmoLibraryFactory.h"
#include "AssetTypeCategories.h"
#include "Engine/StaticMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigGizmoLibraryFactory)

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigGizmoLibraryFactory"

UTLLRN_ControlRigShapeLibraryFactory::UTLLRN_ControlRigShapeLibraryFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UTLLRN_ControlRigShapeLibrary::StaticClass();
}

UObject* UTLLRN_ControlRigShapeLibraryFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UTLLRN_ControlRigShapeLibrary* ShapeLibrary = NewObject<UTLLRN_ControlRigShapeLibrary>(InParent, Name, Flags);

	ShapeLibrary->DefaultShape.StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/TLLRN_ControlRig/Controls/TLLRN_ControlRig_Sphere_solid.TLLRN_ControlRig_Sphere_solid"));
	ShapeLibrary->DefaultMaterial = LoadObject<UMaterial>(nullptr, TEXT("/TLLRN_ControlRig/Controls/TLLRN_ControlRigGizmoMaterial.TLLRN_ControlRigGizmoMaterial"));
	ShapeLibrary->XRayMaterial = LoadObject<UMaterial>(nullptr, TEXT("/TLLRN_ControlRig/Controls/TLLRN_ControlRigXRayMaterial.TLLRN_ControlRigXRayMaterial"));
	ShapeLibrary->MaterialColorParameter = TEXT("Color");

	return ShapeLibrary;
}

FText UTLLRN_ControlRigShapeLibraryFactory::GetDisplayName() const
{
	return LOCTEXT("TLLRN_ControlRigShapeLibraryFactoryName", "TLLRN Control Rig Shape Library");
}

uint32 UTLLRN_ControlRigShapeLibraryFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Animation;
}

#undef LOCTEXT_NAMESPACE

