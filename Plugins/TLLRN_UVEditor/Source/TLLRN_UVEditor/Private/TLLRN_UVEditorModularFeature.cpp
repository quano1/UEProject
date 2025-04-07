// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditorModularFeature.h"

#include "CoreMinimal.h"
#include "TLLRN_UVEditorSubsystem.h"
#include "Editor.h"
#include "GameFramework/Actor.h"

namespace UE
{
namespace Geometry
{
	void FTLLRN_UVEditorModularFeature::LaunchUVEditor(const TArray<TObjectPtr<UObject>>& ObjectsIn)
	{
		UTLLRN_UVEditorSubsystem* UVSubsystem = GEditor->GetEditorSubsystem<UTLLRN_UVEditorSubsystem>();
		check(UVSubsystem);
		TArray<TObjectPtr<UObject>> ProcessedObjects;
		ConvertInputArgsToValidTargets(ObjectsIn, ProcessedObjects);
		UVSubsystem->StartTLLRN_UVEditor(ProcessedObjects);
	}

	bool FTLLRN_UVEditorModularFeature::CanLaunchUVEditor(const TArray<TObjectPtr<UObject>>& ObjectsIn)
	{
		UTLLRN_UVEditorSubsystem* UVSubsystem = GEditor->GetEditorSubsystem<UTLLRN_UVEditorSubsystem>();
		check(UVSubsystem);
		TArray<TObjectPtr<UObject>> ProcessedObjects;
		ConvertInputArgsToValidTargets(ObjectsIn, ProcessedObjects);
		return UVSubsystem->AreObjectsValidTargets(ProcessedObjects);
	}

	void FTLLRN_UVEditorModularFeature::ConvertInputArgsToValidTargets(const TArray<TObjectPtr<UObject>>& ObjectsIn, TArray<TObjectPtr<UObject>>& ObjectsOut) const
	{
		for (const TObjectPtr<UObject>& Object : ObjectsIn)
		{
			const AActor* Actor = Cast<const AActor>(Object);
			if (Actor)
			{
				TArray<UObject*> ActorAssets;
				Actor->GetReferencedContentObjects(ActorAssets);

				if (ActorAssets.Num() > 0)
				{
					for (UObject* Asset : ActorAssets)
					{
						ObjectsOut.AddUnique(Asset);
					}
				}
				else {
					// Need to transform actors to components here because that's what the TLLRN_UVEditor expects to have
					TInlineComponentArray<UActorComponent*> ActorComponents;
					Actor->GetComponents(ActorComponents);
					ObjectsOut.Append(ActorComponents);
				}
			}
			else
			{
				ObjectsOut.AddUnique(Object);
			}
		}
	}
}
}
