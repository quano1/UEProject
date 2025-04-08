// Copyright Epic Games, Inc. All Rights Reserved.

#include "Drawing/TLLRN_PreviewGeometryActor.h"
#include "ToolSetupUtil.h"

// to create sphere root component
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Engine/CollisionProfile.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_PreviewGeometryActor)


UTLLRN_PreviewGeometry::~UTLLRN_PreviewGeometry()
{
	checkf(ParentActor == nullptr, TEXT("You must explicitly Disconnect() UTLLRN_PreviewGeometry before it is GCd"));
}

void UTLLRN_PreviewGeometry::CreateInWorld(UWorld* World, const FTransform& WithTransform)
{
	FRotator Rotation(0.0f, 0.0f, 0.0f);
	FActorSpawnParameters SpawnInfo;
	ParentActor = World->SpawnActor<ATLLRN_PreviewGeometryActor>(FVector::ZeroVector, Rotation, SpawnInfo);

	// root component is a hidden sphere
	USphereComponent* SphereComponent = NewObject<USphereComponent>(ParentActor);
	ParentActor->SetRootComponent(SphereComponent);
	SphereComponent->InitSphereRadius(1.0f);
	SphereComponent->SetVisibility(false);
	SphereComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	ParentActor->SetActorTransform(WithTransform);

	OnCreated();
}


void UTLLRN_PreviewGeometry::Disconnect()
{
	OnDisconnected();

	if (ParentActor != nullptr)
	{
		ParentActor->Destroy();
		ParentActor = nullptr;
	}
}



FTransform UTLLRN_PreviewGeometry::GetTransform() const
{
	if (ParentActor != nullptr)
	{
		return ParentActor->GetTransform();
	}
	return FTransform();
}

void UTLLRN_PreviewGeometry::SetTransform(const FTransform& UseTransform)
{
	if (ParentActor != nullptr)
	{
		ParentActor->SetActorTransform(UseTransform);
	}
}


void UTLLRN_PreviewGeometry::SetAllVisible(bool bVisible)
{
	for (TPair<FString, TObjectPtr<UTLLRN_TriangleSetComponent>> Entry : TriangleSets)
	{
		Entry.Value->SetVisibility(bVisible);
	}
	for (TPair<FString, TObjectPtr<UTLLRN_LineSetComponent>> Entry : LineSets)
	{
		Entry.Value->SetVisibility(bVisible);
	}
	for (TPair<FString, TObjectPtr<UTLLRN_PointSetComponent>> Entry : PointSets)
	{
		Entry.Value->SetVisibility(bVisible);
	}
}


UTLLRN_TriangleSetComponent* UTLLRN_PreviewGeometry::AddTriangleSet(const FString& SetIdentifier)
{
	if (TriangleSets.Contains(SetIdentifier))
	{
		check(false);
		return nullptr;
	}

	UTLLRN_TriangleSetComponent* TriangleSet = NewObject<UTLLRN_TriangleSetComponent>(ParentActor);
	TriangleSet->SetupAttachment(ParentActor->GetRootComponent());
	TriangleSet->RegisterComponent();

	TriangleSets.Add(SetIdentifier, TriangleSet);
	return TriangleSet;
}


UTLLRN_TriangleSetComponent* UTLLRN_PreviewGeometry::FindTriangleSet(const FString& TriangleSetIdentifier)
{
	TObjectPtr<UTLLRN_TriangleSetComponent>* Found = TriangleSets.Find(TriangleSetIdentifier);
	if (Found != nullptr)
	{
		return *Found;
	}
	return nullptr;
}



UTLLRN_TriangleSetComponent* UTLLRN_PreviewGeometry::CreateOrUpdateTriangleSet(const FString& TriangleSetIdentifier, int32 NumIndices,
	TFunctionRef<void(int32 Index, TArray<FTLLRN_RenderableTriangle>& TrianglesOut)> TriangleGenFunc,
	int32 TrianglesPerIndexHint)
{
	UTLLRN_TriangleSetComponent* TriangleSet = FindTriangleSet(TriangleSetIdentifier);
	if (TriangleSet == nullptr)
	{
		TriangleSet = AddTriangleSet(TriangleSetIdentifier);
		if (TriangleSet == nullptr)
		{
			check(false);
			return nullptr;
		}
	}

	TriangleSet->Clear();
	TriangleSet->AddTriangles(NumIndices, TriangleGenFunc, TrianglesPerIndexHint);
	return TriangleSet;
}


UTLLRN_LineSetComponent* UTLLRN_PreviewGeometry::AddLineSet(const FString& SetIdentifier)
{
	if (LineSets.Contains(SetIdentifier))
	{
		check(false);
		return nullptr;
	}

	UTLLRN_LineSetComponent* LineSet = NewObject<UTLLRN_LineSetComponent>(ParentActor);
	LineSet->SetupAttachment(ParentActor->GetRootComponent());

	UMaterialInterface* LineMaterial = ToolSetupUtil::GetDefaultLineComponentMaterial(nullptr);
	if (LineMaterial != nullptr)
	{
		LineSet->SetLineMaterial(LineMaterial);
	}

	LineSet->RegisterComponent();

	LineSets.Add(SetIdentifier, LineSet);
	return LineSet;
}


UTLLRN_LineSetComponent* UTLLRN_PreviewGeometry::FindLineSet(const FString& LineSetIdentifier)
{
	TObjectPtr<UTLLRN_LineSetComponent>* Found = LineSets.Find(LineSetIdentifier);
	if (Found != nullptr)
	{
		return *Found;
	}
	return nullptr;
}

bool UTLLRN_PreviewGeometry::RemoveLineSet(const FString& LineSetIdentifier, bool bDestroy)
{
	TObjectPtr<UTLLRN_LineSetComponent>* Found = LineSets.Find(LineSetIdentifier);
	if (Found != nullptr)
	{
		UTLLRN_LineSetComponent* LineSet = *Found;
		LineSets.Remove(LineSetIdentifier);
		if (bDestroy)
		{
			LineSet->UnregisterComponent();
			LineSet->DestroyComponent();
			LineSet = nullptr;
		}
		return true;
	}
	return false;
}



void UTLLRN_PreviewGeometry::RemoveAllLineSets(bool bDestroy)
{
	if (bDestroy)
	{
		for (TPair<FString, TObjectPtr<UTLLRN_LineSetComponent>> Entry : LineSets)
		{
			Entry.Value->UnregisterComponent();
			Entry.Value->DestroyComponent();
		}
	}
	LineSets.Reset();
}


bool UTLLRN_PreviewGeometry::RemoveTriangleSet(const FString& TriangleSetIdentifier, bool bDestroy)
{
	TObjectPtr<UTLLRN_TriangleSetComponent>* Found = TriangleSets.Find(TriangleSetIdentifier);
	if (Found != nullptr)
	{
		UTLLRN_TriangleSetComponent* TriangleSet = *Found;

		TriangleSets.Remove(TriangleSetIdentifier);
		if (bDestroy)
		{
			TriangleSet->UnregisterComponent();
			TriangleSet->DestroyComponent();
			TriangleSet = nullptr;
		}
		return true;
	}
	return false;
}



void UTLLRN_PreviewGeometry::RemoveAllTriangleSets(bool bDestroy)
{
	if (bDestroy)
	{
		for (TPair<FString, TObjectPtr<UTLLRN_TriangleSetComponent>> Entry : TriangleSets)
		{
			Entry.Value->UnregisterComponent();
			Entry.Value->DestroyComponent();
		}
	}
	TriangleSets.Reset();
}



bool UTLLRN_PreviewGeometry::SetLineSetVisibility(const FString& LineSetIdentifier, bool bVisible)
{
	TObjectPtr<UTLLRN_LineSetComponent>* Found = LineSets.Find(LineSetIdentifier);
	if (Found != nullptr)
	{
		(*Found)->SetVisibility(bVisible);
		return true;
	}
	return false;
}


bool UTLLRN_PreviewGeometry::SetLineSetMaterial(const FString& LineSetIdentifier, UMaterialInterface* NewMaterial)
{
	TObjectPtr<UTLLRN_LineSetComponent>* Found = LineSets.Find(LineSetIdentifier);
	if (Found != nullptr)
	{
		(*Found)->SetLineMaterial(NewMaterial);
		return true;
	}
	return false;
}


void UTLLRN_PreviewGeometry::SetAllLineSetsMaterial(UMaterialInterface* Material)
{
	for (TPair<FString, TObjectPtr<UTLLRN_LineSetComponent>> Entry : LineSets)
	{
		Entry.Value->SetLineMaterial(Material);
	}
}



UTLLRN_LineSetComponent* UTLLRN_PreviewGeometry::CreateOrUpdateLineSet(const FString& LineSetIdentifier, int32 NumIndices,
	TFunctionRef<void(int32 Index, TArray<FRenderableLine>& LinesOut)> LineGenFunc,
	int32 LinesPerIndexHint)
{
	UTLLRN_LineSetComponent* LineSet = FindLineSet(LineSetIdentifier);
	if (LineSet == nullptr)
	{
		LineSet = AddLineSet(LineSetIdentifier);
		if (LineSet == nullptr)
		{
			check(false);
			return nullptr;
		}
	}

	LineSet->Clear();
	LineSet->AddLines(NumIndices, LineGenFunc, LinesPerIndexHint);
	return LineSet;
}


UTLLRN_PointSetComponent* UTLLRN_PreviewGeometry::AddPointSet(const FString& SetIdentifier)
{
	if (PointSets.Contains(SetIdentifier))
	{
		check(false);
		return nullptr;
	}

	UTLLRN_PointSetComponent* PointSet = NewObject<UTLLRN_PointSetComponent>(ParentActor);
	PointSet->SetupAttachment(ParentActor->GetRootComponent());

	UMaterialInterface* PointMaterial = ToolSetupUtil::GetDefaultPointComponentMaterial(nullptr);
	if (PointMaterial != nullptr)
	{
		PointSet->SetPointMaterial(PointMaterial);
	}

	PointSet->RegisterComponent();

	PointSets.Add(SetIdentifier, PointSet);
	return PointSet;
}

UTLLRN_PointSetComponent* UTLLRN_PreviewGeometry::FindPointSet(const FString& PointSetIdentifier)
{
	TObjectPtr<UTLLRN_PointSetComponent>* Found = PointSets.Find(PointSetIdentifier);
	if (Found != nullptr)
	{
		return *Found;
	}
	return nullptr;
}

void UTLLRN_PreviewGeometry::CreateOrUpdatePointSet(const FString& PointSetIdentifier, int32 NumIndices,
	TFunctionRef<void(int32 Index, TArray<FRenderablePoint>& PointsOut)> PointGenFunc,
	int32 PointsPerIndexHint)
{
	UTLLRN_PointSetComponent* PointSet = FindPointSet(PointSetIdentifier);
	if (PointSet == nullptr)
	{
		PointSet = AddPointSet(PointSetIdentifier);
		if (PointSet == nullptr)
		{
			check(false);
			return;
		}
	}

	PointSet->Clear();
	PointSet->AddPoints(NumIndices, PointGenFunc, PointsPerIndexHint);
}

bool UTLLRN_PreviewGeometry::RemovePointSet(const FString& PointSetIdentifier, bool bDestroy)
{
	TObjectPtr<UTLLRN_PointSetComponent>* Found = PointSets.Find(PointSetIdentifier);
	if (Found != nullptr)
	{
		UTLLRN_PointSetComponent* PointSet = *Found;
		PointSets.Remove(PointSetIdentifier);
		if (bDestroy)
		{
			PointSet->UnregisterComponent();
			PointSet->DestroyComponent();
			PointSet = nullptr;
		}
		return true;
	}
	return false;
}

void UTLLRN_PreviewGeometry::RemoveAllPointSets(bool bDestroy)
{
	if (bDestroy)
	{
		for (TPair<FString, TObjectPtr<UTLLRN_PointSetComponent>> Entry : PointSets)
		{
			Entry.Value->UnregisterComponent();
			Entry.Value->DestroyComponent();
		}
	}
	PointSets.Reset();
}

bool UTLLRN_PreviewGeometry::SetPointSetVisibility(const FString& PointSetIdentifier, bool bVisible)
{
	TObjectPtr<UTLLRN_PointSetComponent>* Found = PointSets.Find(PointSetIdentifier);
	if (Found != nullptr)
	{
		(*Found)->SetVisibility(bVisible);
		return true;
	}
	return false;
}

bool UTLLRN_PreviewGeometry::SetPointSetMaterial(const FString& PointSetIdentifier, UMaterialInterface* NewMaterial)
{
	TObjectPtr<UTLLRN_PointSetComponent>* Found = PointSets.Find(PointSetIdentifier);
	if (Found != nullptr)
	{
		(*Found)->SetPointMaterial(NewMaterial);
		return true;
	}
	return false;
}

void UTLLRN_PreviewGeometry::SetAllPointSetsMaterial(UMaterialInterface* Material)
{
	for (TPair<FString, TObjectPtr<UTLLRN_PointSetComponent>> Entry : PointSets)
	{
		Entry.Value->SetPointMaterial(Material);
	}
}
