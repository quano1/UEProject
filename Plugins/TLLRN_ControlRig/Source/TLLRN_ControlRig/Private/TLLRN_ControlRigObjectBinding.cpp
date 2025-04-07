// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigObjectBinding.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TLLRN_ControlRigComponent.h"

FTLLRN_ControlRigObjectBinding::~FTLLRN_ControlRigObjectBinding()
{
}

void FTLLRN_ControlRigObjectBinding::BindToObject(UObject* InObject)
{
	BoundObject = GetBindableObject(InObject);
	TLLRN_ControlRigBind.Broadcast(BoundObject.Get());
}

void FTLLRN_ControlRigObjectBinding::UnbindFromObject()
{
	BoundObject = nullptr;

	TLLRN_ControlRigUnbind.Broadcast();
}

bool FTLLRN_ControlRigObjectBinding::IsBoundToObject(UObject* InObject) const
{
	return InObject != nullptr && BoundObject.Get() == GetBindableObject(InObject);
}

UObject* FTLLRN_ControlRigObjectBinding::GetBoundObject() const
{
	return BoundObject.Get();
}

AActor* FTLLRN_ControlRigObjectBinding::GetHostingActor() const
{
	if (USceneComponent* SceneComponent = Cast<USceneComponent>(BoundObject.Get()))
	{
		return SceneComponent->GetOwner();
	}

	return nullptr;
}

UObject* FTLLRN_ControlRigObjectBinding::GetBindableObject(UObject* InObject) 
{
	// If we are binding to an actor, find the first skeletal mesh component
	if (AActor* Actor = Cast<AActor>(InObject))
	{
		if (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent = Actor->FindComponentByClass<UTLLRN_ControlRigComponent>())
		{
			return TLLRN_ControlRigComponent;
		}
		else if (USkeletalMeshComponent* SkeletalMeshComponent = Actor->FindComponentByClass<USkeletalMeshComponent>())
		{
			return SkeletalMeshComponent;
		}
	}
	else if (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent = Cast<UTLLRN_ControlRigComponent>(InObject))
	{
		return TLLRN_ControlRigComponent;
	}
	else if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(InObject))
	{
		return SkeletalMeshComponent;
	}
	else if (USkeleton* Skeleton = Cast<USkeleton>(InObject))
	{
		return Skeleton;
	}

	return nullptr;
}
