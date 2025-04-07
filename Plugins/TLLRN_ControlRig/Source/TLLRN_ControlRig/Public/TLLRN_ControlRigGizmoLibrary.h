// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"

#include "TLLRN_ControlRigGizmoLibrary.generated.h"

class UTLLRN_ControlRigShapeLibrary;

USTRUCT(BlueprintType, meta = (DisplayName = "Shape"))
struct TLLRN_CONTROLRIG_API FTLLRN_ControlRigShapeDefinition
{
	GENERATED_USTRUCT_BODY()

	FTLLRN_ControlRigShapeDefinition()
	{
		ShapeName = TEXT("Default");
		StaticMesh = nullptr;
		Transform = FTransform::Identity;
	}

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Shape")
	FName ShapeName;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Shape")
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Shape")
	FTransform Transform;

	mutable TWeakObjectPtr<UTLLRN_ControlRigShapeLibrary> Library;
};

UCLASS(BlueprintType, meta = (DisplayName = "TLLRN Control Rig Shape Library"))
class TLLRN_CONTROLRIG_API UTLLRN_ControlRigShapeLibrary : public UObject
{
	GENERATED_BODY()

public:

	UTLLRN_ControlRigShapeLibrary();

	UPROPERTY(EditAnywhere, Category = "ShapeLibrary")
	FTLLRN_ControlRigShapeDefinition DefaultShape;

	UPROPERTY(EditAnywhere, Category = "ShapeLibrary", meta = (DisplayName = "Override Material"))
	TSoftObjectPtr<UMaterial> DefaultMaterial;

	UPROPERTY(EditAnywhere, Category = "ShapeLibrary")
	TSoftObjectPtr<UMaterial> XRayMaterial;

	UPROPERTY(EditAnywhere, Category = "ShapeLibrary")
	FName MaterialColorParameter;

	UPROPERTY(EditAnywhere, Category = "ShapeLibrary")
	TArray<FTLLRN_ControlRigShapeDefinition> Shapes;

	const FTLLRN_ControlRigShapeDefinition* GetShapeByName(const FName& InName, bool bUseDefaultIfNotFound = false) const;
	static const FTLLRN_ControlRigShapeDefinition* GetShapeByName(const FName& InName, const TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>>& InShapeLibraries, const TMap<FString, FString>& InLibraryNameMap, bool bUseDefaultIfNotFound = true);
	static const FString GetShapeName(const UTLLRN_ControlRigShapeLibrary* InShapeLibrary, bool bUseNameSpace, const TMap<FString, FString>& InLibraryNameMap, const FTLLRN_ControlRigShapeDefinition& InShape);

#if WITH_EDITOR

	// UObject interface
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;

#endif

private:

	const TArray<FName> GetUpdatedNameList(bool bReset = false);

	TArray<FName> NameList;
};