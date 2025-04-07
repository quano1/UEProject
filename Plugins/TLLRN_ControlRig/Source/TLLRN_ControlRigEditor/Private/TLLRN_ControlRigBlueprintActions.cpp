// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigBlueprintActions.h"
#include "TLLRN_ControlRigBlueprintFactory.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "TLLRN_ControlRig.h"
#include "Editor/RigVMEditorStyle.h"
#include "ITLLRN_ControlRigEditorModule.h"

#include "Styling/SlateIconFinder.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Styling/AppStyle.h" 
#include "Subsystems/AssetEditorSubsystem.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ToolMenus.h"
#include "ContentBrowserMenuContexts.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "EditorDirectories.h"
#include "ILevelSequenceEditorToolkit.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "MovieSceneToolsProjectSettings.h"
#include "SBlueprintDiff.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "LevelSequenceEditorBlueprintLibrary.h"
#include "Sequencer/TLLRN_ControlRigParameterTrackEditor.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "TLLRN_ModularRig.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigBlueprintActions"

FDelegateHandle FTLLRN_ControlRigBlueprintActions::OnSpawnedSkeletalMeshActorChangedHandle;

UFactory* FTLLRN_ControlRigBlueprintActions::GetFactoryForBlueprintType(UBlueprint* InBlueprint) const
{
	UTLLRN_ControlRigBlueprintFactory* TLLRN_ControlRigBlueprintFactory = NewObject<UTLLRN_ControlRigBlueprintFactory>();
	UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = CastChecked<UTLLRN_ControlRigBlueprint>(InBlueprint);
	TLLRN_ControlRigBlueprintFactory->ParentClass = TSubclassOf<UTLLRN_ControlRig>(*InBlueprint->GeneratedClass);
	return TLLRN_ControlRigBlueprintFactory;
}

void FTLLRN_ControlRigBlueprintActions::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(*ObjIt))
		{
			const bool bBringToFrontIfOpen = true;
			UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
			if (IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(TLLRN_ControlRigBlueprint, bBringToFrontIfOpen))
			{
				EditorInstance->FocusWindow(TLLRN_ControlRigBlueprint);
			}
			else
			{				
				// If any other editors are opened (for example, a BlueprintDiff window), close them 
				AssetEditorSubsystem->CloseAllEditorsForAsset(TLLRN_ControlRigBlueprint);				
				
				ITLLRN_ControlRigEditorModule& TLLRN_ControlRigEditorModule = FModuleManager::LoadModuleChecked<ITLLRN_ControlRigEditorModule>("TLLRN_ControlRigEditor");
				TLLRN_ControlRigEditorModule.CreateTLLRN_ControlRigEditor(Mode, EditWithinLevelEditor, TLLRN_ControlRigBlueprint);
			}
		}
	}
}

TSharedPtr<SWidget> FTLLRN_ControlRigBlueprintActions::GetThumbnailOverlay(const FAssetData& AssetData) const
{
	const FSlateBrush* Icon = FSlateIconFinder::FindIconBrushForClass(UTLLRN_ControlRigBlueprint::StaticClass());

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetNoBrush())
		.Visibility(EVisibility::HitTestInvisible)
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 3.0f))
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		[
			SNew(SImage)
			.Image(Icon)
		];
}

void FTLLRN_ControlRigBlueprintActions::PerformAssetDiff(UObject* OldAsset, UObject* NewAsset, const FRevisionInfo& OldRevision, const FRevisionInfo& NewRevision) const
{
	UBlueprint* OldBlueprint = Cast<UBlueprint>(OldAsset);
	UBlueprint* NewBlueprint = Cast<UBlueprint>(NewAsset);

	static const FText DiffWindowMessage = LOCTEXT("TLLRN_ControlRigDiffWindow", "Opening a diff window will close the control rig editor. {0}.\nAre you sure?");
	
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (OldAsset)
	{
		for (IAssetEditorInstance* Editor : AssetEditorSubsystem->FindEditorsForAsset(OldAsset))
		{
			const EAppReturnType::Type Answer = FMessageDialog::Open( EAppMsgType::YesNo,
					FText::Format(DiffWindowMessage, FText::FromString(OldBlueprint->GetName())));
			if(Answer == EAppReturnType::No)
			{
			   return;
			}
		}
	}
	if (NewAsset)
	{
		for (IAssetEditorInstance* Editor : AssetEditorSubsystem->FindEditorsForAsset(NewAsset))
		{
			const EAppReturnType::Type Answer = FMessageDialog::Open( EAppMsgType::YesNo,
					FText::Format(DiffWindowMessage, FText::FromString(NewBlueprint->GetName())));
			if(Answer == EAppReturnType::No)
			{
				return;
			}
		}
	}

	if (OldAsset)
	{
		AssetEditorSubsystem->CloseAllEditorsForAsset(OldAsset);
	}
	
	if (NewAsset)
	{
		AssetEditorSubsystem->CloseAllEditorsForAsset(NewAsset);
	}

	SBlueprintDiff::CreateDiffWindow(OldBlueprint, NewBlueprint, OldRevision, NewRevision, GetSupportedClass());
}

void FTLLRN_ControlRigBlueprintActions::ExtendSketalMeshToolMenu()
{
	TArray<UToolMenu*> MenusToExtend;
	MenusToExtend.Add(UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.SkeletalMesh.CreateSkeletalMeshSubmenu"));
	MenusToExtend.Add(UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.Skeleton.CreateSkeletalMeshSubmenu"));

	for(UToolMenu* Menu : MenusToExtend)
	{
		if (Menu == nullptr)
		{
			continue;
		}

		FToolMenuSection& Section = Menu->AddSection("TLLRN_ControlRig", LOCTEXT("TLLRN_ControlRigSectionName", "TLLRN Control Rig"));
		Section.AddDynamicEntry("CreateTLLRN_ControlRig", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
		{
			UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>();
			if (Context)
			{
				if (Context->SelectedAssets.Num() > 0)
				{
					static constexpr bool bTLLRN_ModularRig = true;
					InSection.AddMenuEntry(
						"CreateTLLRN_ControlRig",
						LOCTEXT("CreateTLLRN_ControlRig", "TLLRN Control Rig"),
						LOCTEXT("CreateTLLRN_ControlRig_ToolTip", "Creates a control rig and preconfigures it for this asset"),
						FSlateIcon(FRigVMEditorStyle::Get().GetStyleSetName(), "RigVM", "RigVM.Unit"),
						FExecuteAction::CreateLambda([InSelectedAssets = Context->SelectedAssets]()
						{
							TArray<UObject*> SelectedObjects;
							SelectedObjects.Reserve(InSelectedAssets.Num());

							for (const FAssetData& Asset : InSelectedAssets)
							{
								if (UObject* LoadedAsset = Asset.GetAsset())
								{
									SelectedObjects.Add(LoadedAsset);
								}
							}


							for (UObject* SelectedObject : SelectedObjects)
							{
								CreateTLLRN_ControlRigFromSkeletalMeshOrSkeleton(SelectedObject, !bTLLRN_ModularRig);
							}
						})
					);
					InSection.AddMenuEntry(
						"CreateTLLRN_ModularRig",
						LOCTEXT("CreateTLLRN_ModularRig", "Modular Rig"),
						LOCTEXT("CreateTLLRN_ModularRig_ToolTip", "Creates a modular rig and preconfigures it for this asset"),
						FSlateIcon(FRigVMEditorStyle::Get().GetStyleSetName(), "RigVM", "RigVM.Unit"),
						FExecuteAction::CreateLambda([InSelectedAssets = Context->SelectedAssets]()
						{
							TArray<UObject*> SelectedObjects;
							SelectedObjects.Reserve(InSelectedAssets.Num());

							for (const FAssetData& Asset : InSelectedAssets)
							{
								if (UObject* LoadedAsset = Asset.GetAsset())
								{
									SelectedObjects.Add(LoadedAsset);
								}
							}

							for (UObject* SelectedObject : SelectedObjects)
							{
								CreateTLLRN_ControlRigFromSkeletalMeshOrSkeleton(SelectedObject, bTLLRN_ModularRig);
							}
						})
					);
				}
			}
		}));
	}
}

UTLLRN_ControlRigBlueprint* FTLLRN_ControlRigBlueprintActions::CreateNewTLLRN_ControlRigAsset(const FString& InDesiredPackagePath, const bool bTLLRN_ModularRig)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	UTLLRN_ControlRigBlueprintFactory* Factory = NewObject<UTLLRN_ControlRigBlueprintFactory>();
	Factory->ParentClass = bTLLRN_ModularRig ? UTLLRN_ModularRig::StaticClass() : UTLLRN_ControlRig::StaticClass();

	FString UniquePackageName;
	FString UniqueAssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(InDesiredPackagePath, TEXT(""), UniquePackageName, UniqueAssetName);

	if (UniquePackageName.EndsWith(UniqueAssetName))
	{
		UniquePackageName = UniquePackageName.LeftChop(UniqueAssetName.Len() + 1);
	}

	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(*UniqueAssetName, *UniquePackageName, nullptr, Factory);
	return Cast<UTLLRN_ControlRigBlueprint>(NewAsset);
}

UTLLRN_ControlRigBlueprint* FTLLRN_ControlRigBlueprintActions::CreateTLLRN_ControlRigFromSkeletalMeshOrSkeleton(UObject* InSelectedObject, const bool bTLLRN_ModularRig)
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(InSelectedObject);
	USkeleton* Skeleton = Cast<USkeleton>(InSelectedObject);
	const FReferenceSkeleton* RefSkeleton = nullptr;

	if(SkeletalMesh)
	{
		Skeleton = SkeletalMesh->GetSkeleton();
		RefSkeleton = &SkeletalMesh->GetRefSkeleton();
	}
	else if (Skeleton)
	{
		RefSkeleton = &Skeleton->GetReferenceSkeleton();
	}
	else
	{
		UE_LOG(LogTLLRN_ControlRigEditor, Error, TEXT("CreateTLLRN_ControlRigFromSkeletalMeshOrSkeleton: Provided object has to be a SkeletalMesh or Skeleton."));
		return nullptr;
	}

	check(RefSkeleton);

	FString PackagePath = InSelectedObject->GetPathName();
	FString TLLRN_ControlTLLRN_RigName = FString::Printf(TEXT("%s_CtrlRig"), *InSelectedObject->GetName());

	int32 LastSlashPos = INDEX_NONE;
	if (PackagePath.FindLastChar('/', LastSlashPos))
	{
		PackagePath = PackagePath.Left(LastSlashPos);
	}

	UTLLRN_ControlRigBlueprint* NewTLLRN_ControlRigBlueprint = CreateNewTLLRN_ControlRigAsset(PackagePath / TLLRN_ControlTLLRN_RigName, bTLLRN_ModularRig);
	if (NewTLLRN_ControlRigBlueprint == nullptr)
	{
		return nullptr;
	}

	if(UTLLRN_RigHierarchyController* Controller = NewTLLRN_ControlRigBlueprint->GetHierarchyController())
	{
		Controller->ImportBones(*RefSkeleton, NAME_None, false, false, false, false);
		if(SkeletalMesh)
		{
			Controller->ImportCurvesFromSkeletalMesh(SkeletalMesh, NAME_None, false, false);
		}
		else
		{
			Controller->ImportCurves(Skeleton, NAME_None, false, false);
		}
	}
	NewTLLRN_ControlRigBlueprint->SourceHierarchyImport = Skeleton;
	NewTLLRN_ControlRigBlueprint->SourceCurveImport = Skeleton;
	NewTLLRN_ControlRigBlueprint->PropagateHierarchyFromBPToInstances();

	if(SkeletalMesh)
	{
		NewTLLRN_ControlRigBlueprint->SetPreviewMesh(SkeletalMesh);
	}

	if(!bTLLRN_ModularRig)
	{
		NewTLLRN_ControlRigBlueprint->RecompileVM();
	}

	return NewTLLRN_ControlRigBlueprint;
}

USkeletalMesh* FTLLRN_ControlRigBlueprintActions::GetSkeletalMeshFromTLLRN_ControlRigBlueprint(const FAssetData& InAsset)
{
	if (InAsset.GetClass() != UTLLRN_ControlRigBlueprint::StaticClass())
	{
		return nullptr;
	}
	
	if (const UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(InAsset.GetAsset()))
	{
		return Blueprint->GetPreviewMesh();
	}
	return nullptr;
}

void FTLLRN_ControlRigBlueprintActions::PostSpawningSkeletalMeshActor(AActor* InSpawnedActor, UObject* InAsset)
{
	if (InSpawnedActor->HasAnyFlags(RF_Transient) || InSpawnedActor->bIsEditorPreviewActor)
	{
		return;
	}
	
	OnSpawnedSkeletalMeshActorChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddStatic(&FTLLRN_ControlRigBlueprintActions::OnSpawnedSkeletalMeshActorChanged, InAsset);
}

void FTLLRN_ControlRigBlueprintActions::OnSpawnedSkeletalMeshActorChanged(UObject* InObject, FPropertyChangedEvent& InEvent, UObject* InAsset)
{
	if (!OnSpawnedSkeletalMeshActorChangedHandle.IsValid())
	{
		return;
	}

	// we are waiting for the top level property change event
	// after the spawn.
	if (InEvent.Property != nullptr)
	{
		return;
	}

	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(OnSpawnedSkeletalMeshActorChangedHandle);
	OnSpawnedSkeletalMeshActorChangedHandle.Reset();

	// Create a level sequence but delay until next tick so that the creation of the asset is not in the existing transaction
	GEditor->GetTimerManager()->SetTimerForNextTick([InObject, InAsset]()
	{
		ASkeletalMeshActor* MeshActor = Cast<ASkeletalMeshActor>(InObject);
		check(MeshActor);
		UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(InAsset);
		if (RigBlueprint == nullptr)
		{
			return;
		}

		UClass* TLLRN_ControlRigClass = RigBlueprint->GeneratedClass;

		TGuardValue<bool> DisableTrackCreation(FTLLRN_ControlRigParameterTrackEditor::bAutoGenerateTLLRN_ControlRigTrack, false);

		// find a level sequence in the world, if can't find that, create one
		ULevelSequence* Sequence = ULevelSequenceEditorBlueprintLibrary::GetFocusedLevelSequence();
		if (Sequence == nullptr)
		{
			FString SequenceName = FString::Printf(TEXT("%s_Take1"), *InAsset->GetName());

			FString PackagePath;
			const FString DefaultDirectory = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::NEW_ASSET);
			FPackageName::TryConvertFilenameToLongPackageName(DefaultDirectory, PackagePath);
			if (PackagePath.IsEmpty())
			{
				PackagePath = TEXT("/Game");
			}

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
			FString UniquePackageName;
			FString UniqueAssetName;
			AssetToolsModule.Get().CreateUniqueAssetName(PackagePath / SequenceName, TEXT(""), UniquePackageName, UniqueAssetName);

			UPackage* Package = CreatePackage(*UniquePackageName);
			Sequence = NewObject<ULevelSequence>(Package, *UniqueAssetName, RF_Public | RF_Standalone);
			Sequence->Initialize(); //creates movie scene
			Sequence->MarkPackageDirty();

			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(Sequence);

			// Set up some sensible defaults
			const UMovieSceneToolsProjectSettings* ProjectSettings = GetDefault<UMovieSceneToolsProjectSettings>();
			FFrameRate TickResolution = Sequence->GetMovieScene()->GetTickResolution();
			Sequence->GetMovieScene()->SetPlaybackRange((ProjectSettings->DefaultStartTime * TickResolution).FloorToFrame(), (ProjectSettings->DefaultDuration * TickResolution).FloorToFrame().Value);

			if (UActorFactory* ActorFactory = GEditor->FindActorFactoryForActorClass(ALevelSequenceActor::StaticClass()))
			{
				if (ALevelSequenceActor* LevelSequenceActor = Cast<ALevelSequenceActor>(GEditor->UseActorFactory(ActorFactory, FAssetData(Sequence), &FTransform::Identity)))
				{
					LevelSequenceActor->SetSequence(Sequence);
				}
			}
		}

		if (Sequence == nullptr)
		{
			return;
		}

		UMovieScene* MovieScene = Sequence->GetMovieScene();

		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Sequence);

		IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Sequence, false);
		ILevelSequenceEditorToolkit* LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(AssetEditor);
		TWeakPtr<ISequencer> WeakSequencer = LevelSequenceEditor ? LevelSequenceEditor->GetSequencer() : nullptr;

		if (WeakSequencer.IsValid())
		{
			TArray<TWeakObjectPtr<AActor> > ActorsToAdd;
			ActorsToAdd.Add(MeshActor);
			TArray<FGuid> ActorTracks = WeakSequencer.Pin()->AddActors(ActorsToAdd, false);
			FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));

			for (FGuid ActorTrackGuid : ActorTracks)
			{
				//Delete binding from default animating rig
				FGuid CompGuid = WeakSequencer.Pin()->FindObjectId(*(MeshActor->GetSkeletalMeshComponent()), WeakSequencer.Pin()->GetFocusedTemplateID());
				if (CompGuid.IsValid())
				{
					if (TLLRN_ControlRigEditMode)
					{
						UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(MovieScene->FindTrack(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), CompGuid, NAME_None));
						if (Track && Track->GetTLLRN_ControlRig())
						{
							TLLRN_ControlRigEditMode->RemoveTLLRN_ControlRig(Track->GetTLLRN_ControlRig());
						}
					}
					if (!MovieScene->RemovePossessable(CompGuid))
					{
						MovieScene->RemoveSpawnable(CompGuid);
					}
				}

				UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(MovieScene->FindTrack(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), ActorTrackGuid));
				if (!Track)
				{
					Track = MovieScene->AddTrack<UMovieSceneTLLRN_ControlRigParameterTrack>(ActorTrackGuid);
				}

				UTLLRN_ControlRig* TLLRN_ControlRig = Track->GetTLLRN_ControlRig();

				FString ObjectName = (TLLRN_ControlRigClass->GetName());

				if (!TLLRN_ControlRig || TLLRN_ControlRig->GetClass() != TLLRN_ControlRigClass)
				{
					USkeletalMesh* SkeletalMesh = MeshActor->GetSkeletalMeshComponent()->GetSkeletalMeshAsset();
					USkeleton* Skeleton = SkeletalMesh->GetSkeleton();

					ObjectName.RemoveFromEnd(TEXT("_C"));

					// This is either a UTLLRN_ControlRig or a UTLLRN_ModularRig
					TLLRN_ControlRig = NewObject<UTLLRN_ControlRig>(Track, TLLRN_ControlRigClass, FName(*ObjectName), RF_Transactional);
					TLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
					TLLRN_ControlRig->GetObjectBinding()->BindToObject(MeshActor->GetSkeletalMeshComponent());
					TLLRN_ControlRig->GetDataSourceRegistry()->RegisterDataSource(UTLLRN_ControlRig::OwnerComponent, TLLRN_ControlRig->GetObjectBinding()->GetBoundObject());
					TLLRN_ControlRig->Initialize();
					TLLRN_ControlRig->Evaluate_AnyThread();
					TLLRN_ControlRig->CreateTLLRN_RigControlsForCurveContainer();

					WeakSequencer.Pin()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
				}

				UMovieSceneSection* Section = Track->GetAllSections().Num() ? Track->GetAllSections()[0] : nullptr;
				if (!Section)
				{
					Track->Modify();
					Section = Track->CreateTLLRN_ControlRigSection(0, TLLRN_ControlRig, true);
					//mz todo need to have multiple rigs with same class
					Track->SetTrackName(FName(*ObjectName));
					Track->SetDisplayName(FText::FromString(ObjectName));

					WeakSequencer.Pin()->EmptySelection();
					WeakSequencer.Pin()->SelectSection(Section);
					WeakSequencer.Pin()->ThrobSectionSelection();
					WeakSequencer.Pin()->ObjectImplicitlyAdded(TLLRN_ControlRig);
				}

				WeakSequencer.Pin()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
				if (!TLLRN_ControlRigEditMode)
				{
					GLevelEditorModeTools().ActivateMode(FTLLRN_ControlRigEditMode::ModeName);
					TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
				}
				if (TLLRN_ControlRigEditMode)
				{
					TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TLLRN_ControlRig, WeakSequencer.Pin());
				}
			}
		}
	});
}

const TArray<FText>& FTLLRN_ControlRigBlueprintActions::GetSubMenus() const
{
	static const TArray<FText> SubMenus
	{
		LOCTEXT("AnimTLLRN_ControlRigSubMenu", "TLLRN Control Rig")
	};
	return SubMenus;
}

#undef LOCTEXT_NAMESPACE

