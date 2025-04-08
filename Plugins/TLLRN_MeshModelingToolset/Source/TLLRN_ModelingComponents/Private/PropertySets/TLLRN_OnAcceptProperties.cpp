// Copyright Epic Games, Inc. All Rights Reserved.

#include "PropertySets/TLLRN_OnAcceptProperties.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectGlobals.h"
#include "InteractiveToolManager.h"

#if WITH_EDITOR
#include "Editor/UnrealEdEngine.h"
#include "Selection.h"
#include "UnrealEdGlobals.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_OnAcceptProperties)

#define LOCTEXT_NAMESPACE "UTLLRN_OnAcceptHandleSourcesProperties"


void UTLLRN_OnAcceptHandleSourcesPropertiesBase::ApplyMethod(const TArray<AActor*>& Actors, UInteractiveToolManager* ToolManager, const AActor* MustKeepActor)
{
	const ETLLRN_HandleSourcesMethod HandleInputs = GetHandleInputs();

	// Hide or destroy the sources
	bool bKeepSources = HandleInputs == ETLLRN_HandleSourcesMethod::KeepSources;
	if (Actors.Num() == 1 && (HandleInputs == ETLLRN_HandleSourcesMethod::KeepFirstSource || HandleInputs == ETLLRN_HandleSourcesMethod::KeepLastSource))
	{
		// if there's only one actor, keeping any source == keeping all sources
		bKeepSources = true;
	}
	if (!bKeepSources)
	{
		bool bDelete = HandleInputs == ETLLRN_HandleSourcesMethod::DeleteSources
					|| HandleInputs == ETLLRN_HandleSourcesMethod::KeepFirstSource
					|| HandleInputs == ETLLRN_HandleSourcesMethod::KeepLastSource;
		if (bDelete)
		{
			ToolManager->BeginUndoTransaction(LOCTEXT("RemoveSources", "Remove Inputs"));
		}
		else
		{
#if WITH_EDITOR
			ToolManager->BeginUndoTransaction(LOCTEXT("HideSources", "Hide Inputs"));
#endif
		}

		const int32 ActorIdxBegin = HandleInputs == ETLLRN_HandleSourcesMethod::KeepFirstSource ? 1 : 0;
		const int32 ActorIdxEnd = HandleInputs == ETLLRN_HandleSourcesMethod::KeepLastSource ? Actors.Num() - 1 : Actors.Num();

		for (int32 ActorIdx = ActorIdxBegin; ActorIdx < ActorIdxEnd; ActorIdx++)
		{
			AActor* Actor = Actors[ActorIdx];
			if (Actor == MustKeepActor)
			{
				continue;
			}

			if (bDelete)
			{
				if (UWorld* ActorWorld = Actor->GetWorld())
				{
#if WITH_EDITOR
					if (GIsEditor && GUnrealEd)
					{
						GUnrealEd->DeleteActors(TArray{Actor}, ActorWorld, GUnrealEd->GetSelectedActors()->GetElementSelectionSet());
					}
					else
#endif
					{
						ActorWorld->DestroyActor(Actor);
					}
				}
			}
			else
			{
#if WITH_EDITOR
				// Save the actor to the transaction buffer to support undo/redo, but do
				// not call Modify, as we do not want to dirty the actor's package and
				// we're only editing temporary, transient values
				SaveToTransactionBuffer(Actor, false);
				Actor->SetIsTemporarilyHiddenInEditor(true);
#endif
			}
		}
		if (bDelete)
		{
			ToolManager->EndUndoTransaction();
		}
		else
		{
#if WITH_EDITOR
			ToolManager->EndUndoTransaction();
#endif
		}
	}
}

#undef LOCTEXT_NAMESPACE

