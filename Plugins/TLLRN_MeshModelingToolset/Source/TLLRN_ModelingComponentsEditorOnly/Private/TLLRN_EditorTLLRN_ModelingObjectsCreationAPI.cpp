// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_EditorTLLRN_ModelingObjectsCreationAPI.h"
#include "InteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "ContextObjectStore.h"

#include "AssetUtils/CreateStaticMeshUtil.h"
#include "AssetUtils/CreateTexture2DUtil.h"
#include "AssetUtils/CreateMaterialUtil.h"
#include "Physics/ComponentCollisionUtil.h"

#include "ConversionUtils/DynamicMeshToVolume.h"
#include "MeshDescriptionToDynamicMesh.h"

#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"

#include "ToolTargets/TLLRN_VolumeComponentToolTarget.h"  // for CVarModelingMaxVolumeTriangleCount
#include "Engine/BlockingVolume.h"
#include "Components/BrushComponent.h"
#include "Engine/Polys.h"
#include "Model.h"
#include "BSPOps.h"		// in UnrealEd
#include "Editor/EditorEngine.h"		// for FActorLabelUtilities

#include "DynamicMeshActor.h"
#include "Components/DynamicMeshComponent.h"

#include "ActorFactories/ActorFactory.h"
#include "AssetSelection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_EditorTLLRN_ModelingObjectsCreationAPI)

using namespace UE::Geometry;

extern UNREALED_API UEditorEngine* GEditor;

UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI* UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::Register(UInteractiveToolsContext* ToolsContext)
{
	if (ensure(ToolsContext))
	{
		UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI* CreationAPI = ToolsContext->ContextObjectStore->FindContext<UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI>();
		if (CreationAPI)
		{
			return CreationAPI;
		}
		CreationAPI = NewObject<UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI>(ToolsContext);
		ToolsContext->ContextObjectStore->AddContextObject(CreationAPI);
		if (ensure(CreationAPI))
		{
			return CreationAPI;
		}
	}
	return nullptr;
}

UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI* UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::Find(UInteractiveToolsContext* ToolsContext)
{
	if (ensure(ToolsContext))
	{
		UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI* CreationAPI = ToolsContext->ContextObjectStore->FindContext<UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI>();
		if (CreationAPI != nullptr)
		{
			return CreationAPI;
		}
	}
	return nullptr;
}

bool UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::Deregister(UInteractiveToolsContext* ToolsContext)
{
	if (ensure(ToolsContext))
	{
		UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI* CreationAPI = ToolsContext->ContextObjectStore->FindContext<UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI>();
		if (CreationAPI != nullptr)
		{
			ToolsContext->ContextObjectStore->RemoveContextObject(CreationAPI);
		}
		return true;
	}
	return false;
}


FTLLRN_CreateMeshObjectResult UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::CreateMeshObject(const FTLLRN_CreateMeshObjectParams& CreateMeshParams)
{
	// TODO: implement this path
	check(false);
	return FTLLRN_CreateMeshObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_InvalidMesh };
}


FTLLRN_CreateTextureObjectResult UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::CreateTextureObject(const FTLLRN_CreateTextureObjectParams& CreateTexParams)
{
	FTLLRN_CreateTextureObjectParams LocalParams = CreateTexParams;
	return CreateTextureObject(MoveTemp(LocalParams));
}

FTLLRN_CreateMaterialObjectResult UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::CreateMaterialObject(const FTLLRN_CreateMaterialObjectParams& CreateMaterialParams)
{
	FTLLRN_CreateMaterialObjectParams LocalParams = CreateMaterialParams;
	return CreateMaterialObject(MoveTemp(LocalParams));
}

FTLLRN_CreateActorResult UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::CreateNewActor(const FTLLRN_CreateActorParams& TLLRN_CreateActorParams)
{
	FTLLRN_CreateActorParams LocalParams = TLLRN_CreateActorParams;
	return CreateNewActor(MoveTemp(LocalParams));
}



FTLLRN_CreateMeshObjectResult UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::CreateMeshObject(FTLLRN_CreateMeshObjectParams&& CreateMeshParams)
{
	FTLLRN_CreateMeshObjectResult ResultOut;
	if (CreateMeshParams.TypeHint == ETLLRN_CreateObjectTypeHint::Volume)
	{
		ResultOut = CreateVolume(MoveTemp(CreateMeshParams));
	}
	else if (CreateMeshParams.TypeHint == ETLLRN_CreateObjectTypeHint::DynamicMeshActor)
	{
		ResultOut = CreateDynamicMeshActor(MoveTemp(CreateMeshParams));
	}
	else
	{
		ResultOut = CreateStaticMeshAsset(MoveTemp(CreateMeshParams));
	}

	if (ResultOut.IsOK())
	{
		OnModelingMeshCreated.Broadcast(ResultOut);
	}

	return ResultOut;
}


TArray<UMaterialInterface*> UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::FilterMaterials(const TArray<UMaterialInterface*>& MaterialsIn)
{
	TArray<UMaterialInterface*> OutputMaterials = MaterialsIn;
	for (int32 k = 0; k < OutputMaterials.Num(); ++k)
	{
		FString AssetPath = OutputMaterials[k]->GetPathName();
		if (AssetPath.StartsWith(TEXT("/TLLRN_MeshModelingToolsetExp/")))
		{
			OutputMaterials[k] = UMaterial::GetDefaultMaterial(MD_Surface);
		}
	}
	return OutputMaterials;
}


FTLLRN_CreateMeshObjectResult UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::CreateVolume(FTLLRN_CreateMeshObjectParams&& CreateMeshParams)
{
	// determine volume actor type
	UClass* VolumeClass = ABlockingVolume::StaticClass();
	if (CreateMeshParams.TypeHintClass
		&& Cast<AVolume>(CreateMeshParams.TypeHintClass.Get()->GetDefaultObject(false)) != nullptr)
	{
		VolumeClass = CreateMeshParams.TypeHintClass;
	}

	// create new volume actor using factory
	AVolume* const NewVolumeActor = [VolumeClass, &CreateMeshParams]() -> AVolume*
	{
		if (UActorFactory* const VolumeFactory = FActorFactoryAssetProxy::GetFactoryForAssetObject(VolumeClass))
		{
			AActor* const Actor = VolumeFactory->CreateActor(VolumeClass, CreateMeshParams.TargetWorld->GetCurrentLevel(), FTransform::Identity);
			FActorLabelUtilities::SetActorLabelUnique(Actor, CreateMeshParams.BaseName);
			return Cast<AVolume>(Actor);
		}
		return nullptr;
	}();
	if (!NewVolumeActor)
	{
		return FTLLRN_CreateMeshObjectResult{ETLLRN_CreateModelingObjectResult::Failed_ActorCreationFailed};
	}

	NewVolumeActor->BrushType = EBrushType::Brush_Add;
	UModel* Model = NewObject<UModel>(NewVolumeActor);
	NewVolumeActor->Brush = Model;
	NewVolumeActor->GetBrushComponent()->Brush = NewVolumeActor->Brush;

	UE::Conversion::FMeshToVolumeOptions Options;
	Options.bAutoSimplify = true;
	Options.MaxTriangles = FMath::Max(1, CVarModelingMaxVolumeTriangleCount.GetValueOnGameThread());
	if (CreateMeshParams.MeshType == ETLLRN_CreateMeshObjectSourceMeshType::DynamicMesh)
	{
		UE::Conversion::DynamicMeshToVolume(CreateMeshParams.DynamicMesh.GetValue(), NewVolumeActor, Options);
	}
	else if (CreateMeshParams.MeshType == ETLLRN_CreateMeshObjectSourceMeshType::MeshDescription)
	{
		FMeshDescriptionToDynamicMesh Converter;
		FMeshDescription* MeshDescription = &CreateMeshParams.MeshDescription.GetValue();
		FDynamicMesh3 ConvertedMesh;
		Converter.Convert(MeshDescription, ConvertedMesh);
		UE::Conversion::DynamicMeshToVolume(ConvertedMesh, NewVolumeActor, Options);
	}
	else
	{
		return FTLLRN_CreateMeshObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_InvalidMesh };
	}

	NewVolumeActor->SetActorTransform(CreateMeshParams.Transform);
	FActorLabelUtilities::SetActorLabelUnique(NewVolumeActor, CreateMeshParams.BaseName);
	NewVolumeActor->PostEditChange();
	
	// emit result
	FTLLRN_CreateMeshObjectResult ResultOut;
	ResultOut.ResultCode = ETLLRN_CreateModelingObjectResult::Ok;
	ResultOut.NewActor = NewVolumeActor;
	ResultOut.NewComponent = NewVolumeActor->GetBrushComponent();
	ResultOut.NewAsset = nullptr;
	return ResultOut;
}



FTLLRN_CreateMeshObjectResult UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::CreateDynamicMeshActor(FTLLRN_CreateMeshObjectParams&& CreateMeshParams)
{
	// spawn new actor
	FActorSpawnParameters SpawnInfo;
	ADynamicMeshActor* NewActor = CreateMeshParams.TargetWorld->SpawnActor<ADynamicMeshActor>(SpawnInfo);

	UDynamicMeshComponent* NewComponent = NewActor->GetDynamicMeshComponent();

	// assume that DynamicMeshComponent always has tangents on it's internal UDynamicMesh
	NewComponent->SetTangentsType(EDynamicMeshComponentTangentsMode::ExternallyProvided);

	if (CreateMeshParams.MeshType == ETLLRN_CreateMeshObjectSourceMeshType::DynamicMesh)
	{
		FDynamicMesh3 SetMesh = MoveTemp(CreateMeshParams.DynamicMesh.GetValue());
		if (SetMesh.IsCompact() == false)
		{
			SetMesh.CompactInPlace();
		}
		NewComponent->SetMesh(MoveTemp(SetMesh));
		NewComponent->NotifyMeshUpdated();
	}
	else if (CreateMeshParams.MeshType == ETLLRN_CreateMeshObjectSourceMeshType::MeshDescription)
	{
		const FMeshDescription* MeshDescription = &CreateMeshParams.MeshDescription.GetValue();
		FDynamicMesh3 Mesh(EMeshComponents::FaceGroups);
		Mesh.EnableAttributes();
		FMeshDescriptionToDynamicMesh Converter;
		Converter.Convert(MeshDescription, Mesh, true);
		NewComponent->SetMesh(MoveTemp(Mesh));
	}
	else
	{
		return FTLLRN_CreateMeshObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_InvalidMesh };
	}

	NewActor->SetActorTransform(CreateMeshParams.Transform);
	FActorLabelUtilities::SetActorLabelUnique(NewActor, CreateMeshParams.BaseName);

	// set materials
	TArray<UMaterialInterface*> ComponentMaterials = FilterMaterials(CreateMeshParams.Materials);
	for (int32 k = 0; k < ComponentMaterials.Num(); ++k)
	{
		NewComponent->SetMaterial(k, ComponentMaterials[k]);
	}

	// configure collision
	if (CreateMeshParams.bEnableCollision)
	{
		if (CreateMeshParams.CollisionShapeSet.IsSet())
		{
			UE::Geometry::SetSimpleCollision(NewComponent, CreateMeshParams.CollisionShapeSet.GetPtrOrNull());
		}

		NewComponent->CollisionType = CreateMeshParams.CollisionMode;
		// enable complex collision so that raycasts can hit this object
		NewComponent->bEnableComplexCollision = true;

		// force collision update
		NewComponent->UpdateCollision(false);
	}

	// configure raytracing
	NewComponent->SetEnableRaytracing(CreateMeshParams.bEnableRaytracingSupport);

	NewActor->PostEditChange();

	// emit result
	FTLLRN_CreateMeshObjectResult ResultOut;
	ResultOut.ResultCode = ETLLRN_CreateModelingObjectResult::Ok;
	ResultOut.NewActor = NewActor;
	ResultOut.NewComponent = NewComponent;
	ResultOut.NewAsset = nullptr;
	return ResultOut;
}




FTLLRN_CreateMeshObjectResult UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::CreateStaticMeshAsset(FTLLRN_CreateMeshObjectParams&& CreateMeshParams)
{
	UE::AssetUtils::FStaticMeshAssetOptions AssetOptions;
	
	ETLLRN_CreateModelingObjectResult AssetPathResult = GetNewAssetPath(
		AssetOptions.NewAssetPath,
		CreateMeshParams.BaseName,
		nullptr,
		CreateMeshParams.TargetWorld);
		
	if (AssetPathResult != ETLLRN_CreateModelingObjectResult::Ok)
	{
		return FTLLRN_CreateMeshObjectResult{ AssetPathResult };
	}

	AssetOptions.NumSourceModels = 1;
	AssetOptions.NumMaterialSlots = CreateMeshParams.Materials.Num();
	AssetOptions.AssetMaterials = (CreateMeshParams.AssetMaterials.Num() == AssetOptions.NumMaterialSlots) ?
		FilterMaterials(CreateMeshParams.AssetMaterials) : FilterMaterials(CreateMeshParams.Materials);

	AssetOptions.bEnableRecomputeNormals = CreateMeshParams.bEnableRecomputeNormals;
	AssetOptions.bEnableRecomputeTangents = CreateMeshParams.bEnableRecomputeTangents;
	AssetOptions.bGenerateNaniteEnabledMesh = CreateMeshParams.bEnableNanite;
	AssetOptions.NaniteSettings = CreateMeshParams.NaniteSettings;
	AssetOptions.bGenerateLightmapUVs = CreateMeshParams.bGenerateLightmapUVs;

	AssetOptions.bCreatePhysicsBody = CreateMeshParams.bEnableCollision;
	AssetOptions.CollisionType = CreateMeshParams.CollisionMode;

	if (CreateMeshParams.MeshType == ETLLRN_CreateMeshObjectSourceMeshType::DynamicMesh)
	{
		FDynamicMesh3* DynamicMesh = &CreateMeshParams.DynamicMesh.GetValue();
		AssetOptions.SourceMeshes.DynamicMeshes.Add(DynamicMesh);
	}
	else if (CreateMeshParams.MeshType == ETLLRN_CreateMeshObjectSourceMeshType::MeshDescription)
	{
		FMeshDescription* MeshDescription = &CreateMeshParams.MeshDescription.GetValue();
		AssetOptions.SourceMeshes.MoveMeshDescriptions.Add(MeshDescription);
	}
	else
	{
		return FTLLRN_CreateMeshObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_InvalidMesh };
	}

	UE::AssetUtils::FStaticMeshResults ResultData;
	UE::AssetUtils::ECreateStaticMeshResult AssetResult = UE::AssetUtils::CreateStaticMeshAsset(AssetOptions, ResultData);

	if (AssetResult != UE::AssetUtils::ECreateStaticMeshResult::Ok)
	{
		return FTLLRN_CreateMeshObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_AssetCreationFailed };
	}

	UStaticMesh* NewStaticMesh = ResultData.StaticMesh;

	// create new StaticMeshActor
	AStaticMeshActor* const StaticMeshActor = [NewStaticMesh, &CreateMeshParams]() -> AStaticMeshActor*
	{
		if (UActorFactory* const StaticMeshFactory = FActorFactoryAssetProxy::GetFactoryForAssetObject(NewStaticMesh))
		{
			AActor* const Actor = StaticMeshFactory->CreateActor(NewStaticMesh, CreateMeshParams.TargetWorld->GetCurrentLevel(), FTransform::Identity);
			FActorLabelUtilities::SetActorLabelUnique(Actor, CreateMeshParams.BaseName);
			return Cast<AStaticMeshActor>(Actor);
		}
		return nullptr;
	}();
	if (!StaticMeshActor)
	{
		return FTLLRN_CreateMeshObjectResult{ETLLRN_CreateModelingObjectResult::Failed_ActorCreationFailed};
	}

	// set the mesh
	UStaticMeshComponent* const StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();

	// this disconnects the component from various events
	StaticMeshComponent->UnregisterComponent();
	// replace the UStaticMesh in the component
	StaticMeshComponent->SetStaticMesh(NewStaticMesh);

	// set materials
	TArray<UMaterialInterface*> ComponentMaterials = FilterMaterials(CreateMeshParams.Materials);
	for (int32 k = 0; k < ComponentMaterials.Num(); ++k)
	{
		StaticMeshComponent->SetMaterial(k, ComponentMaterials[k]);
	}

	// set simple collision geometry
	if (CreateMeshParams.CollisionShapeSet.IsSet())
	{
		UE::Geometry::SetSimpleCollision(StaticMeshComponent, CreateMeshParams.CollisionShapeSet.GetPtrOrNull(),
			UE::Geometry::GetCollisionSettings(StaticMeshComponent));
	}

	// re-connect the component (?)
	StaticMeshComponent->RegisterComponent();

	NewStaticMesh->PostEditChange();

	StaticMeshComponent->RecreatePhysicsState();

	// update transform
	StaticMeshActor->SetActorTransform(CreateMeshParams.Transform);

	// emit result
	FTLLRN_CreateMeshObjectResult ResultOut;
	ResultOut.ResultCode = ETLLRN_CreateModelingObjectResult::Ok;
	ResultOut.NewActor = StaticMeshActor;
	ResultOut.NewComponent = StaticMeshComponent;
	ResultOut.NewAsset = NewStaticMesh;
	return ResultOut;
}




FTLLRN_CreateTextureObjectResult UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::CreateTextureObject(FTLLRN_CreateTextureObjectParams&& CreateTexParams)
{
	// currently we cannot create a new texture without an existing generated texture to store
	if (!ensure(CreateTexParams.GeneratedTransientTexture))
	{
		return FTLLRN_CreateTextureObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_InvalidTexture };
	}
	UE::AssetUtils::FTexture2DAssetOptions AssetOptions;
	UE::AssetUtils::FTexture2DAssetResults ResultData;
	
	if (CreateTexParams.FullAssetPath != "")
	{
		AssetOptions.NewAssetPath = CreateTexParams.FullAssetPath;
	}
	else
	{
		const ETLLRN_CreateModelingObjectResult AssetPathResult = GetNewAssetPath(
			AssetOptions.NewAssetPath,
			CreateTexParams.BaseName,
			CreateTexParams.StoreRelativeToObject,
			CreateTexParams.TargetWorld);

		if (AssetPathResult != ETLLRN_CreateModelingObjectResult::Ok)
		{
			return FTLLRN_CreateTextureObjectResult{ AssetPathResult };
		}

	}
	const UE::AssetUtils::ECreateTexture2DResult AssetResult = UE::AssetUtils::SaveGeneratedTexture2DAsset(
			CreateTexParams.GeneratedTransientTexture, AssetOptions, ResultData);
	

	if (AssetResult != UE::AssetUtils::ECreateTexture2DResult::Ok)
	{
		return FTLLRN_CreateTextureObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_AssetCreationFailed };
	}

	// emit result
	FTLLRN_CreateTextureObjectResult ResultOut;
	ResultOut.ResultCode = ETLLRN_CreateModelingObjectResult::Ok;
	ResultOut.NewAsset = ResultData.Texture;

	OnModelingTextureCreated.Broadcast(ResultOut);

	return ResultOut;
}




FTLLRN_CreateMaterialObjectResult UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::CreateMaterialObject(FTLLRN_CreateMaterialObjectParams&& CreateMaterialParams)
{
	UE::AssetUtils::FMaterialAssetOptions AssetOptions;

	ETLLRN_CreateModelingObjectResult AssetPathResult = GetNewAssetPath(
		AssetOptions.NewAssetPath,
		CreateMaterialParams.BaseName,
		CreateMaterialParams.StoreRelativeToObject,
		CreateMaterialParams.TargetWorld);

	if (AssetPathResult != ETLLRN_CreateModelingObjectResult::Ok)
	{
		return FTLLRN_CreateMaterialObjectResult{ AssetPathResult };
	}

	// Currently we cannot create a new material without duplicating an existing material
	if (!ensure(CreateMaterialParams.MaterialToDuplicate))
	{
		return FTLLRN_CreateMaterialObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_InvalidMaterial };
	}

	UE::AssetUtils::FMaterialAssetResults ResultData;
	UE::AssetUtils::ECreateMaterialResult AssetResult = UE::AssetUtils::CreateDuplicateMaterial(
		CreateMaterialParams.MaterialToDuplicate,
		AssetOptions,
		ResultData);

	if (AssetResult != UE::AssetUtils::ECreateMaterialResult::Ok)
	{
		return FTLLRN_CreateMaterialObjectResult{ ETLLRN_CreateModelingObjectResult::Failed_AssetCreationFailed };
	}

	// emit result
	FTLLRN_CreateMaterialObjectResult ResultOut;
	ResultOut.ResultCode = ETLLRN_CreateModelingObjectResult::Ok;
	ResultOut.NewAsset = ResultData.NewMaterial;

	OnModelingMaterialCreated.Broadcast(ResultOut);

	return ResultOut;
}




FTLLRN_CreateActorResult UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::CreateNewActor(FTLLRN_CreateActorParams&& TLLRN_CreateActorParams)
{
	// create new Actor
	AActor* NewActor = nullptr;
	
	if (GEditor && TLLRN_CreateActorParams.TemplateAsset)
	{
		if (UActorFactory* const ActorFactory =  FActorFactoryAssetProxy::GetFactoryForAssetObject(TLLRN_CreateActorParams.TemplateAsset))
		{
			NewActor = ActorFactory->CreateActor(TLLRN_CreateActorParams.TemplateAsset, TLLRN_CreateActorParams.TargetWorld->GetCurrentLevel(), TLLRN_CreateActorParams.Transform);
			FActorLabelUtilities::SetActorLabelUnique(NewActor, TLLRN_CreateActorParams.BaseName);
		}
	}

	if (!NewActor)
	{
		return FTLLRN_CreateActorResult{ ETLLRN_CreateModelingObjectResult::Failed_ActorCreationFailed };
	}

	FTLLRN_CreateActorResult ResultOut;
	ResultOut.ResultCode = ETLLRN_CreateModelingObjectResult::Ok;
	ResultOut.NewActor = NewActor;

	OnModelingActorCreated.Broadcast(ResultOut);

	return ResultOut;
}



ETLLRN_CreateModelingObjectResult UTLLRN_EditorTLLRN_ModelingObjectsCreationAPI::GetNewAssetPath(
	FString& OutNewAssetPath,
	const FString& BaseName,
	const UObject* StoreRelativeToObject,
	const UWorld* TargetWorld)
{
	FString RelativeToObjectFolder;
	if (StoreRelativeToObject != nullptr)
	{
		// find path to asset
		UPackage* AssetOuterPackage = CastChecked<UPackage>(StoreRelativeToObject->GetOuter());
		if (ensure(AssetOuterPackage))
		{
			FString AssetPackageName = AssetOuterPackage->GetName();
			RelativeToObjectFolder = FPackageName::GetLongPackagePath(AssetPackageName);
		}
	}
	else
	{
		if (!ensure(TargetWorld)) {
			return ETLLRN_CreateModelingObjectResult::Failed_InvalidWorld;
		}
	}

	if (GetNewAssetPathNameCallback.IsBound())
	{
		OutNewAssetPath = GetNewAssetPathNameCallback.Execute(BaseName, TargetWorld, RelativeToObjectFolder);
		if (OutNewAssetPath.Len() == 0)
		{
			return ETLLRN_CreateModelingObjectResult::Cancelled;
		}
	}
	else
	{
		FString UseBaseFolder = (RelativeToObjectFolder.Len() > 0) ? RelativeToObjectFolder : TEXT("/Game");
		OutNewAssetPath = FPaths::Combine(UseBaseFolder, BaseName);
	}

	return ETLLRN_CreateModelingObjectResult::Ok;
}
