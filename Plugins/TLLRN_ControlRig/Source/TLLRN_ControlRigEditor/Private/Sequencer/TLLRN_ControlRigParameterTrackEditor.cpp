// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sequencer/TLLRN_ControlRigParameterTrackEditor.h"
#include "MVVM/Extensions/ITrackExtension.h"
#include "MVVM/ViewModels/OutlinerViewModel.h"
#include "MVVM/ViewModels/TrackModel.h"
#include "MVVM/ViewModels/SectionModel.h"
#include "MVVM/ViewModels/TrackRowModel.h"
#include "MVVM/ViewModels/SequencerEditorViewModel.h"
#include "MVVM/ViewModels/ViewModelIterators.h"
#include "MVVM/ViewModels/OutlinerViewModel.h"
#include "MVVM/Selection/Selection.h"
#include "Animation/AnimMontage.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"
#include "Framework/Commands/Commands.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SSpinBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GameFramework/Actor.h"
#include "AssetRegistry/AssetData.h"
#include "Modules/ModuleManager.h"
#include "Layout/WidgetPath.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "SequencerSectionPainter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Editor.h"
#include "ISequencerChannelInterface.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "SequencerUtilities.h"
#include "ISectionLayoutBuilder.h"
#include "Styling/AppStyle.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieSceneTimeHelpers.h"
#include "Fonts/FontMeasure.h"
#include "AnimationEditorUtils.h"
#include "Misc/MessageDialog.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/Blueprint.h"
#include "TLLRN_ControlRig.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "EditorModeManager.h"
#include "Engine/Selection.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "LevelEditorViewport.h"
#include "IKeyArea.h"
#include "ISequencer.h"
#include "TLLRN_ControlRigEditorModule.h"
#include "SequencerSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IMainFrameModule.h"
#include "Channels/FloatChannelCurveModel.h"
#include "TransformNoScale.h"
#include "TLLRN_ControlRigComponent.h"
#include "ISequencerObjectChangeListener.h"
#include "MovieSceneToolHelpers.h"
#include "Rigs/FKTLLRN_ControlRig.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Units/Execution/TLLRN_RigUnit_InverseExecution.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "Exporters/AnimSeqExportOption.h"
#include "SBakeToTLLRN_ControlRigDialog.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "RigVMBlueprintGeneratedClass.h"
#include "Animation/SkeletalMeshActor.h"
#include "TimerManager.h"
#include "BakeToTLLRN_ControlRigSettings.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Toolkits/IToolkitHost.h"
#include "EditMode/TLLRN_ControlRigEditModeSettings.h"
#include "TLLRN_ControlTLLRN_RigSpaceChannelEditors.h"
#include "TransformConstraint.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/TransactionObjectEvent.h"
#include "Constraints/MovieSceneConstraintChannelHelper.h"
#include "Constraints/TLLRN_ControlTLLRN_RigTransformableHandle.h"
#include "PropertyEditorModule.h"
#include "Constraints/TransformConstraintChannelInterface.h"
#include "BakingAnimationKeySettings.h"
#include "FrameNumberDetailsCustomization.h"
#include "Editor/UnrealEd/Private/FbxExporter.h"
#include "Sequencer/TLLRN_ControlRigSequencerHelpers.h"
#include "Widgets/Layout/SSpacer.h"
#include "TLLRN_ControlRigSequencerEditorLibrary.h"
#include "LevelSequence.h"
#include "ISequencerModule.h"
#include "CurveModel.h"
#include "FrontendFilterBase.h"

#define LOCTEXT_NAMESPACE "FTLLRN_ControlRigParameterTrackEditor"

bool FTLLRN_ControlRigParameterTrackEditor::bTLLRN_ControlRigEditModeWasOpen = false;
TArray <TPair<UClass*, TArray<FName>>> FTLLRN_ControlRigParameterTrackEditor::PreviousSelectedTLLRN_ControlRigs;

TAutoConsoleVariable<bool> CVarAutoGenerateTLLRN_ControlRigTrack(TEXT("TLLRN_ControlRig.Sequencer.AutoGenerateTrack"), true, TEXT("When true automatically create control rig tracks in Sequencer when a control rig is added to a level."));

TAutoConsoleVariable<bool> CVarSelectedKeysSelectControls(TEXT("TLLRN_ControlRig.Sequencer.SelectedKeysSelectControls"), false, TEXT("When true when we select a key in Sequencer it will select the Control, by default false."));

TAutoConsoleVariable<bool> CVarSelectedSectionSetsSectionToKey(TEXT("TLLRN_ControlRig.Sequencer.SelectedSectionSetsSectionToKey"), true, TEXT("When true when we select a channel in a section, if it's the only section selected we set it as the Section To Key, by default false."));

TAutoConsoleVariable<bool> CVarEnableAdditiveTLLRN_ControlRigs(TEXT("TLLRN_ControlRig.Sequencer.EnableAdditiveTLLRN_ControlRigs"), true, TEXT("When true it is possible to add an additive control rig to a skeletal mesh component."));

static USkeletalMeshComponent* AcquireSkeletalMeshFromObject(UObject* BoundObject, TSharedPtr<ISequencer> SequencerPtr)
{
	if (AActor* Actor = Cast<AActor>(BoundObject))
	{
		if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Actor->GetRootComponent()))
		{
			return SkeletalMeshComponent;
		}

		TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
		Actor->GetComponents(SkeletalMeshComponents);

		if (SkeletalMeshComponents.Num() == 1)
		{
			return SkeletalMeshComponents[0];
		}
	}
	else if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(BoundObject))
	{
		if (SkeletalMeshComponent->GetSkeletalMeshAsset())
		{
			return SkeletalMeshComponent;
		}
	}

	return nullptr;
}


static USkeleton* GetSkeletonFromComponent(UActorComponent* InComponent)
{
	USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(InComponent);
	if (SkeletalMeshComp && SkeletalMeshComp->GetSkeletalMeshAsset() && SkeletalMeshComp->GetSkeletalMeshAsset()->GetSkeleton())
	{
		// @todo Multiple actors, multiple components
		return SkeletalMeshComp->GetSkeletalMeshAsset()->GetSkeleton();
	}

	return nullptr;
}

static USkeleton* AcquireSkeletonFromObjectGuid(const FGuid& Guid, UObject** Object, TSharedPtr<ISequencer> SequencerPtr)
{
	UObject* BoundObject = SequencerPtr.IsValid() ? SequencerPtr->FindSpawnedObjectOrTemplate(Guid) : nullptr;
	*Object = BoundObject;
	if (AActor* Actor = Cast<AActor>(BoundObject))
	{
		if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Actor->GetRootComponent()))
		{
			return GetSkeletonFromComponent(SkeletalMeshComponent);
		}

		TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
		Actor->GetComponents(SkeletalMeshComponents);
		if (SkeletalMeshComponents.Num() == 1)
		{
			return GetSkeletonFromComponent(SkeletalMeshComponents[0]);
		}
		SkeletalMeshComponents.Empty();

		AActor* ActorCDO = Cast<AActor>(Actor->GetClass()->GetDefaultObject());
		if (ActorCDO)
		{
			if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(ActorCDO->GetRootComponent()))
			{
				return GetSkeletonFromComponent(SkeletalMeshComponent);
			}

			ActorCDO->GetComponents(SkeletalMeshComponents);
			if (SkeletalMeshComponents.Num() == 1)
			{
				return GetSkeletonFromComponent(SkeletalMeshComponents[0]);
			}
			SkeletalMeshComponents.Empty();
		}

		UBlueprintGeneratedClass* ActorBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(Actor->GetClass());
		if (ActorBlueprintGeneratedClass && ActorBlueprintGeneratedClass->SimpleConstructionScript)
		{
			const TArray<USCS_Node*>& ActorBlueprintNodes = ActorBlueprintGeneratedClass->SimpleConstructionScript->GetAllNodes();

			for (USCS_Node* Node : ActorBlueprintNodes)
			{
				if (Node->ComponentClass && Node->ComponentClass->IsChildOf(USkeletalMeshComponent::StaticClass()))
				{
					if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Node->GetActualComponentTemplate(ActorBlueprintGeneratedClass)))
					{
						SkeletalMeshComponents.Add(SkeletalMeshComponent);
					}
				}
			}

			if (SkeletalMeshComponents.Num() == 1)
			{
				return GetSkeletonFromComponent(SkeletalMeshComponents[0]);
			}
		}
	}
	else if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(BoundObject))
	{
		if (USkeleton* Skeleton = GetSkeletonFromComponent(SkeletalMeshComponent))
		{
			return Skeleton;
		}
	}

	return nullptr;
}

static USkeletalMeshComponent* AcquireSkeletalMeshFromObjectGuid(const FGuid& Guid, TSharedPtr<ISequencer> SequencerPtr)
{
	UObject* BoundObject = SequencerPtr.IsValid() ? SequencerPtr->FindSpawnedObjectOrTemplate(Guid) : nullptr;
	return AcquireSkeletalMeshFromObject(BoundObject, SequencerPtr);
}

static bool DoesTLLRN_ControlRigAllowMultipleInstances(const FTopLevelAssetPath& InGeneratedClassPath)
{
	const IAssetRegistry& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	
	const FString BlueprintPath = InGeneratedClassPath.ToString().LeftChop(2); // Chop off _C
	const FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(BlueprintPath));

	if (AssetData.GetTagValueRef<bool>(GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, bAllowMultipleInstances)))
	{
		return true;
	}

	return false;
}


class FFrontendFilter_TLLRN_ControlRigFilterByAssetTag: public FFrontendFilter
{
public:
	FFrontendFilter_TLLRN_ControlRigFilterByAssetTag(TSharedPtr<FFrontendFilterCategory> InCategory, const FRigVMTag& InTag) :
		FFrontendFilter(InCategory),
		Tag(InTag)
	{}

	FString GetName() const override { return Tag.bMarksSubjectAsInvalid ? TEXT("Exlude ") + Tag.Name.ToString() : Tag.Name.ToString(); }
	FText GetDisplayName() const override { return FText::FromString(Tag.bMarksSubjectAsInvalid ? TEXT("Exlude ") + Tag.GetLabel() : Tag.GetLabel()); }
	FText GetToolTipText() const override { return Tag.ToolTip; }
	FLinearColor GetColor() const override { return Tag.Color; };
	
	bool PassesFilter(const FContentBrowserItem& InItem) const override
	{
		// FAssetData AssetData;
		// if (InItem.Legacy_TryGetAssetData(AssetData))
		// {
		// 	static const FName AssetVariantPropertyName = GET_MEMBER_NAME_CHECKED(URigVMBlueprint, AssetVariant);
		// 	const FProperty* AssetVariantProperty = CastField<FProperty>(URigVMBlueprint::StaticClass()->FindPropertyByName(AssetVariantPropertyName));
		// 	const FString VariantStr = AssetData.GetTagValueRef<FString>(AssetVariantPropertyName);
		// 	if(!VariantStr.IsEmpty())
		// 	{
		// 		FRigVMVariant AssetVariant;
		// 		AssetVariantProperty->ImportText_Direct(*VariantStr, &AssetVariant, nullptr, EPropertyPortFlags::PPF_None);

		// 		if (Tag.bMarksSubjectAsInvalid)
		// 		{
		// 			if (!AssetVariant.Tags.Contains(Tag))
		// 			{
		// 				return true;
		// 			}
		// 		}
		// 		else
		// 		{
		// 			if (AssetVariant.Tags.Contains(Tag))
		// 			{
		// 				return true;
		// 			}	
		// 		}
		// 	}
		// 	else
		// 	{
		// 		if (Tag.bMarksSubjectAsInvalid)
		// 		{
		// 			return true;
		// 		}
		// 	}
		// }
		return false;
	}

private:
	FRigVMTag Tag;
};

bool FTLLRN_ControlRigParameterTrackEditor::bAutoGenerateTLLRN_ControlRigTrack = true;
FCriticalSection FTLLRN_ControlRigParameterTrackEditor::ControlUndoTransactionMutex;

FTLLRN_ControlRigParameterTrackEditor::FTLLRN_ControlRigParameterTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FKeyframeTrackEditor<UMovieSceneTLLRN_ControlRigParameterTrack>(InSequencer)
	, bCurveDisplayTickIsPending(false)
	, bIsDoingSelection(false)
	, bSkipNextSelectionFromTimer(false)
	, bIsLayeredTLLRN_ControlRig(false)
	, bFilterAssetBySkeleton(true)
	, bFilterAssetByAnimatableControls(false)
	, ControlUndoBracket(0)
	, ControlChangedDuringUndoBracket(0)
{
	FMovieSceneToolsModule::Get().RegisterAnimationBakeHelper(this);

	if (GEditor != nullptr)
	{
		GEditor->RegisterForUndo(this);
	}
}

void FTLLRN_ControlRigParameterTrackEditor::OnInitialize()
{
	if (GetSequencer().IsValid() == false)
	{
		return;
	}
	ISequencer* SequencerPtr = GetSequencer().Get();

	UMovieScene* MovieScene = SequencerPtr->GetFocusedMovieSceneSequence()->GetMovieScene();
	SelectionChangedHandle = SequencerPtr->GetSelectionChangedTracks().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::OnSelectionChanged);
	SequencerChangedHandle = SequencerPtr->OnMovieSceneDataChanged().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::OnSequencerDataChanged);
	OnActivateSequenceChangedHandle = SequencerPtr->OnActivateSequence().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::OnActivateSequenceChanged);
	OnChannelChangedHandle = SequencerPtr->OnChannelChanged().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::OnChannelChanged);
	OnMovieSceneChannelChangedHandle = MovieScene->OnChannelChanged().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::OnChannelChanged);
	OnActorAddedToSequencerHandle = SequencerPtr->OnActorAddedToSequencer().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::HandleActorAdded);

	{
		//we check for two things, one if the control rig has been replaced if so we need to switch.
		//the other is if bound object on the edit mode is null we request a re-evaluate which will reset it up.
		const FDelegateHandle OnObjectsReplacedHandle = FCoreUObjectDelegates::OnObjectsReplaced.AddLambda([this](const TMap<UObject*, UObject*>& ReplacementMap)
		{
			if (GetSequencer().IsValid() && GetSequencer()->GetFocusedMovieSceneSequence() && GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene())
			{
				static bool bHasEnteredSilent = false;
				
				bool bRequestEvaluate = false;
				TMap<UTLLRN_ControlRig*, UTLLRN_ControlRig*> OldToNewTLLRN_ControlRigs;
				FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = GetEditMode();
				if (TLLRN_ControlRigEditMode)
				{
					UMovieScene* MovieScene = GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene();
					TArray<FString>& MuteNodes = MovieScene->GetMuteNodes();

					TArrayView<TWeakObjectPtr<UTLLRN_ControlRig>> TLLRN_ControlRigs = TLLRN_ControlRigEditMode->GetTLLRN_ControlRigs();
					for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRigPtr : TLLRN_ControlRigs)
					{
						if (UTLLRN_ControlRig* TLLRN_ControlRig = TLLRN_ControlRigPtr.Get())
						{
							if (TLLRN_ControlRig->GetObjectBinding() && TLLRN_ControlRig->GetObjectBinding()->GetBoundObject() == nullptr)
							{
								IterateTracks([this, TLLRN_ControlRig, &MuteNodes, &bRequestEvaluate](UMovieSceneTLLRN_ControlRigParameterTrack* Track)
									{
										if (TLLRN_ControlRig == Track->GetTLLRN_ControlRig())
										{
											if (MuteNodes.Find(Track->GetName()) != INDEX_NONE) //only re-evalute if  not muted, TODO, need function to test to see if a track is evaluating at a certain time
											{
												bRequestEvaluate = true;
											}
										}
										return false;
									});
								if (bRequestEvaluate == true)
								{
									break;
								}
							}
						}
					}
				}
				//Reset Bindings for replaced objects.
				for (TPair<UObject*, UObject*> ReplacedObject : ReplacementMap)
				{
					if (UTLLRN_ControlRigComponent* OldTLLRN_ControlRigComponent = Cast<UTLLRN_ControlRigComponent>(ReplacedObject.Key))
					{
						UTLLRN_ControlRigComponent* NewTLLRN_ControlRigComponent = Cast<UTLLRN_ControlRigComponent>(ReplacedObject.Value);
						if (OldTLLRN_ControlRigComponent->GetTLLRN_ControlRig())
						{
							UTLLRN_ControlRig* NewTLLRN_ControlRig = nullptr;
							if (NewTLLRN_ControlRigComponent)
							{
								NewTLLRN_ControlRig = NewTLLRN_ControlRigComponent->GetTLLRN_ControlRig();
							}
							OldToNewTLLRN_ControlRigs.Emplace(OldTLLRN_ControlRigComponent->GetTLLRN_ControlRig(), NewTLLRN_ControlRig);
						}
					}
					else if (UTLLRN_ControlRig* OldTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(ReplacedObject.Key))
					{
						UTLLRN_ControlRig* NewTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(ReplacedObject.Value);
						OldToNewTLLRN_ControlRigs.Emplace(OldTLLRN_ControlRig, NewTLLRN_ControlRig);
					}
				}
				if (OldToNewTLLRN_ControlRigs.Num() > 0)
				{
					//need to avoid any evaluations when doing this replacement otherwise we will evaluate sequencer
					if (bHasEnteredSilent == false)
					{
						GetSequencer()->EnterSilentMode();
						bHasEnteredSilent = true;
					}

					IterateTracks([this, &OldToNewTLLRN_ControlRigs, TLLRN_ControlRigEditMode, &bRequestEvaluate](UMovieSceneTLLRN_ControlRigParameterTrack* Track)
					{
						if (UTLLRN_ControlRig* OldTLLRN_ControlRig = Track->GetTLLRN_ControlRig())
						{
							UTLLRN_ControlRig** NewTLLRN_ControlRig = OldToNewTLLRN_ControlRigs.Find(OldTLLRN_ControlRig);
							if (NewTLLRN_ControlRig)
							{
								TArray<FName> SelectedControls = OldTLLRN_ControlRig->CurrentControlSelection();
								OldTLLRN_ControlRig->ClearControlSelection();
								UnbindTLLRN_ControlRig(OldTLLRN_ControlRig);
								if (*NewTLLRN_ControlRig)
								{
									Track->Modify();
									Track->ReplaceTLLRN_ControlRig(*NewTLLRN_ControlRig, OldTLLRN_ControlRig->GetClass() != (*NewTLLRN_ControlRig)->GetClass());
									BindTLLRN_ControlRig(*NewTLLRN_ControlRig);
									bRequestEvaluate = true;
								}
								else
								{
									Track->ReplaceTLLRN_ControlRig(nullptr, true);
								}
								if (TLLRN_ControlRigEditMode)
								{
									TLLRN_ControlRigEditMode->ReplaceTLLRN_ControlRig(OldTLLRN_ControlRig, *NewTLLRN_ControlRig);

									UTLLRN_ControlRig* PtrNewTLLRN_ControlRig = *NewTLLRN_ControlRig;

									auto UpdateSelectionDelegate = [this, SelectedControls, PtrNewTLLRN_ControlRig, bRequestEvaluate]()
									{

										if (!(FSlateApplication::Get().HasAnyMouseCaptor() || GUnrealEd->IsUserInteracting()))
										{
											TGuardValue<bool> Guard(bIsDoingSelection, true);
											GetSequencer()->ExternalSelectionHasChanged();
											if (PtrNewTLLRN_ControlRig)
											{
												GEditor->GetTimerManager()->SetTimerForNextTick([this, SelectedControls, PtrNewTLLRN_ControlRig, bRequestEvaluate]()
													{
														PtrNewTLLRN_ControlRig->ClearControlSelection();
														for (const FName& ControlName : SelectedControls)
														{
															PtrNewTLLRN_ControlRig->SelectControl(ControlName, true);
														}
													});
											}
											
											if (bHasEnteredSilent == true)
											{
												GetSequencer()->ExitSilentMode();
												bHasEnteredSilent = false;
											}
											
											if (bRequestEvaluate)
											{
												GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
											}
											if (UpdateSelectionTimerHandle.IsValid())
											{
												GEditor->GetTimerManager()->ClearTimer(UpdateSelectionTimerHandle);
											}
										}

									};

									GEditor->GetTimerManager()->SetTimer(UpdateSelectionTimerHandle, UpdateSelectionDelegate, 0.01f, true);
								}
							}
						}

						return false;
					});

					if (!TLLRN_ControlRigEditMode)
					{
						if (bRequestEvaluate)
						{
							GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
						}
					}
				}

				if (TLLRN_ControlRigEditMode && bRequestEvaluate)
				{
					GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
				}

				// ensure we exit silent mode if it has been entered
				if (bHasEnteredSilent)
				{
					GetSequencer()->ExitSilentMode();
					bHasEnteredSilent = false;
				}
			}
		});
	

		AcquiredResources.Add([=] { FCoreUObjectDelegates::OnObjectsReplaced.Remove(OnObjectsReplacedHandle); });
	}

	// Register all modified/selections for control rigs
	IterateTracks([this](UMovieSceneTLLRN_ControlRigParameterTrack* Track)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Track->GetTLLRN_ControlRig())
		{
			BindTLLRN_ControlRig(TLLRN_ControlRig);
		}

		// Mark layered mode on track color and display name 
		UTLLRN_ControlRigSequencerEditorLibrary::MarkLayeredModeOnTrackDisplay(Track);

		return false;
	});
}

FTLLRN_ControlRigParameterTrackEditor::~FTLLRN_ControlRigParameterTrackEditor()
{
	if (GEditor != nullptr)
	{
		GEditor->UnregisterForUndo(this);
	}

	FMovieSceneToolsModule::Get().UnregisterAnimationBakeHelper(this);
}


void FTLLRN_ControlRigParameterTrackEditor::BindTLLRN_ControlRig(UTLLRN_ControlRig* TLLRN_ControlRig)
{
	if (TLLRN_ControlRig && BoundTLLRN_ControlRigs.Contains(TLLRN_ControlRig) == false)
	{
		TLLRN_ControlRig->ControlModified().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::HandleControlModified);
		TLLRN_ControlRig->OnPostConstruction_AnyThread().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::HandleOnPostConstructed);
		TLLRN_ControlRig->ControlSelected().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::HandleControlSelected);
		TLLRN_ControlRig->ControlUndoBracket().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::HandleControlUndoBracket);
		TLLRN_ControlRig->TLLRN_ControlRigBound().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::HandleOnTLLRN_ControlRigBound);
		
		BoundTLLRN_ControlRigs.Add(TLLRN_ControlRig);
		UMovieSceneTLLRN_ControlRigParameterTrack* Track = FindTrack(TLLRN_ControlRig);
		if (Track)
		{
			for (UMovieSceneSection* BaseSection : Track->GetAllSections())
			{
				if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast< UMovieSceneTLLRN_ControlRigParameterSection>(BaseSection))
				{
					if (Section->GetTLLRN_ControlRig())
					{
						TArray<FTLLRN_SpaceControlNameAndChannel>& SpaceChannels = Section->GetSpaceChannels();
						for (FTLLRN_SpaceControlNameAndChannel& Channel : SpaceChannels)
						{
							HandleOnSpaceAdded(Section, Channel.ControlName, &(Channel.SpaceCurve));
						}
						
						TArray<FConstraintAndActiveChannel>& ConstraintChannels = Section->GetConstraintsChannels();
						for (FConstraintAndActiveChannel& Channel: ConstraintChannels)
						{ 
							HandleOnConstraintAdded(Section, &(Channel.ActiveChannel));
						}
					}
				}
			}
			Track->SpaceChannelAdded().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::HandleOnSpaceAdded);
			Track->ConstraintChannelAdded().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::HandleOnConstraintAdded);
		}
	}
}
void FTLLRN_ControlRigParameterTrackEditor::UnbindTLLRN_ControlRig(UTLLRN_ControlRig* TLLRN_ControlRig)
{
	if (TLLRN_ControlRig && BoundTLLRN_ControlRigs.Contains(TLLRN_ControlRig) == true)
	{
		UMovieSceneTLLRN_ControlRigParameterTrack* Track = FindTrack(TLLRN_ControlRig);
		if (Track)
		{
			Track->SpaceChannelAdded().RemoveAll(this);
			Track->ConstraintChannelAdded().RemoveAll(this);
		}
		TLLRN_ControlRig->ControlModified().RemoveAll(this);
		TLLRN_ControlRig->OnPostConstruction_AnyThread().RemoveAll(this);
		TLLRN_ControlRig->ControlSelected().RemoveAll(this);
		if (const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = TLLRN_ControlRig->GetObjectBinding())
		{
			Binding->OnTLLRN_ControlRigBind().RemoveAll(this);
		}
		TLLRN_ControlRig->ControlUndoBracket().RemoveAll(this);
		TLLRN_ControlRig->TLLRN_ControlRigBound().RemoveAll(this);
		
		BoundTLLRN_ControlRigs.Remove(TLLRN_ControlRig);
		ClearOutAllSpaceAndConstraintDelegates(TLLRN_ControlRig);
	}
}
void FTLLRN_ControlRigParameterTrackEditor::UnbindAllTLLRN_ControlRigs()
{
	ClearOutAllSpaceAndConstraintDelegates();
	TArray<TWeakObjectPtr<UTLLRN_ControlRig>> TLLRN_ControlRigs = BoundTLLRN_ControlRigs;
	for (TWeakObjectPtr<UTLLRN_ControlRig>& ObjectPtr : TLLRN_ControlRigs)
	{
		if (ObjectPtr.IsValid())
		{
			UTLLRN_ControlRig* TLLRN_ControlRig = ObjectPtr.Get();
			UnbindTLLRN_ControlRig(TLLRN_ControlRig);
		}
	}
	BoundTLLRN_ControlRigs.SetNum(0);
}


void FTLLRN_ControlRigParameterTrackEditor::ObjectImplicitlyAdded(UObject* InObject)
{
	UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(InObject);
	if (TLLRN_ControlRig)
	{
		BindTLLRN_ControlRig(TLLRN_ControlRig);
	}
}

void FTLLRN_ControlRigParameterTrackEditor::ObjectImplicitlyRemoved(UObject* InObject)
{
	UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(InObject);
	if (TLLRN_ControlRig)
	{
		UnbindTLLRN_ControlRig(TLLRN_ControlRig);
	}
}

void FTLLRN_ControlRigParameterTrackEditor::OnRelease()
{
	UWorld* World = GCurrentLevelEditingViewportClient ? GCurrentLevelEditingViewportClient->GetWorld() : nullptr;
	FConstraintsManagerController& Controller = FConstraintsManagerController::Get(World);
	for (FDelegateHandle& Handle : ConstraintHandlesToClear)
	{
		if (Handle.IsValid())
		{
			Controller.GetNotifyDelegate().Remove(Handle);
		}
	}
	ConstraintHandlesToClear.Reset();

	FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = GetEditMode();
	PreviousSelectedTLLRN_ControlRigs.Reset();
	if (TLLRN_ControlRigEditMode)
	{
		bTLLRN_ControlRigEditModeWasOpen = true;
		for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : BoundTLLRN_ControlRigs)
		{
			if (TLLRN_ControlRig.IsValid())
			{
				TPair<UClass*, TArray<FName>> ClassAndName;
				ClassAndName.Key = TLLRN_ControlRig->GetClass();
				ClassAndName.Value = TLLRN_ControlRig->CurrentControlSelection();
				PreviousSelectedTLLRN_ControlRigs.Add(ClassAndName);
			}
		}
		TLLRN_ControlRigEditMode->Exit(); //deactive mode below doesn't exit for some reason so need to make sure things are cleaned up
		if (FEditorModeTools* Tools = GetEditorModeTools())
		{
			Tools->DeactivateMode(FTLLRN_ControlRigEditMode::ModeName);
		}

		TLLRN_ControlRigEditMode->SetObjects(nullptr, nullptr, GetSequencer());
	}
	else
	{
		bTLLRN_ControlRigEditModeWasOpen = false;
	}
	UnbindAllTLLRN_ControlRigs();
	if (GetSequencer().IsValid())
	{
		if (SelectionChangedHandle.IsValid())
		{
			GetSequencer()->GetSelectionChangedTracks().Remove(SelectionChangedHandle);
		}
		if (SequencerChangedHandle.IsValid())
		{
			GetSequencer()->OnMovieSceneDataChanged().Remove(SequencerChangedHandle);
		}
		if (OnActivateSequenceChangedHandle.IsValid())
		{
			GetSequencer()->OnActivateSequence().Remove(OnActivateSequenceChangedHandle);
		}
		if (CurveChangedHandle.IsValid())
		{
			GetSequencer()->GetCurveDisplayChanged().Remove(CurveChangedHandle);
		}
		if (OnActorAddedToSequencerHandle.IsValid())
		{
			GetSequencer()->OnActorAddedToSequencer().Remove(OnActorAddedToSequencerHandle);
		}
		if (OnChannelChangedHandle.IsValid())
		{
			GetSequencer()->OnChannelChanged().Remove(OnChannelChangedHandle);
		}

		if (GetSequencer()->GetFocusedMovieSceneSequence() && GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene())
		{
			UMovieScene* MovieScene = GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene();
			if (OnMovieSceneChannelChangedHandle.IsValid())
			{
				MovieScene->OnChannelChanged().Remove(OnMovieSceneChannelChangedHandle);
			}
		}
	}
	AcquiredResources.Release();

}

TSharedRef<ISequencerTrackEditor> FTLLRN_ControlRigParameterTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FTLLRN_ControlRigParameterTrackEditor(InSequencer));
}

bool FTLLRN_ControlRigParameterTrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
	ETrackSupport TrackSupported = InSequence ? InSequence->IsTrackSupported(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass()) : ETrackSupport::Default;

	if (TrackSupported == ETrackSupport::NotSupported)
	{
		return false;
	}

	return (InSequence && InSequence->IsA(ULevelSequence::StaticClass())) || TrackSupported == ETrackSupport::Supported;
}

bool FTLLRN_ControlRigParameterTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return Type == UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass();
}


TSharedRef<ISequencerSection> FTLLRN_ControlRigParameterTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	check(SupportsType(SectionObject.GetOuter()->GetClass()));

	return MakeShareable(new FTLLRN_ControlRigParameterSection(SectionObject, GetSequencer()));
}

void FTLLRN_ControlRigParameterTrackEditor::BuildObjectBindingContextMenu(FMenuBuilder& MenuBuilder, const TArray<FGuid>& ObjectBindings, const UClass* ObjectClass)
{
	if(!ObjectClass)
	{
		return;
	}
	
	if (ObjectClass->IsChildOf(USkeletalMeshComponent::StaticClass()) ||
		ObjectClass->IsChildOf(AActor::StaticClass()) ||
		ObjectClass->IsChildOf(UChildActorComponent::StaticClass()))
	{
		const TSharedPtr<ISequencer> ParentSequencer = GetSequencer();
		UObject* BoundObject = nullptr;
		USkeleton* Skeleton = AcquireSkeletonFromObjectGuid(ObjectBindings[0], &BoundObject, ParentSequencer);
		USkeletalMeshComponent* SkelMeshComp = AcquireSkeletalMeshFromObject(BoundObject, ParentSequencer);

		if (Skeleton && SkelMeshComp)
		{
			MenuBuilder.BeginSection("TLLRN Control Rig", LOCTEXT("TLLRN_ControlRig", "TLLRN Control Rig"));
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("FilterAssetBySkeleton", "Filter Asset By Skeleton"),
					LOCTEXT("FilterAssetBySkeletonTooltip", "Filters Control Rig assets to match current skeleton"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSP(this, &FTLLRN_ControlRigParameterTrackEditor::ToggleFilterAssetBySkeleton),
						FCanExecuteAction(),
						FIsActionChecked::CreateSP(this, &FTLLRN_ControlRigParameterTrackEditor::IsToggleFilterAssetBySkeleton)
					),
					NAME_None,
					EUserInterfaceActionType::ToggleButton);

				MenuBuilder.AddSubMenu(
					LOCTEXT("BakeToTLLRN_ControlRig", "Bake To Control Rig"),
					LOCTEXT("BakeToTLLRN_ControlRigTooltip", "Bake to an invertible Control Rig that matches this skeleton"),
					FNewMenuDelegate::CreateRaw(this, &FTLLRN_ControlRigParameterTrackEditor::BakeToTLLRN_ControlRigSubMenu, ObjectBindings[0], BoundObject, SkelMeshComp, Skeleton)
				);
			}
			MenuBuilder.EndSection();
		}
	}
}

static bool ClassViewerSortPredicate(const FClassViewerSortElementInfo& A, const  FClassViewerSortElementInfo& B)
{
	if ((A.Class == UFKTLLRN_ControlRig::StaticClass() && B.Class == UFKTLLRN_ControlRig::StaticClass()) ||
				(A.Class != UFKTLLRN_ControlRig::StaticClass() && B.Class != UFKTLLRN_ControlRig::StaticClass()))
	{
		return  (*A.DisplayName).Compare(*B.DisplayName, ESearchCase::IgnoreCase) < 0;
	}
	else
	{
		return A.Class == UFKTLLRN_ControlRig::StaticClass();
	}
}

/** Filter class does not allow classes that already exist in a skeletal mesh component. */
class FClassViewerHideAlreadyAddedRigsFilter : public IClassViewerFilter
{
public:
	FClassViewerHideAlreadyAddedRigsFilter(const TArray<UClass*> InExistingClasses)
		: AlreadyAddedRigs(InExistingClasses)
	{}

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs ) override
	{
		return IsClassAllowed_Internal(InClass->GetClassPathName());
	}
	
	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions,
										const TSharedRef< const class IUnloadedBlueprintData > InUnloadedClassData,
										TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return IsClassAllowed_Internal(InUnloadedClassData.Get().GetClassPathName());
	}
	
private:
	bool IsClassAllowed_Internal(const FTopLevelAssetPath& InGeneratedClassPath)
	{
		if (DoesTLLRN_ControlRigAllowMultipleInstances(InGeneratedClassPath))
		{
			return true;
		}
		
		return !AlreadyAddedRigs.ContainsByPredicate([InGeneratedClassPath](const UClass* Class)
			{
				return InGeneratedClassPath == Class->GetClassPathName();
			});
	}

	
	TArray<UClass*> AlreadyAddedRigs;
};

void FTLLRN_ControlRigParameterTrackEditor::BakeToTLLRN_ControlRigSubMenu(FMenuBuilder& MenuBuilder, FGuid ObjectBinding, UObject* BoundObject, USkeletalMeshComponent* SkelMeshComp, USkeleton* Skeleton)
{
	const TSharedPtr<ISequencer> ParentSequencer = GetSequencer();

	if (Skeleton)
	{
		FClassViewerInitializationOptions Options;
		Options.bShowUnloadedBlueprints = true;
		Options.NameTypeToDisplay = EClassViewerNameTypeToDisplay::DisplayName;
		const TSharedPtr<FTLLRN_ControlRigClassFilter> ClassFilter = MakeShareable(new FTLLRN_ControlRigClassFilter(bFilterAssetBySkeleton, false, true, Skeleton));
		Options.ClassFilters.Add(ClassFilter.ToSharedRef());
		Options.bShowNoneOption = false;
		Options.ExtraPickerCommonClasses.Add(UFKTLLRN_ControlRig::StaticClass());
		Options.ClassViewerSortPredicate = ClassViewerSortPredicate;

		FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

		const TSharedRef<SWidget> ClassViewer = ClassViewerModule.CreateClassViewer(Options, FOnClassPicked::CreateRaw(this, &FTLLRN_ControlRigParameterTrackEditor::BakeToTLLRN_ControlRig, ObjectBinding, BoundObject, SkelMeshComp, Skeleton));
		MenuBuilder.AddWidget(ClassViewer, FText::GetEmpty(), true);
	}
}

class SBakeToAnimAndTLLRN_ControlRigOptionsWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBakeToAnimAndTLLRN_ControlRigOptionsWindow)
		: _ExportOptions(nullptr), _BakeSettings(nullptr)
		, _WidgetWindow()
	{}

	SLATE_ARGUMENT(UAnimSeqExportOption*, ExportOptions)
		SLATE_ARGUMENT(UBakeToTLLRN_ControlRigSettings*, BakeSettings)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
		SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }

	FReply OnExport()
	{
		bShouldExport = true;
		if (WidgetWindow.IsValid())
		{
			WidgetWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}


	FReply OnCancel()
	{
		bShouldExport = false;
		if (WidgetWindow.IsValid())
		{
			WidgetWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			return OnCancel();
		}

		return FReply::Unhandled();
	}

	bool ShouldExport() const
	{
		return bShouldExport;
	}


	SBakeToAnimAndTLLRN_ControlRigOptionsWindow()
		: ExportOptions(nullptr)
		, BakeSettings(nullptr)
		, bShouldExport(false)
	{}

private:

	FReply OnResetToDefaultClick() const;

private:
	UAnimSeqExportOption* ExportOptions;
	UBakeToTLLRN_ControlRigSettings* BakeSettings;
	TSharedPtr<class IDetailsView> DetailsView;
	TSharedPtr<class IDetailsView> DetailsView2;
	TWeakPtr< SWindow > WidgetWindow;
	bool			bShouldExport;
};


void SBakeToAnimAndTLLRN_ControlRigOptionsWindow::Construct(const FArguments& InArgs)
{
	ExportOptions = InArgs._ExportOptions;
	BakeSettings = InArgs._BakeSettings;
	WidgetWindow = InArgs._WidgetWindow;

	check(ExportOptions);

	FText CancelText = LOCTEXT("AnimSequenceOptions_Cancel", "Cancel");
	FText CancelTooltipText = LOCTEXT("AnimSequenceOptions_Cancel_ToolTip", "Cancel control rig creation");

	TSharedPtr<SBox> HeaderToolBox;
	TSharedPtr<SHorizontalBox> AnimHeaderButtons;
	TSharedPtr<SBox> InspectorBox;
	TSharedPtr<SBox> InspectorBox2;
	this->ChildSlot
		[
			SNew(SBox)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SAssignNew(HeaderToolBox, SBox)
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Font(FAppStyle::GetFontStyle("CurveEd.LabelFont"))
		.Text(LOCTEXT("Export_CurrentFileTitle", "Current File: "))
		]
		]
		]
	+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(2)
		[
			SAssignNew(InspectorBox, SBox)
		]
	+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(2)
		[
			SAssignNew(InspectorBox2, SBox)
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(2)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(2)
		+ SUniformGridPanel::Slot(1, 0)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
		.Text(LOCTEXT("Create", "Create"))
		.OnClicked(this, &SBakeToAnimAndTLLRN_ControlRigOptionsWindow::OnExport)
		]
	+ SUniformGridPanel::Slot(2, 0)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
		.Text(CancelText)
		.ToolTipText(CancelTooltipText)
		.OnClicked(this, &SBakeToAnimAndTLLRN_ControlRigOptionsWindow::OnCancel)
		]
		]
			]
		];

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView2 = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	InspectorBox->SetContent(DetailsView->AsShared());
	InspectorBox2->SetContent(DetailsView2->AsShared());
	HeaderToolBox->SetContent(
		SNew(SBorder)
		.Padding(FMargin(3))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		[
			SNew(SBox)
			.HAlign(HAlign_Right)
		[
			SAssignNew(AnimHeaderButtons, SHorizontalBox)
			+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(2.0f, 0.0f))
		[
			SNew(SButton)
			.Text(LOCTEXT("AnimSequenceOptions_ResetOptions", "Reset to Default"))
		.OnClicked(this, &SBakeToAnimAndTLLRN_ControlRigOptionsWindow::OnResetToDefaultClick)
		]
		]
		]
		]
	);

	DetailsView->SetObject(ExportOptions);
	DetailsView2->SetObject(BakeSettings);
}

FReply SBakeToAnimAndTLLRN_ControlRigOptionsWindow::OnResetToDefaultClick() const
{
	if (ExportOptions)
	{
		ExportOptions->ResetToDefault();
		//Refresh the view to make sure the custom UI are updating correctly
		DetailsView->SetObject(ExportOptions, true);
	}

	if (BakeSettings)
	{
		BakeSettings->Reset();
		DetailsView2->SetObject(BakeSettings, true);
	}
	
	return FReply::Handled();
}

void FTLLRN_ControlRigParameterTrackEditor::SmartReduce(TSharedPtr<ISequencer>& InSequencer, const FSmartReduceParams& InParams, UMovieSceneTLLRN_ControlRigParameterSection * ParamSection)
{
	using namespace UE::Sequencer;

	if (ParamSection)
	{
		ISequencerModule& SequencerModule = FModuleManager::LoadModuleChecked<ISequencerModule>("Sequencer");
		const bool bNeedToTestExisting = false;
		TOptional<FKeyHandleSet> KeyHandleSet;

		FMovieSceneChannelProxy& ChannelProxy = ParamSection->GetChannelProxy();
		for (const FMovieSceneChannelEntry& Entry : ParamSection->GetChannelProxy().GetAllEntries())
		{
			const FName ChannelTypeName = Entry.GetChannelTypeName();
			TArrayView<FMovieSceneChannel* const> Channels = Entry.GetChannels();
			for (int32 Index = 0; Index < Channels.Num(); ++Index)
			{
				FMovieSceneChannelHandle ChannelHandle = ChannelProxy.MakeHandle(ChannelTypeName, Index);
				ISequencerChannelInterface* EditorInterface = SequencerModule.FindChannelEditorInterface(ChannelHandle.GetChannelTypeName());

				FCreateCurveEditorModelParams CurveModelParams{ ParamSection, ParamSection, InSequencer.ToSharedRef() };
				if (TUniquePtr<FCurveModel> CurveModel = EditorInterface->CreateCurveEditorModel_Raw(ChannelHandle, CurveModelParams))
				{
					FKeyHandleSet OutHandleSet;
					UCurveEditorSmartReduceFilter::SmartReduce(CurveModel.Get(), InParams, KeyHandleSet, bNeedToTestExisting, OutHandleSet);
				}
			}
		}
	}
}

bool FTLLRN_ControlRigParameterTrackEditor::LoadAnimationIntoSection(TSharedPtr<ISequencer>& SequencerPtr, UAnimSequence* AnimSequence, USkeletalMeshComponent* SkelMeshComp,
	FFrameNumber StartFrame, bool bReduceKeys, const FSmartReduceParams& ReduceParams, bool bResetControls, UMovieSceneTLLRN_ControlRigParameterSection* ParamSection)
{
	EMovieSceneKeyInterpolation DefaultInterpolation = SequencerPtr->GetKeyInterpolation();
	UMovieSceneSequence* OwnerSequence = SequencerPtr->GetFocusedMovieSceneSequence();
	UMovieScene* OwnerMovieScene = OwnerSequence->GetMovieScene();
	if (ParamSection->LoadAnimSequenceIntoThisSection(AnimSequence, FFrameNumber(0), OwnerMovieScene, SkelMeshComp,
		false, 0.0, bResetControls, StartFrame, DefaultInterpolation))
	{
		if (bReduceKeys)
		{
			SmartReduce(SequencerPtr, ReduceParams, ParamSection);
		}
		return true;
	}
	return false;
}

void FTLLRN_ControlRigParameterTrackEditor::BakeToTLLRN_ControlRig(UClass* InClass, FGuid ObjectBinding, UObject* BoundActor, USkeletalMeshComponent* SkelMeshComp, USkeleton* Skeleton)
{
	FSlateApplication::Get().DismissAllMenus();
	const TSharedPtr<ISequencer> SequencerParent = GetSequencer();

	if (InClass && InClass->IsChildOf(UTLLRN_ControlRig::StaticClass()) && SequencerParent.IsValid())
	{

		UMovieSceneSequence* OwnerSequence = GetSequencer()->GetFocusedMovieSceneSequence();
		UMovieSceneSequence* RootSequence = GetSequencer()->GetRootMovieSceneSequence();
		UMovieScene* OwnerMovieScene = OwnerSequence->GetMovieScene();
		{
			UAnimSequence* TempAnimSequence = NewObject<UAnimSequence>(GetTransientPackage(), NAME_None);
			TempAnimSequence->SetSkeleton(Skeleton);
			const TSharedPtr<ISequencer> ParentSequencer = GetSequencer();
			FMovieSceneSequenceIDRef Template = ParentSequencer->GetFocusedTemplateID();
			FMovieSceneSequenceTransform RootToLocalTransform = ParentSequencer->GetFocusedMovieSceneSequenceTransform();
			UAnimSeqExportOption* AnimSeqExportOption = NewObject<UAnimSeqExportOption>(GetTransientPackage(), NAME_None);
			UBakeToTLLRN_ControlRigSettings* BakeSettings = GetMutableDefault<UBakeToTLLRN_ControlRigSettings>();
			AnimSeqExportOption->bTransactRecording = false;
			AnimSeqExportOption->CustomDisplayRate = ParentSequencer->GetFocusedDisplayRate();

			TSharedPtr<SWindow> ParentWindow;
			if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
			{
				IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
				ParentWindow = MainFrame.GetParentWindow();
			}

			TSharedRef<SWindow> Window = SNew(SWindow)
				.Title(LOCTEXT("AnimSeqTitle", "Options For Baking"))
				.SizingRule(ESizingRule::UserSized)
				.AutoCenter(EAutoCenter::PrimaryWorkArea)
				.ClientSize(FVector2D(500, 445));

			TSharedPtr<SBakeToAnimAndTLLRN_ControlRigOptionsWindow> OptionWindow;
			Window->SetContent
			(
				SAssignNew(OptionWindow, SBakeToAnimAndTLLRN_ControlRigOptionsWindow)
				.ExportOptions(AnimSeqExportOption)
				.BakeSettings(BakeSettings)
				.WidgetWindow(Window)
			);

			FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

			if (OptionWindow.Get()->ShouldExport())
			{
				FAnimExportSequenceParameters AESP;
				AESP.Player = ParentSequencer.Get();
				AESP.RootToLocalTransform = RootToLocalTransform;
				AESP.MovieSceneSequence = OwnerSequence;
				AESP.RootMovieSceneSequence = RootSequence;
				bool bResult = MovieSceneToolHelpers::ExportToAnimSequence(TempAnimSequence, AnimSeqExportOption, AESP, SkelMeshComp);
				if (bResult == false)
				{
					TempAnimSequence->MarkAsGarbage();
					AnimSeqExportOption->MarkAsGarbage();
					return;
				}

				const FScopedTransaction Transaction(LOCTEXT("BakeToTLLRN_ControlRig_Transaction", "Bake To Control Rig"));
				
				bool bReuseTLLRN_ControlRig = false; //if same Class just re-use it, and put into a new section
				OwnerMovieScene->Modify();
				UMovieSceneTLLRN_ControlRigParameterTrack* Track = OwnerMovieScene->FindTrack<UMovieSceneTLLRN_ControlRigParameterTrack>(ObjectBinding);
				if (Track)
				{
					if (Track->GetTLLRN_ControlRig() && Track->GetTLLRN_ControlRig()->GetClass() == InClass)
					{
						bReuseTLLRN_ControlRig = true;
					}
					Track->Modify();
					Track->RemoveAllAnimationData();//removes all sections and sectiontokey
				}
				else
				{
					Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(AddTrack(OwnerMovieScene, ObjectBinding, UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), NAME_None));
					if (Track)
					{
						Track->Modify();
					}
				}

				if (Track)
				{

					FString ObjectName = InClass->GetName();
					ObjectName.RemoveFromEnd(TEXT("_C"));
					UTLLRN_ControlRig* TLLRN_ControlRig = bReuseTLLRN_ControlRig ? Track->GetTLLRN_ControlRig() : NewObject<UTLLRN_ControlRig>(Track, InClass, FName(*ObjectName), RF_Transactional);
					if (InClass != UFKTLLRN_ControlRig::StaticClass() && !TLLRN_ControlRig->SupportsEvent(FTLLRN_RigUnit_InverseExecution::EventName))
					{
						TempAnimSequence->MarkAsGarbage();
						AnimSeqExportOption->MarkAsGarbage();
						OwnerMovieScene->RemoveTrack(*Track);
						return;
					}

					FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = GetEditMode();
					if (!TLLRN_ControlRigEditMode)
					{
						TLLRN_ControlRigEditMode = GetEditMode(true);
					}
					else
					{
						/* mz todo we don't unbind  will test more
						UTLLRN_ControlRig* OldTLLRN_ControlRig = TLLRN_ControlRigEditMode->GetTLLRN_ControlRig(false);
						if (OldTLLRN_ControlRig)
						{
							UnbindTLLRN_ControlRig(OldTLLRN_ControlRig);
						}
						*/
					}

					if (bReuseTLLRN_ControlRig == false)
					{
						TLLRN_ControlRig->Modify();
						TLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
						TLLRN_ControlRig->GetObjectBinding()->BindToObject(BoundActor);
						TLLRN_ControlRig->GetDataSourceRegistry()->RegisterDataSource(UTLLRN_ControlRig::OwnerComponent, TLLRN_ControlRig->GetObjectBinding()->GetBoundObject());
						TLLRN_ControlRig->Initialize();
						TLLRN_ControlRig->RequestInit();
						TLLRN_ControlRig->SetBoneInitialTransformsFromSkeletalMeshComponent(SkelMeshComp, true);
						TLLRN_ControlRig->Evaluate_AnyThread();
					}

					const bool bSequencerOwnsTLLRN_ControlRig = true;
					UMovieSceneSection* NewSection = Track->CreateTLLRN_ControlRigSection(0, TLLRN_ControlRig, bSequencerOwnsTLLRN_ControlRig);
					UMovieSceneTLLRN_ControlRigParameterSection* ParamSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(NewSection);

					//mz todo need to have multiple rigs with same class
					Track->SetTrackName(FName(*ObjectName));
					Track->SetDisplayName(FText::FromString(ObjectName));

					TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
					SequencerPtr->EmptySelection();
					SequencerPtr->SelectSection(NewSection);
					SequencerPtr->ThrobSectionSelection();
					SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
					TOptional<TRange<FFrameNumber>> OptionalRange = SequencerPtr->GetSubSequenceRange();
					FFrameNumber StartFrame = OptionalRange.IsSet() ? OptionalRange.GetValue().GetLowerBoundValue() : OwnerMovieScene->GetPlaybackRange().GetLowerBoundValue();
					LoadAnimationIntoSection(SequencerPtr, TempAnimSequence, SkelMeshComp, StartFrame,
						BakeSettings->bReduceKeys, BakeSettings->SmartReduce, BakeSettings->bResetControls, ParamSection);
					//Turn Off Any Skeletal Animation Tracks
					TArray<UMovieSceneSkeletalAnimationTrack*> SkelAnimationTracks;
					if (const FMovieSceneBinding* Binding = OwnerMovieScene->FindBinding(ObjectBinding))
					{
						for (UMovieSceneTrack* MovieSceneTrack : Binding->GetTracks())
						{
							if (UMovieSceneSkeletalAnimationTrack* SkelTrack = Cast<UMovieSceneSkeletalAnimationTrack>(MovieSceneTrack))
							{
								SkelAnimationTracks.Add(SkelTrack);
							}
						}
					}
					FGuid SkelMeshGuid = GetSequencer()->FindObjectId(*SkelMeshComp, GetSequencer()->GetFocusedTemplateID());
					if (const FMovieSceneBinding* Binding = OwnerMovieScene->FindBinding(SkelMeshGuid))
					{
						for (UMovieSceneTrack* MovieSceneTrack : Binding->GetTracks())
						{
							if (UMovieSceneSkeletalAnimationTrack* SkelTrack = Cast<UMovieSceneSkeletalAnimationTrack>(MovieSceneTrack))
							{
								SkelAnimationTracks.Add(SkelTrack);
							}
						}
					}

					for (UMovieSceneSkeletalAnimationTrack* SkelTrack : SkelAnimationTracks)
					{
						SkelTrack->Modify();
						//can't just turn off the track so need to mute the sections
						const TArray<UMovieSceneSection*>& Sections = SkelTrack->GetAllSections();
						for (UMovieSceneSection* Section : Sections)
						{
							if (Section)
							{
								Section->TryModify();
								Section->SetIsActive(false);
							}
						}
					}

					//Finish Setup
					if (TLLRN_ControlRigEditMode)
					{
						TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TLLRN_ControlRig, GetSequencer());
					}
					BindTLLRN_ControlRig(TLLRN_ControlRig);

					TempAnimSequence->MarkAsGarbage();
					AnimSeqExportOption->MarkAsGarbage();
					GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);


				}
			}
		}
	}
}

static void IterateTracksInMovieScene(UMovieScene& MovieScene, TFunctionRef<bool(UMovieSceneTLLRN_ControlRigParameterTrack*)> Callback)
{
	TArray<UMovieSceneTLLRN_ControlRigParameterTrack*> Tracks;
	
	const TArray<FMovieSceneBinding>& Bindings = MovieScene.GetBindings();
	for (const FMovieSceneBinding& Binding : Bindings)
	{
		TArray<UMovieSceneTrack*> FoundTracks = MovieScene.FindTracks(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding.GetObjectGuid(), NAME_None);
		for(UMovieSceneTrack* Track : FoundTracks)
		{
			if (UMovieSceneTLLRN_ControlRigParameterTrack* CRTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(Track))
			{
				Callback(CRTrack);
			}
		}
	}
	
	for (UMovieSceneTrack* Track : MovieScene.GetTracks())
	{
		if (UMovieSceneTLLRN_ControlRigParameterTrack* CRTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(Track))
		{
			Callback(CRTrack);
		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::IterateTracks(TFunctionRef<bool(UMovieSceneTLLRN_ControlRigParameterTrack*)> Callback) const
{
	UMovieScene* MovieScene = GetSequencer().IsValid() && GetSequencer()->GetFocusedMovieSceneSequence() ? GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene() : nullptr;
	if (!MovieScene)
	{
		return;
	}

	IterateTracksInMovieScene(*MovieScene, Callback);
}


void FTLLRN_ControlRigParameterTrackEditor::BakeInvertedPose(UTLLRN_ControlRig* InTLLRN_ControlRig, UMovieSceneTLLRN_ControlRigParameterTrack* Track)
{
	if (!InTLLRN_ControlRig->IsAdditive())
	{
		return;
	}

	UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Track->GetSectionToKey());
	USkeletalMeshComponent* SkelMeshComp = Cast<USkeletalMeshComponent>(InTLLRN_ControlRig->GetObjectBinding()->GetBoundObject());
	UMovieSceneSequence* RootMovieSceneSequence = GetSequencer()->GetRootMovieSceneSequence();
	UMovieSceneSequence* MovieSceneSequence = GetSequencer()->GetFocusedMovieSceneSequence();
	UMovieScene* MovieScene = MovieSceneSequence->GetMovieScene();
	UAnimSeqExportOption* ExportOptions = NewObject<UAnimSeqExportOption>(GetTransientPackage(), NAME_None);

	//@sara to do, not sure if you want to key reduce after, but BakeSettings isn't used
	//UBakeToTLLRN_ControlRigSettings* BakeSettings = GetMutableDefault<UBakeToTLLRN_ControlRigSettings>();
	const TSharedPtr<ISequencer> ParentSequencer = GetSequencer();
	FMovieSceneSequenceIDRef Template = ParentSequencer->GetFocusedTemplateID();
	FMovieSceneSequenceTransform RootToLocalTransform = ParentSequencer->GetFocusedMovieSceneSequenceTransform();

	if (ExportOptions == nullptr || MovieScene == nullptr || SkelMeshComp == nullptr)
	{
		UE_LOG(LogMovieScene, Error, TEXT("FTLLRN_ControlRigParameterTrackEditor::BakeInvertedPose All parameters must be valid."));
		return;
	}
	
	const FScopedTransaction Transaction(LOCTEXT("BakeInvertedPose_Transaction", "Bake Inverted Pose"));

	UnFbx::FLevelSequenceAnimTrackAdapter AnimTrackAdapter(ParentSequencer.Get(), MovieSceneSequence, RootMovieSceneSequence, RootToLocalTransform);
	int32 AnimationLength = AnimTrackAdapter.GetLength();
	FScopedSlowTask Progress(AnimationLength, LOCTEXT("BakingToTLLRN_ControlRig_SlowTask", "Baking To Control Rig..."));
	Progress.MakeDialog(true);

	auto DelegateHandle = InTLLRN_ControlRig->OnPreAdditiveValuesApplication_AnyThread().AddLambda([](UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
	{
		InTLLRN_ControlRig->InvertInputPose();
	});

	auto KeyFrame = [this, ParentSequencer, InTLLRN_ControlRig, SkelMeshComp](const FFrameNumber FrameNumber)
	{
		const FFrameNumber NewTime = ConvertFrameTime(FrameNumber, ParentSequencer->GetFocusedDisplayRate(), ParentSequencer->GetFocusedTickResolution()).FrameNumber;
		float LocalTime = ParentSequencer->GetFocusedTickResolution().AsSeconds(FFrameTime(NewTime));

		AddControlKeys(SkelMeshComp, InTLLRN_ControlRig, InTLLRN_ControlRig->GetFName(), NAME_None, ETLLRN_ControlRigContextChannelToKey::AllTransform, 
				ESequencerKeyMode::ManualKeyForced, LocalTime);
	};

	FInitAnimationCB InitCallback = FInitAnimationCB::CreateLambda([]{});
	FStartAnimationCB StartCallback = FStartAnimationCB::CreateLambda([AnimTrackAdapter, KeyFrame]
	{
		KeyFrame(AnimTrackAdapter.GetLocalStartFrame());
	});
	FTickAnimationCB TickCallback = FTickAnimationCB::CreateLambda([KeyFrame, &Progress](float DeltaTime, FFrameNumber FrameNumber)
	{
		KeyFrame(FrameNumber);
		Progress.EnterProgressFrame(1);
	});
	FEndAnimationCB EndCallback = FEndAnimationCB::CreateLambda([]{});

	FAnimExportSequenceParameters AESP;
	AESP.Player = ParentSequencer.Get();
	AESP.RootToLocalTransform = RootToLocalTransform;
	AESP.MovieSceneSequence = MovieSceneSequence;
	AESP.RootMovieSceneSequence = RootMovieSceneSequence;

	MovieSceneToolHelpers::BakeToSkelMeshToCallbacks(AESP,SkelMeshComp, ExportOptions,
		InitCallback, StartCallback, TickCallback, EndCallback);

	InTLLRN_ControlRig->OnPreAdditiveValuesApplication_AnyThread().Remove(DelegateHandle);
	ParentSequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
}

bool FTLLRN_ControlRigParameterTrackEditor::IsLayered(UMovieSceneTLLRN_ControlRigParameterTrack* Track) const
{
	UTLLRN_ControlRig* TLLRN_ControlRig = Track->GetTLLRN_ControlRig();
	if (!TLLRN_ControlRig)
	{
		return false;
	}
	return TLLRN_ControlRig->IsAdditive();
}

void FTLLRN_ControlRigParameterTrackEditor::ConvertIsLayered(UMovieSceneTLLRN_ControlRigParameterTrack* Track)
{
	UTLLRN_ControlRig* TLLRN_ControlRig = Track->GetTLLRN_ControlRig();
	if (!TLLRN_ControlRig)
	{
		return;
	}

	const bool bSetAdditive = !TLLRN_ControlRig->IsAdditive();
	UTLLRN_ControlRigSequencerEditorLibrary::SetTLLRN_ControlRigLayeredMode(Track, bSetAdditive);
}

void FTLLRN_ControlRigParameterTrackEditor::BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const TArray<FGuid>& ObjectBindings, const UClass* ObjectClass)
{
	if(!ObjectClass)
	{
		return;
	}
	
	if (ObjectClass->IsChildOf(USkeletalMeshComponent::StaticClass()) ||
		ObjectClass->IsChildOf(AActor::StaticClass()) ||
		ObjectClass->IsChildOf(UChildActorComponent::StaticClass()))
	{
		const TSharedPtr<ISequencer> ParentSequencer = GetSequencer();
		UObject* BoundObject = nullptr;
		USkeleton* Skeleton = AcquireSkeletonFromObjectGuid(ObjectBindings[0], &BoundObject, ParentSequencer);

		if (AActor* BoundActor = Cast<AActor>(BoundObject))
		{
			if (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent = BoundActor->FindComponentByClass<UTLLRN_ControlRigComponent>())
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("AddTLLRN_ControlRigTrack", "Add Control Rig Track"),
					LOCTEXT("AddTLLRN_ControlRigTrackTooltip", "Adds an animation Control Rig track"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSP(this, &FTLLRN_ControlRigParameterTrackEditor::AddTLLRN_ControlRigFromComponent, ObjectBindings[0]),
						FCanExecuteAction()
					)
				);
				return;
			}
		}

		if (Skeleton)
		{
			UMovieSceneTrack* Track = nullptr;
			MenuBuilder.AddSubMenu(LOCTEXT("TLLRN_ControlRigText", "TLLRN Control Rig"), FText(), FNewMenuDelegate::CreateSP(this, &FTLLRN_ControlRigParameterTrackEditor::HandleAddTrackSubMenu, ObjectBindings, Track));
		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::ToggleIsAdditiveTLLRN_ControlRig()
{
	bIsLayeredTLLRN_ControlRig = bIsLayeredTLLRN_ControlRig ? false : true;
	RefreshTLLRN_ControlRigPickerDelegate.ExecuteIfBound(true);
}

bool FTLLRN_ControlRigParameterTrackEditor::IsToggleIsAdditiveTLLRN_ControlRig()
{
	return bIsLayeredTLLRN_ControlRig;
}

void FTLLRN_ControlRigParameterTrackEditor::ToggleFilterAssetBySkeleton()
{
	bFilterAssetBySkeleton = bFilterAssetBySkeleton ? false : true;
	RefreshTLLRN_ControlRigPickerDelegate.ExecuteIfBound(true);
}

bool FTLLRN_ControlRigParameterTrackEditor::IsToggleFilterAssetBySkeleton()
{
	return bFilterAssetBySkeleton;
}

void FTLLRN_ControlRigParameterTrackEditor::ToggleFilterAssetByAnimatableControls()
{
	bFilterAssetByAnimatableControls = bFilterAssetByAnimatableControls ? false : true;
	RefreshTLLRN_ControlRigPickerDelegate.ExecuteIfBound(true);
}

bool FTLLRN_ControlRigParameterTrackEditor::IsToggleFilterAssetByAnimatableControls()
{
	return bFilterAssetByAnimatableControls;
}

void FTLLRN_ControlRigParameterTrackEditor::HandleAddTrackSubMenu(FMenuBuilder& MenuBuilder, TArray<FGuid> ObjectBindings, UMovieSceneTrack* Track)
{
	if (CVarEnableAdditiveTLLRN_ControlRigs->GetBool())
	{
		MenuBuilder.AddMenuEntry(
		LOCTEXT("IsLayeredTLLRN_ControlRig", "Layered"),
		LOCTEXT("IsLayeredTLLRN_ControlRigTooltip", "When checked, a layered control rig will be added"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FTLLRN_ControlRigParameterTrackEditor::ToggleIsAdditiveTLLRN_ControlRig),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &FTLLRN_ControlRigParameterTrackEditor::IsToggleIsAdditiveTLLRN_ControlRig)
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton);
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("FilterAssetBySkeleton", "Filter Asset By Skeleton"),
		LOCTEXT("FilterAssetBySkeletonTooltip", "Filters Control Rig assets to match current skeleton"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FTLLRN_ControlRigParameterTrackEditor::ToggleFilterAssetBySkeleton),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &FTLLRN_ControlRigParameterTrackEditor::IsToggleFilterAssetBySkeleton)
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("FilterAssetByAnimatableControls", "Filter Asset By Animatable Controls"),
		LOCTEXT("FilterAssetByAnimatableControlsTooltip", "Filters Control Rig assets to only show those with Animatable Controls"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FTLLRN_ControlRigParameterTrackEditor::ToggleFilterAssetByAnimatableControls),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &FTLLRN_ControlRigParameterTrackEditor::IsToggleFilterAssetByAnimatableControls)
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton);

	const TSharedPtr<ISequencer> ParentSequencer = GetSequencer();
	UObject* BoundObject = nullptr;
	//todo support multiple bindings?
	USkeleton* Skeleton = AcquireSkeletonFromObjectGuid(ObjectBindings[0], &BoundObject, ParentSequencer);

	if (Skeleton)
	{
		TArray<UClass*> ExistingRigs;
		USkeletalMeshComponent* SkeletalMeshComponent = AcquireSkeletalMeshFromObject(BoundObject, ParentSequencer);
		IterateTracks([&ExistingRigs, SkeletalMeshComponent](UMovieSceneTLLRN_ControlRigParameterTrack* Track) -> bool
		{
			if (UTLLRN_ControlRig* TLLRN_ControlRig = Track->GetTLLRN_ControlRig())
			{
				if (TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig->GetObjectBinding())
				{
					if (ObjectBinding.IsValid() && ObjectBinding->GetBoundObject() == SkeletalMeshComponent)
					{
						ExistingRigs.Add(TLLRN_ControlRig->GetClass());
					}
				}
			}
			return true;
		});
		MenuBuilder.AddSeparator();
		MenuBuilder.AddMenuEntry(
			LOCTEXT("FKTLLRN_ControlRig", "FK Control Rig"),
			LOCTEXT("FKTLLRN_ControlRigTooltip", "Adds the FK Control Rig"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FTLLRN_ControlRigParameterTrackEditor::AddFKTLLRN_ControlRig, BoundObject, ObjectBindings[0]),
				FCanExecuteAction::CreateLambda([ExistingRigs]()
				{
					if (!ExistingRigs.Contains(UFKTLLRN_ControlRig::StaticClass()))
					{
						return true;
					}
					return false;
				})
			),
			NAME_None,
			EUserInterfaceActionType::Button);
		
		FAssetPickerConfig AssetPickerConfig;
		{
			AssetPickerConfig.SelectionMode = ESelectionMode::Single;
			AssetPickerConfig.bAddFilterUI = true;
			AssetPickerConfig.bFocusSearchBoxWhenOpened = true;
			AssetPickerConfig.bForceShowPluginContent = true;
			AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateRaw(this, &FTLLRN_ControlRigParameterTrackEditor::AddTLLRN_ControlRig, BoundObject, ObjectBindings[0]);
			AssetPickerConfig.OnAssetEnterPressed = FOnAssetEnterPressed::CreateRaw(this, &FTLLRN_ControlRigParameterTrackEditor::AddTLLRN_ControlRig, BoundObject, ObjectBindings[0]);
			AssetPickerConfig.RefreshAssetViewDelegates.Add(&RefreshTLLRN_ControlRigPickerDelegate);
			AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateLambda([this, ExistingRigs, Skeleton](const FAssetData& AssetData)
			{
				if (!IsTLLRN_ControlRigAllowed(AssetData, ExistingRigs, Skeleton))
				{
					// Should be filtered out
					return true;
				}

				return false;
			});
			AssetPickerConfig.bAllowNullSelection = false;
			AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
			AssetPickerConfig.Filter.bRecursiveClasses = true;
			AssetPickerConfig.Filter.ClassPaths.Add((UTLLRN_ControlRigBlueprint::StaticClass())->GetClassPathName());
			AssetPickerConfig.SaveSettingsName = TEXT("SequencerTLLRN_ControlRigTrackAssetPicker");
			TSharedRef<FFrontendFilterCategory>	TLLRN_ControlRigFilterCategory = MakeShared<FFrontendFilterCategory>(LOCTEXT("TLLRN_ControlRigFilterCategoryName", "TLLRN Control Rig Tags"), LOCTEXT("TLLRN_ControlRigFilterCategoryToolTip", "Filter TLLRN_ControlRigs by variant tags specified in TLLRN_ControlRig Blueprint class settings"));
			const URigVMProjectSettings* Settings = GetDefault<URigVMProjectSettings>(URigVMProjectSettings::StaticClass());
			TArray<FRigVMTag> AvailableTags = Settings->VariantTags;

			for (const FRigVMTag& Tag : AvailableTags)
			{
				AssetPickerConfig.ExtraFrontendFilters.Add(MakeShared<FFrontendFilter_TLLRN_ControlRigFilterByAssetTag>(TLLRN_ControlRigFilterCategory, Tag));
			}

			// This is so that we can remove the "Other Filters" section easily
			AssetPickerConfig.bUseSectionsForCustomFilterCategories = true;
			// Make sure we only show TLLRN_ControlRig filters to avoid confusion 
			AssetPickerConfig.OnExtendAddFilterMenu = FOnExtendAddFilterMenu::CreateLambda([TLLRN_ControlRigFilters = AssetPickerConfig.ExtraFrontendFilters](UToolMenu* InToolMenu)
			{
				// "AssetFilterBarFilterAdvancedAsset" taken from SAssetFilterBar.h PopulateAddFilterMenu()
				InToolMenu->RemoveSection("AssetFilterBarFilterAdvancedAsset");
				InToolMenu->RemoveSection("Other Filters");
			});
		}

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

		TSharedPtr<SBox> MenuEntry = SNew(SBox)
		// Extra space to display filter capsules horizontally
		.WidthOverride(600.f)
		.HeightOverride(300.f)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		];

		MenuBuilder.AddWidget(MenuEntry.ToSharedRef(), FText::GetEmpty(), true);
	}
}

bool FTLLRN_ControlRigParameterTrackEditor::IsTLLRN_ControlRigAllowed(const FAssetData& AssetData, TArray<UClass*> ExistingRigs, USkeleton* Skeleton)
{
	static const FName TLLRN_RigModuleSettingsPropertyName = GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, TLLRN_RigModuleSettings);
	const FProperty* TLLRN_RigModuleSettingsProperty = CastField<FProperty>(UTLLRN_ControlRigBlueprint::StaticClass()->FindPropertyByName(TLLRN_RigModuleSettingsPropertyName));
	const FString TLLRN_RigModuleSettingsStr = AssetData.GetTagValueRef<FString>(TLLRN_RigModuleSettingsPropertyName);
	if(!TLLRN_RigModuleSettingsStr.IsEmpty())
	{
		FTLLRN_RigModuleSettings TLLRN_RigModuleSettings;
		TLLRN_RigModuleSettingsProperty->ImportText_Direct(*TLLRN_RigModuleSettingsStr, &TLLRN_RigModuleSettings, nullptr, EPropertyPortFlags::PPF_None);
		
		// Currently rig module can only be used in a modular rig, not in sequencer
		// see UTLLRN_ControlRigBlueprint::IsTLLRN_ControlTLLRN_RigModule()
		if (TLLRN_RigModuleSettings.Identifier.IsValid())
		{
			return false;
		}
	}
	
	if (UTLLRN_ControlRigBlueprint* LoadedTLLRN_ControlRig = Cast<UTLLRN_ControlRigBlueprint>(AssetData.FastGetAsset()))
	{
		if (ExistingRigs.Contains(LoadedTLLRN_ControlRig->GetRigVMBlueprintGeneratedClass()))
		{
			if (!AssetData.GetTagValueRef<bool>(GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, bAllowMultipleInstances)))
			{
				return false;
			}	
		}
	}

	const IAssetRegistry& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	
	const bool bExposesAnimatableControls = AssetData.GetTagValueRef<bool>(TEXT("bExposesAnimatableControls"));
	if (bFilterAssetByAnimatableControls == true && bExposesAnimatableControls == false)
	{
		return false;
	}
	if (bIsLayeredTLLRN_ControlRig)
	{		
		FAssetDataTagMapSharedView::FFindTagResult Tag = AssetData.TagsAndValues.FindTag(TEXT("SupportedEventNames"));
		if (Tag.IsSet())
		{
			bool bHasInversion = false;
			FString EventString = FTLLRN_RigUnit_InverseExecution::EventName.ToString();
			FString OldEventString = FString(TEXT("Inverse"));
			TArray<FString> SupportedEventNames;
			Tag.GetValue().ParseIntoArray(SupportedEventNames, TEXT(","), true);

			for (const FString& Name : SupportedEventNames)
			{
				if (Name.Contains(EventString) || Name.Contains(OldEventString))
				{
					bHasInversion = true;
					break;
				}
			}
			if (bHasInversion == false)
			{
				return false;
			}
		}
	}
	if (bFilterAssetBySkeleton)
	{
		FString SkeletonName;
		if (Skeleton)
		{
			SkeletonName = FAssetData(Skeleton).GetExportTextName();
		}
		FString PreviewSkeletalMesh = AssetData.GetTagValueRef<FString>(TEXT("PreviewSkeletalMesh"));
		if (PreviewSkeletalMesh.Len() > 0)
		{
			FAssetData SkelMeshData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(PreviewSkeletalMesh));
			FString PreviewSkeleton = SkelMeshData.GetTagValueRef<FString>(TEXT("Skeleton"));
			if (PreviewSkeleton == SkeletonName)
			{
				return true;
			}
			else if(Skeleton)
			{
				if (Skeleton->IsCompatibleForEditor(PreviewSkeleton))
				{
					return true;
				}
			}
		}
		FString PreviewSkeleton = AssetData.GetTagValueRef<FString>(TEXT("PreviewSkeleton"));
		if (PreviewSkeleton == SkeletonName)
		{
			return true;
		}
		else if (Skeleton)
		{
			if (Skeleton->IsCompatibleForEditor(PreviewSkeleton))
			{
				return true;
			}
		}
		FString SourceHierarchyImport = AssetData.GetTagValueRef<FString>(TEXT("SourceHierarchyImport"));
		if (SourceHierarchyImport == SkeletonName)
		{
			return true;
		}
		else if (Skeleton)
		{
			if (Skeleton->IsCompatibleForEditor(SourceHierarchyImport))
			{
				return true;
			}
		}
		FString SourceCurveImport = AssetData.GetTagValueRef<FString>(TEXT("SourceCurveImport"));
		if (SourceCurveImport == SkeletonName)
		{
			return true;
		}
		else if (Skeleton)
		{
			if (Skeleton->IsCompatibleForEditor(SourceCurveImport))
			{
				return true;
			}
		}

		if (!FSoftObjectPath(PreviewSkeletalMesh).IsValid() &&
			!FSoftObjectPath(PreviewSkeleton).IsValid() &&
			!FSoftObjectPath(SourceHierarchyImport).IsValid() &&
			!FSoftObjectPath(SourceCurveImport).IsValid())
		{
			// this indicates that the rig can work on any skeleton (for example, utility rigs or deformer rigs)
			return true;
		}
		
		return false;
	}
	return true;	
}

/*
bool FTLLRN_ControlRigParameterTrackEditor::ShouldFilterAsset(const FAssetData& AssetData)
{
	if (AssetData.AssetClassPath == UTLLRN_ControlRig::StaticClass()->GetClassPathName())
	{
		return true;
	}
	return false;
}

void FTLLRN_ControlRigParameterTrackEditor::OnTLLRN_ControlRigAssetSelected(const FAssetData& AssetData, TArray<FGuid> ObjectBindings, USkeleton* Skeleton)
{
	FSlateApplication::Get().DismissAllMenus();

	UObject* SelectedObject = AssetData.GetAsset();todo
	TSharedPtr<ISequencer> SequencerPtr = GetSequencer();

	if (SelectedObject && SelectedObject->IsA(UTLLRN_ControlRig::StaticClass()) && SequencerPtr.IsValid())
	{
		UTLLRN_ControlRig* TLLRN_ControlRig = CastChecked<UTLLRN_ControlRig>(AssetData.GetAsset());

		const FScopedTransaction Transaction(LOCTEXT("AddAnimation_Transaction", "Add Animation"));

		for (FGuid ObjectBinding : ObjectBindings)
		{
			UObject* Object = SequencerPtr->FindSpawnedObjectOrTemplate(ObjectBinding);
			int32 RowIndex = INDEX_NONE;
			//AnimatablePropertyChanged(FOnKeyProperty::CreateRaw(this, &FTLLRN_ControlRigParameterTrackEditor::AddTLLRN_ControlRig, BoundObject, ObjectBindings[0], Skeleton)));
		}
	}
*/

static TArray<UMovieSceneTLLRN_ControlRigParameterTrack*> GetExistingTLLRN_ControlRigTracksForSkeletalMeshComponent(UMovieScene* MovieScene, USkeletalMeshComponent* SkeletalMeshComponent)
{
	TArray<UMovieSceneTLLRN_ControlRigParameterTrack*> ExistingTLLRN_ControlRigTracks;
	IterateTracksInMovieScene(*MovieScene, [&ExistingTLLRN_ControlRigTracks, SkeletalMeshComponent](UMovieSceneTLLRN_ControlRigParameterTrack* Track) -> bool
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Track->GetTLLRN_ControlRig())
		{
			if (TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig->GetObjectBinding())
			{
				if (ObjectBinding.IsValid() && ObjectBinding->GetBoundObject() == SkeletalMeshComponent)
				{
					ExistingTLLRN_ControlRigTracks.Add(Track);
				}
			}
		}
		return true;
	});

	return ExistingTLLRN_ControlRigTracks;
}

static UMovieSceneTLLRN_ControlRigParameterTrack* AddTLLRN_ControlRig(TSharedPtr<ISequencer> SharedSequencer , UMovieSceneSequence* Sequence, const UClass* InClass, FGuid ObjectBinding, UTLLRN_ControlRig* InExistingTLLRN_ControlRig, bool bIsAdditiveTLLRN_ControlRig)
{
	FSlateApplication::Get().DismissAllMenus();
	if (!InClass || !InClass->IsChildOf(UTLLRN_ControlRig::StaticClass()) ||
		!Sequence || !Sequence->GetMovieScene())
	{
		return nullptr;
	}

	UMovieScene* OwnerMovieScene = Sequence->GetMovieScene();
	ISequencer* Sequencer = nullptr; // will be valid  if we have a ISequencer AND it's focused.
	if (SharedSequencer.IsValid() && SharedSequencer->GetFocusedMovieSceneSequence() == Sequence)
	{
		Sequencer = SharedSequencer.Get();
	}
	Sequence->Modify();
	OwnerMovieScene->Modify();

	if (bIsAdditiveTLLRN_ControlRig && InClass != UFKTLLRN_ControlRig::StaticClass() && !InClass->GetDefaultObject<UTLLRN_ControlRig>()->SupportsEvent(FTLLRN_RigUnit_InverseExecution::EventName))
	{
		UE_LOG(LogTLLRN_ControlRigEditor, Error, TEXT("Cannot add an additive control rig which does not contain a backwards solve event."));
		return nullptr;
	}

	FScopedTransaction AddTLLRN_ControlRigTrackTransaction(LOCTEXT("AddTLLRN_ControlRigTrack", "Add Control Rig Track"));

	TArray<UMovieSceneTLLRN_ControlRigParameterTrack*> ExistingRigTracks;
	
	if (USkeletalMeshComponent* SkeletalMeshComponent = AcquireSkeletalMeshFromObjectGuid(ObjectBinding, SharedSequencer))
	{
		ExistingRigTracks = GetExistingTLLRN_ControlRigTracksForSkeletalMeshComponent(OwnerMovieScene, SkeletalMeshComponent);
	}
	
	UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(OwnerMovieScene->AddTrack(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), ObjectBinding));
	if (Track)
	{
		TArray<FName> ExistingObjectNames;
		for (UMovieSceneTLLRN_ControlRigParameterTrack* RigTrack : ExistingRigTracks)
		{
			if (UTLLRN_ControlRig* Rig = RigTrack->GetTLLRN_ControlRig())
			{
				if (Rig->GetClass() == InClass)
				{
					ExistingObjectNames.Add(RigTrack->GetTrackName());
				}
			}
		}
		
		FString ObjectName = InClass->GetName(); //GetDisplayNameText().ToString();
		ObjectName.RemoveFromEnd(TEXT("_C"));

		{
			FName UniqueObjectName = *ObjectName;
			int32 UniqueSuffix = 1;
			while(ExistingObjectNames.Contains(UniqueObjectName))
			{
				UniqueObjectName = *(ObjectName + TEXT("_") + FString::FromInt(UniqueSuffix));
				UniqueSuffix++;
			}

			ObjectName = UniqueObjectName.ToString();
		}
		
		
		bool bSequencerOwnsTLLRN_ControlRig = false;
		UTLLRN_ControlRig* TLLRN_ControlRig = InExistingTLLRN_ControlRig;
		if (TLLRN_ControlRig == nullptr)
		{
			TLLRN_ControlRig = NewObject<UTLLRN_ControlRig>(Track, InClass, FName(*ObjectName), RF_Transactional);
			bSequencerOwnsTLLRN_ControlRig = true;
		}

		TLLRN_ControlRig->Modify();
		if (UFKTLLRN_ControlRig* FKTLLRN_ControlRig = Cast<UFKTLLRN_ControlRig>(Cast<UTLLRN_ControlRig>(TLLRN_ControlRig)))
		{
			if (bIsAdditiveTLLRN_ControlRig)
			{
				FKTLLRN_ControlRig->SetApplyMode(ETLLRN_ControlRigFKRigExecuteMode::Additive);
			}
		}
		else
		{
			TLLRN_ControlRig->SetIsAdditive(bIsAdditiveTLLRN_ControlRig);
		}
		TLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
		// Do not re-initialize existing control rig
		if (!InExistingTLLRN_ControlRig)
		{
			TLLRN_ControlRig->Initialize();
		}
		TLLRN_ControlRig->Evaluate_AnyThread();

		if (SharedSequencer.IsValid())
		{
			SharedSequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
		}

		Track->Modify();
		UMovieSceneSection* NewSection = Track->CreateTLLRN_ControlRigSection(0, TLLRN_ControlRig, bSequencerOwnsTLLRN_ControlRig);
		NewSection->Modify();

		if (bIsAdditiveTLLRN_ControlRig)
		{
			const FString AdditiveObjectName = ObjectName + TEXT(" (Layered)");
			Track->SetTrackName(FName(*ObjectName));
			Track->SetDisplayName(FText::FromString(AdditiveObjectName));
			Track->SetColorTint(UMovieSceneTLLRN_ControlRigParameterTrack::LayeredRigTrackColor);
		}
		else
		{
			//mz todo need to have multiple rigs with same class
			Track->SetTrackName(FName(*ObjectName));
			Track->SetDisplayName(FText::FromString(ObjectName));
			Track->SetColorTint(UMovieSceneTLLRN_ControlRigParameterTrack::AbsoluteRigTrackColor);
		}

		if (SharedSequencer.IsValid())
		{
			SharedSequencer->EmptySelection();
			SharedSequencer->SelectSection(NewSection);
			SharedSequencer->ThrobSectionSelection();
			SharedSequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
			SharedSequencer->ObjectImplicitlyAdded(TLLRN_ControlRig);
		}

		FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
		if (!TLLRN_ControlRigEditMode)
		{
			GLevelEditorModeTools().ActivateMode(FTLLRN_ControlRigEditMode::ModeName);
			TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));

		}
		if (TLLRN_ControlRigEditMode)
		{
			TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TLLRN_ControlRig, SharedSequencer);
		}
		return Track;
	}
	return nullptr;
}
static UMovieSceneTrack* FindOrCreateTLLRN_ControlRigTrack(TSharedPtr<ISequencer>& Sequencer, UWorld* World, const UClass* TLLRN_ControlRigClass, const FMovieSceneBindingProxy& InBinding, bool bIsLayeredTLLRN_ControlRig)
{
	UMovieScene* MovieScene = InBinding.Sequence ? InBinding.Sequence->GetMovieScene() : nullptr;
	UMovieSceneTrack* BaseTrack = nullptr;
	if (MovieScene && InBinding.BindingID.IsValid())
	{
		if (const FMovieSceneBinding* Binding = MovieScene->FindBinding(InBinding.BindingID))
		{
			if (!DoesTLLRN_ControlRigAllowMultipleInstances(TLLRN_ControlRigClass->GetClassPathName()))
			{
				TArray<UMovieSceneTrack*> Tracks = MovieScene->FindTracks(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding->GetObjectGuid(), NAME_None);
				for (UMovieSceneTrack* AnyOleTrack : Tracks)
				{
					UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(AnyOleTrack);
					if (Track && Track->GetTLLRN_ControlRig() && Track->GetTLLRN_ControlRig()->GetClass() == TLLRN_ControlRigClass)
					{
						return Track;
					}
				}
			}

			UMovieSceneTLLRN_ControlRigParameterTrack* Track = AddTLLRN_ControlRig(Sequencer, InBinding.Sequence, TLLRN_ControlRigClass, InBinding.BindingID, nullptr, bIsLayeredTLLRN_ControlRig);

			if (Track)
			{
				BaseTrack = Track;
			}
		}
	}
	return BaseTrack;
}

void FTLLRN_ControlRigParameterTrackEditor::AddTLLRN_ControlRig(const UClass* InClass, UObject* BoundActor, FGuid ObjectBinding, UTLLRN_ControlRig* InExistingTLLRN_ControlRig)
{
	UWorld* World = GCurrentLevelEditingViewportClient ? GCurrentLevelEditingViewportClient->GetWorld() : nullptr;
	UMovieSceneSequence* Sequence = GetSequencer()->GetFocusedMovieSceneSequence();
	FMovieSceneBindingProxy BindingProxy(ObjectBinding, Sequence);
	TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
	//UTLLRN_ControlRigSequencerEditorLibrary::FindOrCreateTLLRN_ControlRigTrack.. in 5.5 we will redo this but for 
	//5.4.4 we can't change headers so for now we just make the change here locally
	if (UMovieSceneTrack* Track = FindOrCreateTLLRN_ControlRigTrack(SequencerPtr, World, InClass, BindingProxy, bIsLayeredTLLRN_ControlRig))
	{
		BindTLLRN_ControlRig(CastChecked<UMovieSceneTLLRN_ControlRigParameterTrack>(Track)->GetTLLRN_ControlRig());
	}
}

void FTLLRN_ControlRigParameterTrackEditor::AddTLLRN_ControlRig(const FAssetData& InAsset, UObject* BoundActor, FGuid ObjectBinding)
{
	if (UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(InAsset.GetAsset()))
	{
		AddTLLRN_ControlRig(TLLRN_ControlRigBlueprint->GetRigVMBlueprintGeneratedClass(), BoundActor, ObjectBinding);
	}
}

void FTLLRN_ControlRigParameterTrackEditor::AddTLLRN_ControlRig(const TArray<FAssetData>& InAssets, UObject* BoundActor, FGuid ObjectBinding)
{
	if (InAssets.Num() > 0)
	{
		AddTLLRN_ControlRig(InAssets[0], BoundActor, ObjectBinding);	
	}
}

void FTLLRN_ControlRigParameterTrackEditor::AddFKTLLRN_ControlRig(UObject* BoundActor, FGuid ObjectBinding)
{
	AddTLLRN_ControlRig(UFKTLLRN_ControlRig::StaticClass(), BoundActor, ObjectBinding);
}

void FTLLRN_ControlRigParameterTrackEditor::AddTLLRN_ControlRig(UClass* InClass, UObject* BoundActor, FGuid ObjectBinding)
{
	if (InClass == UFKTLLRN_ControlRig::StaticClass())
	{
		AcquireSkeletonFromObjectGuid(ObjectBinding, &BoundActor, GetSequencer());
	}
	AddTLLRN_ControlRig(InClass, BoundActor, ObjectBinding, nullptr);
}

//This now adds all of the control rig components, not just the first one
void FTLLRN_ControlRigParameterTrackEditor::AddTLLRN_ControlRigFromComponent(FGuid InGuid)
{
	const TSharedPtr<ISequencer> ParentSequencer = GetSequencer();
	UObject* BoundObject = ParentSequencer.IsValid() ? ParentSequencer->FindSpawnedObjectOrTemplate(InGuid) : nullptr;

	if (AActor* BoundActor = Cast<AActor>(BoundObject))
	{
		TArray<UTLLRN_ControlRigComponent*> TLLRN_ControlRigComponents;
		BoundActor->GetComponents(TLLRN_ControlRigComponents);
		for (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent : TLLRN_ControlRigComponents)
		{
			if (UTLLRN_ControlRig* CR = TLLRN_ControlRigComponent->GetTLLRN_ControlRig())
			{
				AddTLLRN_ControlRig(CR->GetClass(), BoundActor, InGuid, CR);
			}
		}

	}
}

bool FTLLRN_ControlRigParameterTrackEditor::HasTransformKeyOverridePriority() const
{
	return false;
}

bool FTLLRN_ControlRigParameterTrackEditor::CanAddTransformKeysForSelectedObjects() const
{
	// WASD hotkeys to fly the viewport can conflict with hotkeys for setting keyframes (ie. s). 
	// If the viewport is moving, disregard setting keyframes.
	for (FLevelEditorViewportClient* LevelVC : GEditor->GetLevelViewportClients())
	{
		if (LevelVC && LevelVC->IsMovingCamera())
		{
			return false;
		}
	}

	if (!GetSequencer()->IsAllowedToChange())
	{
		return false;
	}

	FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = GetEditMode();
	if (TLLRN_ControlRigEditMode)
	{
		TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> SelectedControls;
		TLLRN_ControlRigEditMode->GetAllSelectedControls(SelectedControls);
		return (SelectedControls.Num() > 0);
		}
	return false;
}

void FTLLRN_ControlRigParameterTrackEditor::OnAddTransformKeysForSelectedObjects(EMovieSceneTransformChannel Channel)
{
	if (!GetSequencer()->IsAllowedToChange())
	{
		return;
	}
	
	const FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = GetEditMode();
	if (!TLLRN_ControlRigEditMode)
	{
		return;
	}

	TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> SelectedControls;
	TLLRN_ControlRigEditMode->GetAllSelectedControls(SelectedControls);
	if (SelectedControls.Num() <= 0)
	{
		return;
	}
	const ETLLRN_ControlRigContextChannelToKey ChannelsToKey = static_cast<ETLLRN_ControlRigContextChannelToKey>(Channel); 
	FScopedTransaction KeyTransaction(LOCTEXT("SetKeysOnControls", "Set Keys On Controls"), !GIsTransacting);

	static constexpr bool bInConstraintSpace = true;
	FTLLRN_RigControlModifiedContext NotifyDrivenContext;
	NotifyDrivenContext.SetKey = ETLLRN_ControlRigSetKey::Always;
	for (const TPair<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>>& Selection : SelectedControls)
	{
		UTLLRN_ControlRig* TLLRN_ControlRig = Selection.Key;
		if (const TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig->GetObjectBinding())
		{
			if (UObject* Object = ObjectBinding->GetBoundObject())
			{
				const FName Name(*TLLRN_ControlRig->GetName());
			
				const TArray<FName> ControlNames = TLLRN_ControlRig->CurrentControlSelection();
				for (const FName& ControlName : ControlNames)
				{
					if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ControlName))
					{
						AddControlKeys(Object, TLLRN_ControlRig, Name, ControlName, ChannelsToKey,
							ESequencerKeyMode::ManualKeyForced, FLT_MAX, bInConstraintSpace);
						FTLLRN_ControlRigEditMode::NotifyDrivenControls(TLLRN_ControlRig, ControlElement->GetKey(), NotifyDrivenContext);
					}
				}
			}
		}
	}
}

//function to evaluate a Control and Set it on the TLLRN_ControlRig
static void EvaluateThisControl(UMovieSceneTLLRN_ControlRigParameterSection* Section, const FName& ControlName, const FFrameTime& FrameTime)
{
	if (!Section)
	{
		return;
	}
	UTLLRN_ControlRig* TLLRN_ControlRig = Section->GetTLLRN_ControlRig();
	if (!TLLRN_ControlRig)
	{
		return;
	}
	if(FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ControlName))
	{ 
		FTLLRN_ControlRigInteractionScope InteractionScope(TLLRN_ControlRig, ControlElement->GetKey());
		UTLLRN_RigHierarchy* TLLRN_RigHierarchy = TLLRN_ControlRig->GetHierarchy();
		
		//eval any space for this channel, if not additive section
		if (Section->GetBlendType().Get() != EMovieSceneBlendType::Additive)
		{
			TOptional<FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey> SpaceKey = Section->EvaluateSpaceChannel(FrameTime, ControlName);
			if (SpaceKey.IsSet())
			{
				const FTLLRN_RigElementKey ControlKey = ControlElement->GetKey();
				switch (SpaceKey.GetValue().SpaceType)
				{
				case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent:
					TLLRN_ControlRig->SwitchToParent(ControlKey, TLLRN_RigHierarchy->GetDefaultParent(ControlKey), false, true);
					break;
				case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::World:
					TLLRN_ControlRig->SwitchToParent(ControlKey, TLLRN_RigHierarchy->GetWorldSpaceReferenceKey(), false, true);
					break;
				case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig:
					TLLRN_ControlRig->SwitchToParent(ControlKey, SpaceKey.GetValue().TLLRN_ControlTLLRN_RigElement, false, true);
					break;	

				}
			}
		}
		const bool bSetupUndo = false;
		switch (ControlElement->Settings.ControlType)
		{
			case ETLLRN_RigControlType::Bool:
			{
				if (Section->GetBlendType().Get() != EMovieSceneBlendType::Additive)
				{
					TOptional <bool> Value = Section->EvaluateBoolParameter(FrameTime, ControlName);
					if (Value.IsSet())
					{
						TLLRN_ControlRig->SetControlValue<bool>(ControlName, Value.GetValue(), true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);
					}
				}
				break;
			}
			case ETLLRN_RigControlType::Integer:
			{
				if (Section->GetBlendType().Get() != EMovieSceneBlendType::Additive)
				{
					if (ControlElement->Settings.ControlEnum)
					{
						TOptional<uint8> Value = Section->EvaluateEnumParameter(FrameTime, ControlName);
						if (Value.IsSet())
						{
							int32 IVal = (int32)Value.GetValue();
							TLLRN_ControlRig->SetControlValue<int32>(ControlName, IVal, true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);
						}
					}
					else
					{
						TOptional <int32> Value = Section->EvaluateIntegerParameter(FrameTime, ControlName);
						if (Value.IsSet())
						{
							TLLRN_ControlRig->SetControlValue<int32>(ControlName, Value.GetValue(), true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);
						}
					}
				}
				break;

			}
			case ETLLRN_RigControlType::Float:
			case ETLLRN_RigControlType::ScaleFloat:
			{
				TOptional <float> Value = Section->EvaluateScalarParameter(FrameTime, ControlName);
				if (Value.IsSet())
				{
					TLLRN_ControlRig->SetControlValue<float>(ControlName, Value.GetValue(), true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);
				}
				break;
			}
			case ETLLRN_RigControlType::Vector2D:
			{
				TOptional <FVector2D> Value = Section->EvaluateVector2DParameter(FrameTime, ControlName);
				if (Value.IsSet())
				{
					TLLRN_ControlRig->SetControlValue<FVector2D>(ControlName, Value.GetValue(), true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);
				}
				break;
			}
			case ETLLRN_RigControlType::Position:
			case ETLLRN_RigControlType::Scale:
			case ETLLRN_RigControlType::Rotator:
			{
				TOptional <FVector> Value = Section->EvaluateVectorParameter(FrameTime, ControlName);
				if (Value.IsSet())
				{
					FVector3f FloatVal = (FVector3f)Value.GetValue();
					TLLRN_ControlRig->SetControlValue<FVector3f>(ControlName, FloatVal, true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);
				}
				break;
			}

			case ETLLRN_RigControlType::Transform:
			case ETLLRN_RigControlType::TransformNoScale:
			case ETLLRN_RigControlType::EulerTransform:
			{
				// @MikeZ here I suppose we want to retrieve the rotation order and then also extract the Euler angles
				// instead of an assumed FRotator coming from the section?
				// EEulerRotationOrder RotationOrder = SomehowGetRotationOrder();
					
				TOptional <FEulerTransform> Value = Section->EvaluateTransformParameter(FrameTime, ControlName);
				if (Value.IsSet())
				{
					if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Transform)
					{
						FVector EulerAngle(Value.GetValue().Rotation.Roll, Value.GetValue().Rotation.Pitch, Value.GetValue().Rotation.Yaw);
						TLLRN_RigHierarchy->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
						TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FTransform_Float>(ControlName, Value.GetValue().ToFTransform(), true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);
					}
					else if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::TransformNoScale)
					{
						FTransformNoScale NoScale = Value.GetValue().ToFTransform();
						FVector EulerAngle(Value.GetValue().Rotation.Roll, Value.GetValue().Rotation.Pitch, Value.GetValue().Rotation.Yaw);
						TLLRN_RigHierarchy->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
						TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FTransformNoScale_Float>(ControlName, NoScale, true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);
					}
					else if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
					{
						const FEulerTransform& Euler = Value.GetValue();
						FVector EulerAngle(Euler.Rotation.Roll, Euler.Rotation.Pitch, Euler.Rotation.Yaw);
						FQuat Quat = TLLRN_RigHierarchy->GetControlQuaternion(ControlElement, EulerAngle);
						TLLRN_RigHierarchy->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
						FRotator UERotator(Quat);
						FEulerTransform Transform = Euler;
						Transform.Rotation = UERotator;
						TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FEulerTransform_Float>(ControlName, Transform, true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);

					}
				}
				break;

			}
		}
		//note we don't need to evaluate the control rig, setting the value is enough
	}
}

//When a channel is changed via Sequencer we need to call SetControlValue on it so that Control Rig can handle seeing that this is a change, but just on this value
//and then it send back a key if needed, which happens with IK/FK switches. Hopefully new IK/FK system will remove need for this at some point.
//We also compensate since the changed control could happen at a space switch boundary.
//Finally, since they can happen thousands of times interactively when moving a bunch of keys on a control rig we move to doing this into the next tick
struct FChannelChangedStruct
{
	FTimerHandle TimerHandle;
	bool bWasSetAlready = false;
	TMap<UMovieSceneTLLRN_ControlRigParameterSection*, TSet<FName>> SectionControlNames;
};

void FTLLRN_ControlRigParameterTrackEditor::OnChannelChanged(const FMovieSceneChannelMetaData* MetaData, UMovieSceneSection* InSection)
{
	static FChannelChangedStruct ChannelChanged;

	UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection);
	TSharedPtr<ISequencer> SequencerPtr = GetSequencer();

	if (Section && Section->GetTLLRN_ControlRig() && MetaData && SequencerPtr.IsValid())
	{
		TArray<FString> StringArray;
		FString String = MetaData->Name.ToString();
		String.ParseIntoArray(StringArray, TEXT("."));
		if (StringArray.Num() > 0)
		{
			FName ControlName(*StringArray[0]);
			if (ChannelChanged.bWasSetAlready == false)
			{
				ChannelChanged.bWasSetAlready = true;
				ChannelChanged.SectionControlNames.Reset();
				auto ChannelChangedDelegate = [this]()
				{
					TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
					if (!SequencerPtr.IsValid())
					{
						GEditor->GetTimerManager()->ClearTimer(ChannelChanged.TimerHandle);
						return;
					}

					if (!(FSlateApplication::Get().HasAnyMouseCaptor() || GUnrealEd->IsUserInteracting()))
					{
						if (ChannelChanged.TimerHandle.IsValid())
						{
							FFrameTime Time = SequencerPtr->GetLocalTime().Time;
							GEditor->GetTimerManager()->ClearTimer(ChannelChanged.TimerHandle);
							ChannelChanged.TimerHandle.Invalidate();
							ChannelChanged.bWasSetAlready = false;
							TOptional<FFrameNumber> Optional;	
							ISequencer* SequencerRaw = SequencerPtr.Get();
							for (TPair <UMovieSceneTLLRN_ControlRigParameterSection*, TSet<FName>>& Pair : ChannelChanged.SectionControlNames)
							{
								for (FName& ControlName : Pair.Value)
								{
									Pair.Key->ControlsToSet.Empty();
									Pair.Key->ControlsToSet.Add(ControlName);
									//only do the fk/ik hack for bool's since that's only where it's needed
									//otherwise we would incorrectly set values on an addtive section which has scale values of 0.0.
									if (FTLLRN_RigControlElement* ControlElement = Pair.Key->GetTLLRN_ControlRig()->FindControl(ControlName))
									{
										if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Bool)
										{
											EvaluateThisControl(Pair.Key, ControlName, Time);
										}
									}
									Pair.Key->ControlsToSet.Empty();
								}
							}
							ChannelChanged.SectionControlNames.Reset();
						}
					}
				};
				GEditor->GetTimerManager()->SetTimer(ChannelChanged.TimerHandle, ChannelChangedDelegate, 0.01f, true);			
			}
			if (TSet<FName>* SetOfNames = ChannelChanged.SectionControlNames.Find(Section))
			{
				SetOfNames->Add(ControlName);
			}
			else
			{
				TSet<FName> Names;
				Names.Add(ControlName);
				ChannelChanged.SectionControlNames.Add(Section, Names);
			}
		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::AddConstraintToSequencer(const TSharedPtr<ISequencer>& InSequencer, UTickableTransformConstraint* InConstraint)
{
	TGuardValue<bool> DisableTrackCreation(bAutoGenerateTLLRN_ControlRigTrack, false);
	FMovieSceneConstraintChannelHelper::AddConstraintToSequencer(InSequencer, InConstraint);
}

void FTLLRN_ControlRigParameterTrackEditor::AddTrackForComponent(USceneComponent* InComponent, FGuid InBinding) 
{
	if (USkeletalMeshComponent* SkelMeshComp = Cast<USkeletalMeshComponent>(InComponent))
	{
		if(bAutoGenerateTLLRN_ControlRigTrack && !SkelMeshComp->GetDefaultAnimatingRig().IsNull())
		{
			UObject* Object = SkelMeshComp->GetDefaultAnimatingRig().LoadSynchronous();
			if (Object != nullptr && (Object->IsA<UTLLRN_ControlRigBlueprint>() || Object->IsA<UTLLRN_ControlRigComponent>() || Object->IsA<URigVMBlueprintGeneratedClass>()))
			{
				FGuid Binding = InBinding.IsValid() ? InBinding : GetSequencer()->GetHandleToObject(InComponent, true /*bCreateHandle*/);
				if (Binding.IsValid())
				{
					UMovieSceneSequence* OwnerSequence = GetSequencer()->GetFocusedMovieSceneSequence();
					UMovieScene* OwnerMovieScene = OwnerSequence->GetMovieScene();
					UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(OwnerMovieScene->FindTrack(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Binding, NAME_None));
					if (Track == nullptr)
					{
						URigVMBlueprintGeneratedClass* RigClass = nullptr;
						if (UTLLRN_ControlRigBlueprint* BPTLLRN_ControlRig = Cast<UTLLRN_ControlRigBlueprint>(Object))
						{
							RigClass = BPTLLRN_ControlRig->GetRigVMBlueprintGeneratedClass();
						}
						else
						{
							RigClass = Cast<URigVMBlueprintGeneratedClass>(Object);
						}

						if (RigClass)
						{
							if (UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigClass->GetDefaultObject(true /* create if needed */)))
							{
								AddTLLRN_ControlRig(CDO->GetClass(), InComponent, Binding);
							}
						}
					}
				}
			}
		}
	}
	TArray<USceneComponent*> ChildComponents;
	InComponent->GetChildrenComponents(false, ChildComponents);
	for (USceneComponent* ChildComponent : ChildComponents)
	{
		AddTrackForComponent(ChildComponent, FGuid());
	}
}

//test to see if actor has a constraint, in which case we need to add a constraint channel/key
//or a control rig in which case we create a track if cvar is off
void FTLLRN_ControlRigParameterTrackEditor::HandleActorAdded(AActor* Actor, FGuid TargetObjectGuid)
{
	if (Actor == nullptr)
	{
		return;
	}

	//test for constraint
	const FConstraintsManagerController& Controller = FConstraintsManagerController::Get(Actor->GetWorld());
	const TArray< TWeakObjectPtr<UTickableConstraint>> Constraints = Controller.GetAllConstraints();
	for (const TWeakObjectPtr<UTickableConstraint>& WeakConstraint : Constraints)
	{
		if (UTickableTransformConstraint* Constraint = WeakConstraint.IsValid() ? Cast<UTickableTransformConstraint>(WeakConstraint.Get()) : nullptr)
		{
			if (UObject* Child = Constraint->ChildTRSHandle ? Constraint->ChildTRSHandle->GetTarget().Get() : nullptr)
			{
				const AActor* TargetActor = Child->IsA<AActor>() ? Cast<AActor>(Child) : Child->GetTypedOuter<AActor>();
				if (TargetActor == Actor)
				{
					AddConstraintToSequencer(GetSequencer(), Constraint);
				}		
			}
		}
	}
	
	//test for control rig

	if (!CVarAutoGenerateTLLRN_ControlRigTrack.GetValueOnGameThread())
	{
		return;
	}

	if (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent = Actor->FindComponentByClass<UTLLRN_ControlRigComponent>())
	{
		AddTLLRN_ControlRigFromComponent(TargetObjectGuid);
		return;
	}

	if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Actor->GetRootComponent()))
	{
		AddTrackForComponent(SkeletalMeshComponent, TargetObjectGuid);
		return;
	}

	for (UActorComponent* Component : Actor->GetComponents())
	{
		if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
		{
			AddTrackForComponent(SceneComp, FGuid());
		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::OnActivateSequenceChanged(FMovieSceneSequenceIDRef ID)
{
	IterateTracks([this](UMovieSceneTLLRN_ControlRigParameterTrack* Track)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Track->GetTLLRN_ControlRig())
		{
			BindTLLRN_ControlRig(TLLRN_ControlRig);
		}
		
		return false;
	});
	if (bTLLRN_ControlRigEditModeWasOpen && GetSequencer()->IsLevelEditorSequencer())
	{
		GEditor->GetTimerManager()->SetTimerForNextTick([this]()
		{
			//we need to make sure pending deactivated edit modes, including a possible control rig edit mode
			//get totally removed which only happens on a tick 
			if (GEditor->GetActiveViewport() && GEditor->GetActiveViewport()->GetClient())
			{
				if (FEditorModeTools* EditorModeTools = GetEditorModeTools())
				{
					FViewport* ActiveViewport = GEditor->GetActiveViewport();
					FEditorViewportClient* EditorViewClient = (FEditorViewportClient*)ActiveViewport->GetClient();
					EditorModeTools->Tick(EditorViewClient, 0.033f);
				}
			}
			GEditor->GetTimerManager()->SetTimerForNextTick([this]()
			{
				//now we can recreate it
				FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = GetEditMode(true);
				if (TLLRN_ControlRigEditMode)
				{
					bool bSequencerSet = false;
					for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRig : BoundTLLRN_ControlRigs)
					{
						if (TLLRN_ControlRig.IsValid())
						{
							TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TLLRN_ControlRig.Get(), GetSequencer());
							bSequencerSet = true;
							for (int32 Index = 0; Index < PreviousSelectedTLLRN_ControlRigs.Num(); ++Index)
							{
								if (TLLRN_ControlRig.Get()->GetClass() == PreviousSelectedTLLRN_ControlRigs[Index].Key)
								{
									for (const FName& ControlName : PreviousSelectedTLLRN_ControlRigs[Index].Value)
									{
										TLLRN_ControlRig.Get()->SelectControl(ControlName, true);
									}
									PreviousSelectedTLLRN_ControlRigs.RemoveAt(Index);
									break;
								}
							}
						}
					}
					if (bSequencerSet == false)
					{
						TLLRN_ControlRigEditMode->SetSequencer(GetSequencer());
					}
				
				}
				PreviousSelectedTLLRN_ControlRigs.Reset();
			});
		});
	}
}


void FTLLRN_ControlRigParameterTrackEditor::OnSequencerDataChanged(EMovieSceneDataChangeType DataChangeType)
{
	const UMovieScene* MovieScene = GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene();
	FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = GetEditMode();

	//if we have a valid control rig edit mode need to check and see the control rig in that mode is still in a track
	//if not we get rid of it.
	if (TLLRN_ControlRigEditMode && TLLRN_ControlRigEditMode->GetTLLRN_ControlRigsArray(false /*bIsVisible*/).Num() != 0 && MovieScene && (DataChangeType == EMovieSceneDataChangeType::MovieSceneStructureItemRemoved ||
		DataChangeType == EMovieSceneDataChangeType::Unknown))
	{
		TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs = TLLRN_ControlRigEditMode->GetTLLRN_ControlRigsArray(false /*bIsVisible*/);
		const float FPS = (float)GetSequencer()->GetFocusedDisplayRate().AsDecimal();
		for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
		{
			if (TLLRN_ControlRig)
			{
				TLLRN_ControlRig->SetFramesPerSecond(FPS);

				bool bTLLRN_ControlRigInTrack = false;
				IterateTracks([&bTLLRN_ControlRigInTrack, TLLRN_ControlRig](const UMovieSceneTLLRN_ControlRigParameterTrack* Track)
				{
					if(Track->GetTLLRN_ControlRig() == TLLRN_ControlRig)
			        {
						bTLLRN_ControlRigInTrack = true;
						return false;
			        }

					return true;
				});

				if (bTLLRN_ControlRigInTrack == false)
				{
					TLLRN_ControlRigEditMode->RemoveTLLRN_ControlRig(TLLRN_ControlRig);
				}
			}
		}
	}
}


void FTLLRN_ControlRigParameterTrackEditor::PostEvaluation(UMovieScene* MovieScene, FFrameNumber Frame)
{
	if (MovieScene)
	{
		IterateTracksInMovieScene(*MovieScene, [this](const UMovieSceneTLLRN_ControlRigParameterTrack* Track)
		{
			if (const UTLLRN_ControlRig* TLLRN_ControlRig = Track->GetTLLRN_ControlRig())
				{
					if (TLLRN_ControlRig->GetObjectBinding())
					{
						if (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent = Cast<UTLLRN_ControlRigComponent>(TLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
						{
							TLLRN_ControlRigComponent->Update(.1f); //delta time doesn't matter.
					}
				}
			}
			return false;
		});
	}
}

void FTLLRN_ControlRigParameterTrackEditor::OnSelectionChanged(TArray<UMovieSceneTrack*> InTracks)
{
	if (bIsDoingSelection || GetSequencer().IsValid() == false)
	{
		return;
	}

	TGuardValue<bool> Guard(bIsDoingSelection, true);

	if(bSkipNextSelectionFromTimer)
	{
		bSkipNextSelectionFromTimer = false;
		return;
	}

	FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = GetEditMode();
	bool bEditModeExisted = TLLRN_ControlRigEditMode != nullptr;
	UTLLRN_ControlRig* TLLRN_ControlRig = nullptr;

	TArray<const IKeyArea*> KeyAreas;
	const bool UseSelectedKeys = CVarSelectedKeysSelectControls.GetValueOnGameThread();
	GetSequencer()->GetSelectedKeyAreas(KeyAreas, UseSelectedKeys);
	if (KeyAreas.Num() <= 0)
	{ 
		if (FSlateApplication::Get().GetModifierKeys().IsShiftDown() == false && 
			FSlateApplication::Get().GetModifierKeys().IsControlDown() == false && TLLRN_ControlRigEditMode)
		{
			TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> AllSelectedControls;
			TLLRN_ControlRigEditMode->GetAllSelectedControls(AllSelectedControls);
			for (TPair<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>>& SelectedControl : AllSelectedControls)
			{
				TLLRN_ControlRig = SelectedControl.Key;
				if (TLLRN_ControlRig && TLLRN_ControlRig->CurrentControlSelection().Num() > 0)
				{
					FScopedTransaction ScopedTransaction(LOCTEXT("SelectControlTransaction", "Select Control"), !GIsTransacting);
					TLLRN_ControlRig->ClearControlSelection();
				}
			}
		}
		for (UMovieSceneTrack* Track : InTracks)
		{
			UMovieSceneTLLRN_ControlRigParameterTrack* CRTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(Track);
			if (CRTrack)
			{
				UTLLRN_ControlRig* TrackTLLRN_ControlRig = CRTrack->GetTLLRN_ControlRig();
				if (TrackTLLRN_ControlRig)
				{
					if (TLLRN_ControlRigEditMode)
					{
						const bool bAdded = TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TrackTLLRN_ControlRig, GetSequencer());
						if (bAdded)
						{
							TLLRN_ControlRigEditMode->RequestToRecreateControlShapeActors(TrackTLLRN_ControlRig);
						}
						break;
					}
					else
					{
						TLLRN_ControlRigEditMode = GetEditMode(true);
						if (TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TrackTLLRN_ControlRig->GetObjectBinding())
						{
							if (TLLRN_ControlRigEditMode)
							{
								const bool bAdded = TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TrackTLLRN_ControlRig, GetSequencer());
								if (bAdded)
								{
									TLLRN_ControlRigEditMode->RequestToRecreateControlShapeActors(TrackTLLRN_ControlRig);
								}
							}
						}
					}
				}
			}
		}
		const bool bSelectedSectionSetsSectionToKey = CVarSelectedSectionSetsSectionToKey.GetValueOnGameThread();
		if (bSelectedSectionSetsSectionToKey)
		{
			TMap<UMovieSceneTrack*, TSet<UMovieSceneTLLRN_ControlRigParameterSection*>> TracksAndSections;
			using namespace UE::Sequencer;
			for (FViewModelPtr ViewModel : GetSequencer()->GetViewModel()->GetSelection()->Outliner)
			{
				if (TViewModelPtr<FTrackRowModel> TrackRowModel = ViewModel.ImplicitCast())
				{
					for (UMovieSceneSection* Section : TrackRowModel->GetSections())
					{
						if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Section))
						{
							if (UMovieSceneTrack* Track = Section->GetTypedOuter<UMovieSceneTrack>())
							{
								TracksAndSections.FindOrAdd(Track).Add(CRSection);
							}
						}
					}
				}
			}

			//if we have only one  selected section per track and the track has more than one section we set that to the section to key
			for (TPair<UMovieSceneTrack*, TSet<UMovieSceneTLLRN_ControlRigParameterSection*>>& TrackPair : TracksAndSections)
			{
				if (TrackPair.Key->GetAllSections().Num() > 0 && TrackPair.Value.Num() == 1)
				{
					TrackPair.Key->SetSectionToKey(TrackPair.Value.Array()[0]);
				}
			}

		}
		return;
	}
	SelectRigsAndControls(TLLRN_ControlRig, KeyAreas);

	// If the edit mode has been activated, we need to synchronize the external selection (possibly again to account for control rig control actors selection)
	if (!bEditModeExisted && GetEditMode() != nullptr)
	{
		FSequencerUtilities::SynchronizeExternalSelectionWithSequencerSelection(GetSequencer().ToSharedRef());
	}
	
}

void FTLLRN_ControlRigParameterTrackEditor::SelectRigsAndControls(UTLLRN_ControlRig* TLLRN_ControlRig, const TArray<const IKeyArea*>& KeyAreas)
{
	FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = GetEditMode();
	
	//if selection set's section to key we need to keep track of selected sections for each track.
	const bool bSelectedSectionSetsSectionToKey = CVarSelectedSectionSetsSectionToKey.GetValueOnGameThread();
	TMap<UMovieSceneTrack*, TSet<UMovieSceneTLLRN_ControlRigParameterSection*>> TracksAndSections;

	TArray<FString> StringArray;
	//we have two sets here one to see if selection has really changed that contains the attirbutes, the other to select just the parent
	TMap<UTLLRN_ControlRig*, TSet<FName>> RigsAndControls;
	for (const IKeyArea* KeyArea : KeyAreas)
	{
		UMovieSceneTLLRN_ControlRigParameterSection* MovieSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(KeyArea->GetOwningSection());
		if (MovieSection)
		{
			TLLRN_ControlRig = MovieSection->GetTLLRN_ControlRig();
			//Only create the edit mode if we have a KeyAra selected and it's not set and we have some boundobjects.
			if (!TLLRN_ControlRigEditMode)
			{
				TLLRN_ControlRigEditMode = GetEditMode(true);
				if (TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig->GetObjectBinding())
				{
					if (TLLRN_ControlRigEditMode)
					{
						TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TLLRN_ControlRig, GetSequencer());
					}
				}
			}
			else
			{
				if (TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TLLRN_ControlRig, GetSequencer()))
				{
					//force an evaluation, this will get the control rig setup so edit mode looks good.
					if (GetSequencer().IsValid())
					{
						GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::Unknown);
					}
				}
			}

			const FMovieSceneChannelMetaData* MetaData = KeyArea->GetChannel().GetMetaData();
			if (MetaData)
			{
				StringArray.SetNum(0);
				FString String = MetaData->Name.ToString();
				String.ParseIntoArray(StringArray, TEXT("."));
				if (StringArray.Num() > 0)
				{
					const FName ControlName(*StringArray[0]);

					// skip nested controls which have the shape enabled flag turned on
					if(const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
					{

						if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control)))
						{
						
							if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Bool ||
								ControlElement->Settings.ControlType == ETLLRN_RigControlType::Float ||
								ControlElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat ||
								ControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer)
							{
								if (ControlElement->Settings.SupportsShape() || !Hierarchy->IsAnimatable(ControlElement))
								{

									if (const FTLLRN_RigControlElement* ParentControlElement = Cast<FTLLRN_RigControlElement>(Hierarchy->GetFirstParent(ControlElement)))
									{
										if (const TSet<FName>* Controls = RigsAndControls.Find(TLLRN_ControlRig))
										{
											if (Controls->Contains(ParentControlElement->GetFName()))
											{
												continue;
											}
										}
									}
								}
							}
							RigsAndControls.FindOrAdd(TLLRN_ControlRig).Add(ControlName);
						}
					}
				}
			}
			if (bSelectedSectionSetsSectionToKey)
			{
				if (UMovieSceneTrack* Track = MovieSection->GetTypedOuter<UMovieSceneTrack>())
				{
					TracksAndSections.FindOrAdd(Track).Add(MovieSection);
				}
			}
		}
	}

	//only create transaction if selection is really different.
	bool bEndTransaction = false;
	
	TMap<UTLLRN_ControlRig*, TArray<FName>> TLLRN_ControlRigsToClearSelection;
	//get current selection which we will clear if different
	if (TLLRN_ControlRigEditMode)
	{
		TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> SelectedControls;
		TLLRN_ControlRigEditMode->GetAllSelectedControls(SelectedControls);
		for (TPair<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>>& Selection : SelectedControls)
		{
			TLLRN_ControlRig = Selection.Key;
			if (TLLRN_ControlRig)
			{
				TArray<FName> SelectedControlNames = TLLRN_ControlRig->CurrentControlSelection();
				TLLRN_ControlRigsToClearSelection.Add(TLLRN_ControlRig, SelectedControlNames);
			}
		}
	}

	for (TPair<UTLLRN_ControlRig*, TSet<FName>>& Pair : RigsAndControls)
	{
		//check to see if new selection is same als old selection
		bool bIsSame = true;
		if (TArray<FName>* SelectedNames = TLLRN_ControlRigsToClearSelection.Find(Pair.Key))
		{
			TSet<FName>* FullNames = RigsAndControls.Find(Pair.Key);
			if (!FullNames)
			{
				continue; // should never happen
			}
			if (SelectedNames->Num() != FullNames->Num())
			{ 
				bIsSame = false;
				if (!GIsTransacting && bEndTransaction == false)
				{
					bEndTransaction = true;
					GEditor->BeginTransaction(LOCTEXT("SelectControl", "Select Control"));
				}
				Pair.Key->ClearControlSelection();
				TLLRN_ControlRigsToClearSelection.Remove(Pair.Key); //remove it
			}
			else//okay if same check and see if equal...
			{
				for (const FName& Name : (*SelectedNames))
				{
					if (FullNames->Contains(Name) == false)
					{
						bIsSame = false;
						if (!GIsTransacting && bEndTransaction == false)
						{
							bEndTransaction = true;
							GEditor->BeginTransaction(LOCTEXT("SelectControl", "Select Control"));
						}
						Pair.Key->ClearControlSelection();
						TLLRN_ControlRigsToClearSelection.Remove(Pair.Key); //remove it
						break; //break out
					}
				}
			}
			if (bIsSame == true)
			{
				TLLRN_ControlRigsToClearSelection.Remove(Pair.Key); //remove it
			}
		}
		else
		{
			bIsSame = false;
		}
		if (bIsSame == false)
		{
			for (const FName& Name : Pair.Value)
			{
				if (!GIsTransacting && bEndTransaction == false)
				{
					bEndTransaction = true;
					GEditor->BeginTransaction(LOCTEXT("SelectControl", "Select Control"));
				}
				Pair.Key->SelectControl(Name, true);
			}
		}
	}
	//go through and clear those still not cleared
	for (TPair<UTLLRN_ControlRig*, TArray<FName>>& SelectedPairs : TLLRN_ControlRigsToClearSelection)
	{
		if (!GIsTransacting && bEndTransaction == false)
		{
			bEndTransaction = true;
			GEditor->BeginTransaction(LOCTEXT("SelectControl", "Select Control"));
		}
		SelectedPairs.Key->ClearControlSelection();
	}
	//if we have only one  selected section per track and the track has more than one section we set that to the section to key
	for (TPair<UMovieSceneTrack*, TSet<UMovieSceneTLLRN_ControlRigParameterSection*>>& TrackPair : TracksAndSections)
	{
		if (TrackPair.Key->GetAllSections().Num() > 0 && TrackPair.Value.Num() == 1)
		{
			TrackPair.Key->SetSectionToKey(TrackPair.Value.Array()[0]);
		}
	}
	if (bEndTransaction)
	{
		GEditor->EndTransaction();
	}
}


FMovieSceneTrackEditor::FFindOrCreateHandleResult FTLLRN_ControlRigParameterTrackEditor::FindOrCreateHandleToObject(UObject* InObj, UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	const bool bCreateHandleIfMissing = false;
	FName CreatedFolderName = NAME_None;

	FFindOrCreateHandleResult Result;
	bool bHandleWasValid = GetSequencer()->GetHandleToObject(InObj, bCreateHandleIfMissing).IsValid();

	Result.Handle = GetSequencer()->GetHandleToObject(InObj, bCreateHandleIfMissing, CreatedFolderName);
	Result.bWasCreated = bHandleWasValid == false && Result.Handle.IsValid();

	const UMovieScene* MovieScene = GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene();

	// Prioritize a control rig parameter track on this component if it matches the handle
	if (Result.Handle.IsValid())
	{
		if (UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(MovieScene->FindTrack(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), Result.Handle, NAME_None)))
		{
			if (InTLLRN_ControlRig == nullptr || (Track->GetTLLRN_ControlRig() == InTLLRN_ControlRig))
			{
				return Result;
			}
		}
	}

	// If the owner has a control rig parameter track, let's use it
	if (const USceneComponent* SceneComponent = Cast<USceneComponent>(InObj))
	{
		// If the owner has a control rig parameter track, let's use it
		UObject* OwnerObject = SceneComponent->GetOwner();
		const FGuid OwnerHandle = GetSequencer()->GetHandleToObject(OwnerObject, bCreateHandleIfMissing);
	    bHandleWasValid = OwnerHandle.IsValid();
	    if (OwnerHandle.IsValid())
	    {
		    if (UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(MovieScene->FindTrack(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), OwnerHandle, NAME_None)))
		    {
			    if (InTLLRN_ControlRig == nullptr || (Track->GetTLLRN_ControlRig() == InTLLRN_ControlRig))
			    {
				    Result.Handle = OwnerHandle;
				    Result.bWasCreated = bHandleWasValid == false && Result.Handle.IsValid();
				    return Result;
			    }
		    }
	    }
    
	    // If the component handle doesn't exist, let's use the owner handle
	    if (Result.Handle.IsValid() == false)
	    {
		    Result.Handle = OwnerHandle;
		    Result.bWasCreated = bHandleWasValid == false && Result.Handle.IsValid();
		}
	}
	return Result;
}

void FTLLRN_ControlRigParameterTrackEditor::SelectSequencerNodeInSection(UMovieSceneTLLRN_ControlRigParameterSection* ParamSection, const FName& ControlName, bool bSelected)
{
	if (ParamSection)
	{
		FTLLRN_ChannelMapInfo* pChannelIndex = ParamSection->ControlChannelMap.Find(ControlName);
		if (pChannelIndex != nullptr)
		{
			if (pChannelIndex->ParentControlIndex == INDEX_NONE)
			{
				int32 CategoryIndex = ParamSection->GetActiveCategoryIndex(ControlName);
				if (CategoryIndex != INDEX_NONE)
				{
					GetSequencer()->SelectByNthCategoryNode(ParamSection, CategoryIndex, bSelected);
				}
			}
			else
			{
				const FName FloatChannelTypeName = FMovieSceneFloatChannel::StaticStruct()->GetFName();

				FMovieSceneChannelProxy& ChannelProxy = ParamSection->GetChannelProxy();
				for (const FMovieSceneChannelEntry& Entry : ParamSection->GetChannelProxy().GetAllEntries())
				{
					const FName ChannelTypeName = Entry.GetChannelTypeName();
					if (pChannelIndex->ChannelTypeName == ChannelTypeName || (ChannelTypeName == FloatChannelTypeName && pChannelIndex->ChannelTypeName == NAME_None))
					{
						FMovieSceneChannelHandle Channel = ChannelProxy.MakeHandle(ChannelTypeName, pChannelIndex->ChannelIndex);
						TArray<FMovieSceneChannelHandle> Channels;
						Channels.Add(Channel);
						GetSequencer()->SelectByChannels(ParamSection, Channels, false, bSelected);
						break;
					}
				}
			}
		}
	}
}

FMovieSceneTrackEditor::FFindOrCreateTrackResult FTLLRN_ControlRigParameterTrackEditor::FindOrCreateTLLRN_ControlRigTrackForObject(FGuid ObjectHandle, UTLLRN_ControlRig* TLLRN_ControlRig, FName PropertyName, bool bCreateTrackIfMissing)
{
	FFindOrCreateTrackResult Result;
	bool bTrackExisted = false;

	IterateTracks([&Result, &bTrackExisted, TLLRN_ControlRig](UMovieSceneTLLRN_ControlRigParameterTrack* Track)
	{
		if (Track->GetTLLRN_ControlRig() == TLLRN_ControlRig)
		{
			Result.Track = Track;
			bTrackExisted = true;
		}
		return false;
	});

	// Only create track if the object handle is valid
	if (!Result.Track && bCreateTrackIfMissing && ObjectHandle.IsValid())
	{
		UMovieScene* MovieScene = GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene();
		Result.Track = AddTrack(MovieScene, ObjectHandle, UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), PropertyName);
	}

	Result.bWasCreated = bTrackExisted == false && Result.Track != nullptr;

	return Result;
}

UMovieSceneTLLRN_ControlRigParameterTrack* FTLLRN_ControlRigParameterTrackEditor::FindTrack(const UTLLRN_ControlRig* InTLLRN_ControlRig) const
{
	if (!GetSequencer().IsValid())
	{
		return nullptr;
	}
	
	UMovieSceneTLLRN_ControlRigParameterTrack* FoundTrack = nullptr;
	IterateTracks([InTLLRN_ControlRig, &FoundTrack](UMovieSceneTLLRN_ControlRigParameterTrack* Track)
	{
		if (Track->GetTLLRN_ControlRig() == InTLLRN_ControlRig)
		{
			FoundTrack = Track;
			return false;
		}
		return true;
	});

	return FoundTrack;
}

void FTLLRN_ControlRigParameterTrackEditor::HandleOnSpaceAdded(UMovieSceneTLLRN_ControlRigParameterSection* Section, const FName& ControlName, FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* SpaceChannel)
{
	if (SpaceChannel)
	{
		if (!SpaceChannel->OnKeyMovedEvent().IsBound())
		{
			SpaceChannel->OnKeyMovedEvent().AddLambda([this, Section](FMovieSceneChannel* Channel, const  TArray<FKeyMoveEventItem>& MovedItems)
				{
					FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* SpaceChannel = static_cast<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel*>(Channel);
					HandleSpaceKeyMoved(Section, SpaceChannel, MovedItems);
				});
		}
		if (!SpaceChannel->OnKeyDeletedEvent().IsBound())
		{
			SpaceChannel->OnKeyDeletedEvent().AddLambda([this, Section](FMovieSceneChannel* Channel, const  TArray<FKeyAddOrDeleteEventItem>& Items)
				{
					FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* SpaceChannel = static_cast<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel*>(Channel);
					HandleSpaceKeyDeleted(Section, SpaceChannel, Items);
				});
		}
	}
	//todoo do we need to remove this or not mz
}

bool FTLLRN_ControlRigParameterTrackEditor::MatchesContext(const FTransactionContext& InContext, const TArray<TPair<UObject*, FTransactionObjectEvent>>& TransactionObjects) const
{
	SectionsGettingUndone.SetNum(0);
	// Check if we care about the undo/redo
	bool bGettingUndone = false;
	for (const TPair<UObject*, FTransactionObjectEvent>& TransactionObjectPair : TransactionObjects)
	{

		UObject* Object = TransactionObjectPair.Key;
		while (Object != nullptr)
		{
			const UClass* ObjectClass = Object->GetClass();
			if (ObjectClass && ObjectClass->IsChildOf(UMovieSceneTLLRN_ControlRigParameterSection::StaticClass()))
			{
				UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast< UMovieSceneTLLRN_ControlRigParameterSection>(Object);
				if (Section)
				{
					SectionsGettingUndone.Add(Section);
				}
				bGettingUndone =  true;
				break;
			}
			Object = Object->GetOuter();
		}
	}
	
	return bGettingUndone;
}

void FTLLRN_ControlRigParameterTrackEditor::PostUndo(bool bSuccess)
{
	for (UMovieSceneTLLRN_ControlRigParameterSection* Section : SectionsGettingUndone)
	{
		if (Section->GetTLLRN_ControlRig())
		{
			TArray<FTLLRN_SpaceControlNameAndChannel>& SpaceChannels = Section->GetSpaceChannels();
			for(FTLLRN_SpaceControlNameAndChannel& Channel: SpaceChannels)
			{ 
				HandleOnSpaceAdded(Section, Channel.ControlName, &(Channel.SpaceCurve));
			}

			TArray<FConstraintAndActiveChannel>& ConstraintChannels = Section->GetConstraintsChannels();
			for (FConstraintAndActiveChannel& Channel: ConstraintChannels)
			{ 
				HandleOnConstraintAdded(Section, &(Channel.ActiveChannel));
			}
		}
	}
}


void FTLLRN_ControlRigParameterTrackEditor::HandleSpaceKeyDeleted(
	UMovieSceneTLLRN_ControlRigParameterSection* Section,
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel,
	const TArray<FKeyAddOrDeleteEventItem>& DeletedItems) const
{
	const TSharedPtr<ISequencer> ParentSequencer = GetSequencer();

	if (Section && Section->GetTLLRN_ControlRig() && Channel && ParentSequencer.IsValid())
	{
		const FName ControlName = Section->FindControlNameFromSpaceChannel(Channel);
		for (const FKeyAddOrDeleteEventItem& EventItem : DeletedItems)
		{
			FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerSpaceChannelKeyDeleted(
				Section->GetTLLRN_ControlRig(), ParentSequencer.Get(), ControlName, Channel, Section,EventItem.Frame);
		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::HandleSpaceKeyMoved(
	UMovieSceneTLLRN_ControlRigParameterSection* Section,
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* SpaceChannel,
	const  TArray<FKeyMoveEventItem>& MovedItems)
{
	if (Section && Section->GetTLLRN_ControlRig() && SpaceChannel)
	{
		const FName ControlName = Section->FindControlNameFromSpaceChannel(SpaceChannel);
		for (const FKeyMoveEventItem& MoveEventItem : MovedItems)
		{
			FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::HandleSpaceKeyTimeChanged(
				Section->GetTLLRN_ControlRig(), ControlName, SpaceChannel, Section,
				MoveEventItem.Frame, MoveEventItem.NewFrame);
		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::ClearOutAllSpaceAndConstraintDelegates(const UTLLRN_ControlRig* InOptionalTLLRN_ControlRig) const
{
	UTickableTransformConstraint::GetOnConstraintChanged().RemoveAll(this);
	
	const UMovieScene* MovieScene = GetSequencer().IsValid() && GetSequencer()->GetFocusedMovieSceneSequence() ? GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene() : nullptr;
	if (!MovieScene)
	{
		return;
	}

	IterateTracks([InOptionalTLLRN_ControlRig](const UMovieSceneTLLRN_ControlRigParameterTrack* Track)
	{
		if (InOptionalTLLRN_ControlRig && Track->GetTLLRN_ControlRig() != InOptionalTLLRN_ControlRig)
		{
			return true;
		}
				
		for (UMovieSceneSection* Section : Track->GetAllSections())
		{
			if (UMovieSceneTLLRN_ControlRigParameterSection* CRSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Section))
			{
				// clear space channels
				TArray<FTLLRN_SpaceControlNameAndChannel>& Channels = CRSection->GetSpaceChannels();
				for (FTLLRN_SpaceControlNameAndChannel& SpaceAndChannel : Channels)
				{
					SpaceAndChannel.SpaceCurve.OnKeyMovedEvent().Clear();
					SpaceAndChannel.SpaceCurve.OnKeyDeletedEvent().Clear();
				}

				// clear constraint channels
				TArray<FConstraintAndActiveChannel>& ConstraintChannels = CRSection->GetConstraintsChannels();
				for (FConstraintAndActiveChannel& Channel: ConstraintChannels)
				{
					Channel.ActiveChannel.OnKeyMovedEvent().Clear();
					Channel.ActiveChannel.OnKeyDeletedEvent().Clear();
				}

				if (CRSection->OnConstraintRemovedHandle.IsValid())
				{
					if (const UTLLRN_ControlRig* TLLRN_ControlRig = CRSection->GetTLLRN_ControlRig())
					{
						FConstraintsManagerController& Controller = FConstraintsManagerController::Get(TLLRN_ControlRig->GetWorld());
						Controller.GetNotifyDelegate().Remove(CRSection->OnConstraintRemovedHandle);
						CRSection->OnConstraintRemovedHandle.Reset();
					}
				}
			}
		}

		return false;
	});
}

namespace
{
	struct FConstraintAndControlData
	{
		static FConstraintAndControlData CreateFromSection(
			const UMovieSceneTLLRN_ControlRigParameterSection* InSection,
			const FMovieSceneConstraintChannel* InConstraintChannel)
		{
			FConstraintAndControlData Data;
			
			// get constraint channel
			const TArray<FConstraintAndActiveChannel>& ConstraintChannels = InSection->GetConstraintsChannels();
			const int32 Index = ConstraintChannels.IndexOfByPredicate([InConstraintChannel](const FConstraintAndActiveChannel& InChannel)
			{
				return &(InChannel.ActiveChannel) == InConstraintChannel;
			});
	
			if (Index == INDEX_NONE)
			{
				return Data;
			}

			Data.Constraint = Cast<UTickableTransformConstraint>(ConstraintChannels[Index].GetConstraint().Get());

			// get constraint name
			auto GetControlName = [InSection, Index]()
			{
				using NameInfoIterator = TMap<FName, FTLLRN_ChannelMapInfo>::TRangedForConstIterator;
				for (NameInfoIterator It = InSection->ControlChannelMap.begin(); It; ++It)
				{
					const FTLLRN_ChannelMapInfo& Info = It->Value;
					if (Info.ConstraintsIndex.Contains(Index))
					{
						return It->Key;
					}
				}

				static const FName DummyName = NAME_None;
				return DummyName;
			};
	
			Data.ControlName = GetControlName();
			
			return Data;
		}

		bool IsValid() const
		{
			return Constraint.IsValid() && ControlName != NAME_None; 
		}
		
		TWeakObjectPtr<UTickableTransformConstraint> Constraint = nullptr;
		FName ControlName = NAME_None;
	};
}

void FTLLRN_ControlRigParameterTrackEditor::HandleOnConstraintAdded(
	IMovieSceneConstrainedSection* InSection,
	FMovieSceneConstraintChannel* InConstraintChannel)
{
	if (!InConstraintChannel)
	{
		return;
	}

	// handle key moved
	if (!InConstraintChannel->OnKeyMovedEvent().IsBound())
	{
		InConstraintChannel->OnKeyMovedEvent().AddLambda([this, InSection](
			FMovieSceneChannel* InChannel, const TArray<FKeyMoveEventItem>& InMovedItems)
			{
				const FMovieSceneConstraintChannel* ConstraintChannel = static_cast<FMovieSceneConstraintChannel*>(InChannel);
				HandleConstraintKeyMoved(InSection, ConstraintChannel, InMovedItems);
			});
	}

	// handle key deleted
	if (!InConstraintChannel->OnKeyDeletedEvent().IsBound())
	{
		InConstraintChannel->OnKeyDeletedEvent().AddLambda([this, InSection](
			FMovieSceneChannel* InChannel, const TArray<FKeyAddOrDeleteEventItem>& InDeletedItems)
			{
				const FMovieSceneConstraintChannel* ConstraintChannel = static_cast<FMovieSceneConstraintChannel*>(InChannel);
				HandleConstraintKeyDeleted(InSection, ConstraintChannel, InDeletedItems);
			});
	}

	// handle constraint deleted
	if (InSection)
	{
		HandleConstraintRemoved(InSection);
	}

	if (!UTickableTransformConstraint::GetOnConstraintChanged().IsBoundToObject(this))
	{
		UTickableTransformConstraint::GetOnConstraintChanged().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::HandleConstraintPropertyChanged);
	}
}

void FTLLRN_ControlRigParameterTrackEditor::HandleConstraintKeyDeleted(
	IMovieSceneConstrainedSection* InSection,
	const FMovieSceneConstraintChannel* InConstraintChannel,
	const TArray<FKeyAddOrDeleteEventItem>& InDeletedItems) const
{
	if (FMovieSceneConstraintChannelHelper::bDoNotCompensate)
	{
		return;
	}
	
	UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection);

	const UTLLRN_ControlRig* TLLRN_ControlRig = Section ? Section->GetTLLRN_ControlRig() : nullptr;
	if (!TLLRN_ControlRig || !InConstraintChannel)
	{
		return;
	}
	
	const FConstraintAndControlData ConstraintAndControlData =
		FConstraintAndControlData::CreateFromSection(Section, InConstraintChannel);
	if (ConstraintAndControlData.IsValid())
	{

		UTickableTransformConstraint* Constraint = ConstraintAndControlData.Constraint.Get();
		for (const FKeyAddOrDeleteEventItem& EventItem: InDeletedItems)
		{
			FMovieSceneConstraintChannelHelper::HandleConstraintKeyDeleted(
				Constraint, InConstraintChannel,
				GetSequencer(), Section,
				EventItem.Frame);
		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::HandleConstraintKeyMoved(
	IMovieSceneConstrainedSection* InSection,
	const FMovieSceneConstraintChannel* InConstraintChannel,
	const TArray<FKeyMoveEventItem>& InMovedItems)
{
	UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection);

	const FConstraintAndControlData ConstraintAndControlData =
	FConstraintAndControlData::CreateFromSection(Section, InConstraintChannel);

	if (ConstraintAndControlData.IsValid())
	{
		const UTickableTransformConstraint* Constraint = ConstraintAndControlData.Constraint.Get();
		for (const FKeyMoveEventItem& MoveEventItem : InMovedItems)
		{
			FMovieSceneConstraintChannelHelper::HandleConstraintKeyMoved(
				Constraint, InConstraintChannel, Section,
				MoveEventItem.Frame, MoveEventItem.NewFrame);
		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::HandleConstraintRemoved(IMovieSceneConstrainedSection* InSection) 
{
	UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(InSection);

	if (const UTLLRN_ControlRig* TLLRN_ControlRig = Section->GetTLLRN_ControlRig())
	{
		FConstraintsManagerController& Controller = FConstraintsManagerController::Get(TLLRN_ControlRig->GetWorld());
		if (!InSection->OnConstraintRemovedHandle.IsValid())
		{
			InSection->OnConstraintRemovedHandle =
			Controller.GetNotifyDelegate().AddLambda(
				[InSection,Section, this](EConstraintsManagerNotifyType InNotifyType, UObject *InObject)
			{
				switch (InNotifyType)
				{
					case EConstraintsManagerNotifyType::ConstraintAdded:
						break;
					case EConstraintsManagerNotifyType::ConstraintRemoved:
					case EConstraintsManagerNotifyType::ConstraintRemovedWithCompensation:
						{
							const UTickableConstraint* Constraint = Cast<UTickableConstraint>(InObject);
							if (!IsValid(Constraint))
							{
								return;
							}

							const FConstraintAndActiveChannel* ConstraintChannel = InSection->GetConstraintChannel(Constraint->ConstraintID);
							if (!ConstraintChannel || ConstraintChannel->GetConstraint().Get() != Constraint)
							{
								return;
							}

							TSharedPtr<ISequencer> Sequencer = GetSequencer();
							
							const bool bCompensate = (InNotifyType == EConstraintsManagerNotifyType::ConstraintRemovedWithCompensation);
							if (bCompensate && ConstraintChannel->GetConstraint().Get())
							{
								FMovieSceneConstraintChannelHelper::HandleConstraintRemoved(
									ConstraintChannel->GetConstraint().Get(),
									&ConstraintChannel->ActiveChannel,
									Sequencer,
									Section);
							}

							InSection->RemoveConstraintChannel(Constraint);

							if (Sequencer)
							{
								Sequencer->RecreateCurveEditor();
							}
						}
						break;
					case EConstraintsManagerNotifyType::ManagerUpdated:
						InSection->OnConstraintsChanged();
						break;		
				}
			});
			ConstraintHandlesToClear.Add(InSection->OnConstraintRemovedHandle);
		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::HandleConstraintPropertyChanged(UTickableTransformConstraint* InConstraint, const FPropertyChangedEvent& InPropertyChangedEvent) const
{
	if (!IsValid(InConstraint))
	{
		return;
	}

	// find constraint section
	const UTLLRN_TransformableControlHandle* Handle = Cast<UTLLRN_TransformableControlHandle>(InConstraint->ChildTRSHandle);
	if (!IsValid(Handle) || !Handle->IsValid())
	{
		return;
	}

	const FConstraintChannelInterfaceRegistry& InterfaceRegistry = FConstraintChannelInterfaceRegistry::Get();	
	ITransformConstraintChannelInterface* Interface = InterfaceRegistry.FindConstraintChannelInterface(Handle->GetClass());
	if (!Interface)
	{
		return;
	}
	
	UMovieSceneSection* Section = Interface->GetHandleConstraintSection(Handle, GetSequencer());
	IMovieSceneConstrainedSection* ConstraintSection = Cast<IMovieSceneConstrainedSection>(Section);
	if (!ConstraintSection)
	{
		return;
	}

	// find corresponding channel
	const TArray<FConstraintAndActiveChannel>& ConstraintChannels = ConstraintSection->GetConstraintsChannels();
	const FConstraintAndActiveChannel* Channel = ConstraintChannels.FindByPredicate([InConstraint](const FConstraintAndActiveChannel& Channel)
	{
		return Channel.GetConstraint() == InConstraint;
	});

	if (!Channel)
	{
		return;
	}

	FMovieSceneConstraintChannelHelper::HandleConstraintPropertyChanged(
			InConstraint, Channel->ActiveChannel, InPropertyChangedEvent, GetSequencer(), Section);
}

void FTLLRN_ControlRigParameterTrackEditor::SetUpEditModeIfNeeded(UTLLRN_ControlRig* TLLRN_ControlRig)
{
	if (TLLRN_ControlRig)
	{
		//this could clear the selection so if it does reset it
		TArray<FName> TLLRN_ControlRigSelection = TLLRN_ControlRig->CurrentControlSelection();

		FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = GetEditMode();
		if (!TLLRN_ControlRigEditMode)
		{
			TLLRN_ControlRigEditMode = GetEditMode(true);
			if (TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig->GetObjectBinding())
			{
				if (TLLRN_ControlRigEditMode)
				{
					TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TLLRN_ControlRig, GetSequencer());
				}
			}
		}
		else
		{
			if (TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TLLRN_ControlRig, GetSequencer()))
			{
				//force an evaluation, this will get the control rig setup so edit mode looks good.
				if (GetSequencer().IsValid())
				{
					GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::Unknown);
				}
			}
		}
		TArray<FName> NewTLLRN_ControlRigSelection = TLLRN_ControlRig->CurrentControlSelection();
		if (TLLRN_ControlRigSelection.Num() != NewTLLRN_ControlRigSelection.Num())
		{
			TLLRN_ControlRig->ClearControlSelection();
			for (const FName& Name : TLLRN_ControlRigSelection)
			{
				TLLRN_ControlRig->SelectControl(Name, true);
			}
		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* ControlElement, bool bSelected)
{
	using namespace UE::Sequencer;

	if(ControlElement == nullptr)
	{
		return;
	}
	
	UTLLRN_RigHierarchy* Hierarchy = Subject->GetHierarchy();
	static bool bIsSelectingIndirectControl = false;
	static TArray<FTLLRN_RigControlElement*> SelectedElements = {};

	// Avoid cyclic selection
	if (SelectedElements.Contains(ControlElement))
	{
		return;
	}

	if(ControlElement->CanDriveControls())
	{
		const TArray<FTLLRN_RigElementKey>& DrivenControls = ControlElement->Settings.DrivenControls;
		for(const FTLLRN_RigElementKey& DrivenKey : DrivenControls)
		{
			if(FTLLRN_RigControlElement* DrivenControl = Hierarchy->Find<FTLLRN_RigControlElement>(DrivenKey))
			{
				TGuardValue<bool> SubControlGuard(bIsSelectingIndirectControl, true);

				TArray<FTLLRN_RigControlElement*> NewSelection = SelectedElements;
				NewSelection.Add(ControlElement);
				TGuardValue<TArray<FTLLRN_RigControlElement*>> SelectedElementsGuard(SelectedElements, NewSelection);
				
				HandleControlSelected(Subject, DrivenControl, bSelected);
			}
		}
		if(ControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::ProxyControl)
		{
			return;
		}
	}
	
	//if parent selected we select child here if it's a bool,integer or single float
	TArray<FTLLRN_RigControl> Controls;

	if(!bIsSelectingIndirectControl)
	{
		if (UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController())
		{
			Hierarchy->ForEach<FTLLRN_RigControlElement>([Hierarchy, ControlElement, Controller, bSelected](FTLLRN_RigControlElement* OtherControlElement) -> bool
				{
					if (OtherControlElement->Settings.ControlType == ETLLRN_RigControlType::Bool ||
						OtherControlElement->Settings.ControlType == ETLLRN_RigControlType::Float ||
						OtherControlElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat ||
						OtherControlElement->Settings.ControlType == ETLLRN_RigControlType::Integer)
					{
						if(OtherControlElement->Settings.SupportsShape() || !Hierarchy->IsAnimatable(OtherControlElement))
						{
							return true;
						}
						
						for (const FTLLRN_RigElementParentConstraint& ParentConstraint : OtherControlElement->ParentConstraints)
						{
							if (ParentConstraint.ParentElement == ControlElement)
							{
								Controller->SelectElement(OtherControlElement->GetKey(), bSelected);
								return true;
							}
						}
					}

					if(OtherControlElement->IsAnimationChannel() && OtherControlElement->Settings.Customization.AvailableSpaces.Contains(ControlElement->GetKey()))
					{
						Controller->SelectElement(OtherControlElement->GetKey(), bSelected);
						return true;
					}

					return true;
				});
		}
	}
	
	if (bIsDoingSelection)
	{
		return;
	}
	TGuardValue<bool> Guard(bIsDoingSelection, true);

	FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = GetEditMode();

	const FName TLLRN_ControlTLLRN_RigName(*Subject->GetName());
	if (TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = Subject->GetObjectBinding())
	{
		UObject* Object = ObjectBinding->GetBoundObject();
		if (!Object)
		{
			return;
		}

		const bool bCreateTrack = false;
		const FFindOrCreateHandleResult HandleResult = FindOrCreateHandleToObject(Object, Subject);
		FFindOrCreateTrackResult TrackResult = FindOrCreateTLLRN_ControlRigTrackForObject(HandleResult.Handle, Subject, TLLRN_ControlTLLRN_RigName, bCreateTrack);
		UMovieSceneTLLRN_ControlRigParameterTrack* Track = CastChecked<UMovieSceneTLLRN_ControlRigParameterTrack>(TrackResult.Track, ECastCheckedType::NullAllowed);
		if (Track)
		{
			FSelectionEventSuppressor SuppressSelectionEvents = GetSequencer()->GetViewModel()->GetSelection()->SuppressEvents();

			//Just select in section to key, if deselecting makes sure deselected everywhere
			if (bSelected == true)
			{
				UMovieSceneSection* Section = Track->GetSectionToKey(ControlElement->GetFName());
				UMovieSceneTLLRN_ControlRigParameterSection* ParamSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Section);
				SelectSequencerNodeInSection(ParamSection, ControlElement->GetFName(), bSelected);
			}
			else
			{
				for (UMovieSceneSection* BaseSection : Track->GetAllSections())
				{
					if (UMovieSceneTLLRN_ControlRigParameterSection* ParamSection = Cast< UMovieSceneTLLRN_ControlRigParameterSection>(BaseSection))
					{
						SelectSequencerNodeInSection(ParamSection, ControlElement->GetFName(), bSelected);
					}
				}
			}

			SetUpEditModeIfNeeded(Subject);

			//Force refresh later, not now
			bSkipNextSelectionFromTimer = bSkipNextSelectionFromTimer ||
				(bIsSelectingIndirectControl && ControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::AnimationControl);
			GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::RefreshTree);

		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::HandleOnPostConstructed(UTLLRN_ControlRig* Subject, const FName& InEventName)
{
	if (IsInGameThread())
	{
		UTLLRN_ControlRig* TLLRN_ControlRig = CastChecked<UTLLRN_ControlRig>(Subject);

		if (GetSequencer().IsValid())
		{
			//refresh tree for ANY control rig may be FK or procedural
			GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::RefreshTree);
		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::HandleControlModified(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context)
{
	if(ControlElement == nullptr)
	{
		return;
	}
	if (IsInGameThread() == false)
	{
		return;
	}
	if (!GetSequencer().IsValid() || !GetSequencer()->IsAllowedToChange() || Context.SetKey == ETLLRN_ControlRigSetKey::Never 
		|| ControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::ProxyControl
		|| ControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::VisualCue)
	{
		return;
	}
	FTransform  Transform = TLLRN_ControlRig->GetControlLocalTransform(ControlElement->GetFName());
	UMovieScene* MovieScene = GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene();

	IterateTracks([this, TLLRN_ControlRig, ControlElement, Context](UMovieSceneTLLRN_ControlRigParameterTrack* Track)
	{
		if (Track && Track->GetTLLRN_ControlRig() == TLLRN_ControlRig)
		{
			FName Name(*TLLRN_ControlRig->GetName());
			if (TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig->GetObjectBinding())
			{
				if (UObject* Object = ObjectBinding->GetBoundObject())
				{
					ESequencerKeyMode KeyMode = ESequencerKeyMode::AutoKey;
					if (Context.SetKey == ETLLRN_ControlRigSetKey::Always)
					{
						KeyMode = ESequencerKeyMode::ManualKeyForced;
					}
					AddControlKeys(Object, TLLRN_ControlRig, Name, ControlElement->GetFName(), (ETLLRN_ControlRigContextChannelToKey)Context.KeyMask, 
						KeyMode, Context.LocalTime);
					ControlChangedDuringUndoBracket++;
					return true;
				}
			}
		}
		return false;
	});
}

void FTLLRN_ControlRigParameterTrackEditor::HandleControlUndoBracket(UTLLRN_ControlRig* Subject, bool bOpenUndoBracket)
{
	if(IsInGameThread() && bOpenUndoBracket && ControlUndoBracket == 0)
	{
		FScopeLock ScopeLock(&ControlUndoTransactionMutex);
		ControlUndoTransaction = MakeShareable(new FScopedTransaction(LOCTEXT("KeyMultipleControls", "Auto-Key multiple controls")));
		ControlChangedDuringUndoBracket = 0;
	}

	ControlUndoBracket = FMath::Max<int32>(0, ControlUndoBracket + (bOpenUndoBracket ? 1 : -1));
	
	if(!bOpenUndoBracket && ControlUndoBracket == 0)
	{
		FScopeLock ScopeLock(&ControlUndoTransactionMutex);

		/*
		// canceling a sub transaction cancels everything to the top. we need to find a better mechanism for this.
		if(ControlChangedDuringUndoBracket == 0 && ControlUndoTransaction.IsValid())
		{
			ControlUndoTransaction->Cancel();
		}
		*/
		ControlUndoTransaction.Reset();
	}
}

void FTLLRN_ControlRigParameterTrackEditor::HandleOnTLLRN_ControlRigBound(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	if (!InTLLRN_ControlRig)
	{
		return;
	}
	
	UMovieSceneTLLRN_ControlRigParameterTrack* Track = FindTrack(InTLLRN_ControlRig);
	if (!Track)
	{
		return;
	}

	const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = InTLLRN_ControlRig->GetObjectBinding();
	
	for (UMovieSceneSection* BaseSection : Track->GetAllSections())
	{
		if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast< UMovieSceneTLLRN_ControlRigParameterSection>(BaseSection))
		{
			const UTLLRN_ControlRig* TLLRN_ControlRig = Section->GetTLLRN_ControlRig();
			if (TLLRN_ControlRig && InTLLRN_ControlRig == TLLRN_ControlRig)
			{
				if (!Binding->OnTLLRN_ControlRigBind().IsBoundToObject(this))
				{
					Binding->OnTLLRN_ControlRigBind().AddRaw(this, &FTLLRN_ControlRigParameterTrackEditor::HandleOnObjectBoundToTLLRN_ControlRig);
				}
			}
		}
	}
}

void FTLLRN_ControlRigParameterTrackEditor::HandleOnObjectBoundToTLLRN_ControlRig(UObject* InObject)
{
	//reselect these control rigs since selection may get lost
	TMap<TWeakObjectPtr<UTLLRN_ControlRig>, TArray<FName>> ReselectIfNeeded;
	// look for sections to update
	TArray<UMovieSceneTLLRN_ControlRigParameterSection*> SectionsToUpdate;
	for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRigPtr : BoundTLLRN_ControlRigs)
	{
		if (TLLRN_ControlRigPtr.IsValid() == false)
		{
			continue;
		}
		TArray<FName> Selection = TLLRN_ControlRigPtr->CurrentControlSelection();
		if (Selection.Num() > 0)
		{
			ReselectIfNeeded.Add(TLLRN_ControlRigPtr, Selection);
		}
		const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding =
			TLLRN_ControlRigPtr.IsValid() ? TLLRN_ControlRigPtr->GetObjectBinding() : nullptr;
		const UObject* CurrentObject = Binding ? Binding->GetBoundObject() : nullptr;
		if (CurrentObject == InObject)
		{
			if (const UMovieSceneTLLRN_ControlRigParameterTrack* Track = FindTrack(TLLRN_ControlRigPtr.Get()))
			{
				for (UMovieSceneSection* BaseSection : Track->GetAllSections())
				{
					if (UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(BaseSection))
					{
						SectionsToUpdate.AddUnique(Section);
					}
				}
			}
		}
	}

	if (ReselectIfNeeded.Num() > 0)
	{
		GEditor->GetTimerManager()->SetTimerForNextTick([ReselectIfNeeded]()
			{
				GEditor->GetTimerManager()->SetTimerForNextTick([ReselectIfNeeded]()
					{
						for (const TPair <TWeakObjectPtr<UTLLRN_ControlRig>, TArray<FName>>& Pair : ReselectIfNeeded)
						{
							if (Pair.Key.IsValid())
							{
								Pair.Key->ClearControlSelection();
								for (const FName& ControlName : Pair.Value)
								{
									Pair.Key->SelectControl(ControlName, true);
								}
							}
						}
					});

			});
	}
}


void FTLLRN_ControlRigParameterTrackEditor::GetTLLRN_ControlRigKeys(
	UTLLRN_ControlRig* InTLLRN_ControlRig,
	FName ParameterName,
	ETLLRN_ControlRigContextChannelToKey ChannelsToKey,
	ESequencerKeyMode KeyMode,
	UMovieSceneTLLRN_ControlRigParameterSection* SectionToKey,
	FGeneratedTrackKeys& OutGeneratedKeys,
	const bool bInConstraintSpace)
{
	EMovieSceneTransformChannel TransformMask = SectionToKey->GetTransformMask().GetChannels();

	TArray<FTLLRN_RigControlElement*> Controls;
	InTLLRN_ControlRig->GetControlsInOrder(Controls);
	// If key all is enabled, for a key on all the channels
	if (KeyMode != ESequencerKeyMode::ManualKeyForced && GetSequencer()->GetKeyGroupMode() == EKeyGroupMode::KeyAll)
	{
		ChannelsToKey = ETLLRN_ControlRigContextChannelToKey::AllTransform;
	}
	UTLLRN_RigHierarchy* Hierarchy = InTLLRN_ControlRig->GetHierarchy();

	//Need seperate index fo bools,ints and enums and floats since there are seperate entries for each later when they are accessed by the set key stuff.
	int32 SpaceChannelIndex = 0;
	for (int32 LocalControlIndex = 0; LocalControlIndex < Controls.Num(); ++LocalControlIndex)
	{
		FTLLRN_RigControlElement* ControlElement = Controls[LocalControlIndex];
		check(ControlElement);

		if (!Hierarchy->IsAnimatable(ControlElement))
		{
			continue;
		}

		if (FTLLRN_ChannelMapInfo* pChannelIndex = SectionToKey->ControlChannelMap.Find(ControlElement->GetFName()))
		{
			int32 ChannelIndex = pChannelIndex->ChannelIndex;
			const int32 MaskIndex = pChannelIndex->MaskIndex;

			bool bMaskKeyOut = (SectionToKey->GetControlNameMask(ControlElement->GetFName()) == false);
			bool bSetKey = ParameterName.IsNone() || (ControlElement->GetFName() == ParameterName && !bMaskKeyOut);

			FTLLRN_RigControlValue ControlValue = InTLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);

			switch (ControlElement->Settings.ControlType)
			{
			case ETLLRN_RigControlType::Bool:
			{
				bool Val = ControlValue.Get<bool>();
				pChannelIndex->GeneratedKeyIndex = OutGeneratedKeys.Num();
				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneBoolChannel>(ChannelIndex, Val, bSetKey));
				break;
			}
			case ETLLRN_RigControlType::Integer:
			{
				if (ControlElement->Settings.ControlEnum)
				{
					uint8 Val = (uint8)ControlValue.Get<uint8>();
					pChannelIndex->GeneratedKeyIndex = OutGeneratedKeys.Num();
					OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneByteChannel>(ChannelIndex, Val, bSetKey));
				}
				else
				{
					int32 Val = ControlValue.Get<int32>();
					pChannelIndex->GeneratedKeyIndex = OutGeneratedKeys.Num();
					OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneIntegerChannel>(ChannelIndex, Val, bSetKey));
				}
				break;
			}
			case ETLLRN_RigControlType::Float:
			case ETLLRN_RigControlType::ScaleFloat:
			{
				float Val = ControlValue.Get<float>();
				pChannelIndex->GeneratedKeyIndex = OutGeneratedKeys.Num();
				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex, Val, bSetKey));
				break;
			}
			case ETLLRN_RigControlType::Vector2D:
			{
				//use translation x,y for key masks for vector2d
				bool bKeyX = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationX);;
				bool bKeyY = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationY);;
				FVector3f Val = ControlValue.Get<FVector3f>();
				pChannelIndex->GeneratedKeyIndex = OutGeneratedKeys.Num();
				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, Val.X, bKeyX));
				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, Val.Y, bKeyY));
				break;
			}
			case ETLLRN_RigControlType::Position:
			case ETLLRN_RigControlType::Scale:
			case ETLLRN_RigControlType::Rotator:
			{
				bool bKeyX = bSetKey;
				bool bKeyY = bSetKey;
				bool bKeyZ = bSetKey;
				if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Position)
				{
					bKeyX = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationX);
					bKeyY = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationY);
					bKeyZ = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationZ);
				}
				else if(ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
				{
					bKeyX = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationX);
					bKeyY = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationY);
					bKeyZ = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationZ);
				}
				else //scale
				{
					bKeyX = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleX);
					bKeyY = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleY);
					bKeyZ = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleZ);
				}
				
				FVector3f Val = ControlValue.Get<FVector3f>();
				pChannelIndex->GeneratedKeyIndex = OutGeneratedKeys.Num();
				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, Val.X, bKeyX));
				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, Val.Y, bKeyY));
				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, Val.Z, bKeyZ));
				break;
			}

			case ETLLRN_RigControlType::Transform:
			case ETLLRN_RigControlType::TransformNoScale:
			case ETLLRN_RigControlType::EulerTransform:
			{
				FVector Translation, Scale(1.0f, 1.0f, 1.0f);
				FVector Vector = InTLLRN_ControlRig->GetControlSpecifiedEulerAngle(ControlElement);
				FRotator Rotation = FRotator(Vector.Y, Vector.Z, Vector.X);
				if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::TransformNoScale)
				{
					FTransformNoScale NoScale = ControlValue.Get<FTLLRN_RigControlValue::FTransformNoScale_Float>().ToTransform();
					Translation = NoScale.Location;
				}
				else if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
				{
					FEulerTransform Euler = ControlValue.Get<FTLLRN_RigControlValue::FEulerTransform_Float>().ToTransform();
					Translation = Euler.Location;
					Scale = Euler.Scale;
				}
				else
				{
					FTransform Val = ControlValue.Get<FTLLRN_RigControlValue::FTransform_Float>().ToTransform();
					Translation = Val.GetTranslation();
					Scale = Val.GetScale3D();
				}

				if (bInConstraintSpace)
				{
					const uint32 ControlHash = UTLLRN_TransformableControlHandle::ComputeHash(InTLLRN_ControlRig, ControlElement->GetFName());
					TOptional<FTransform> Transform = FTransformConstraintUtils::GetRelativeTransform(InTLLRN_ControlRig->GetWorld(), ControlHash);
					if (Transform)
					{
						Translation = Transform->GetTranslation();
						if (InTLLRN_ControlRig->GetHierarchy()->GetUsePreferredRotationOrder(ControlElement))
						{
							Rotation = ControlElement->PreferredEulerAngles.GetRotatorFromQuat(Transform->GetRotation());
							FVector Angle = Rotation.Euler();
							//need to wind rotators still
							ControlElement->PreferredEulerAngles.SetAngles(Angle, false, ControlElement->PreferredEulerAngles.RotationOrder, true);
							Angle = InTLLRN_ControlRig->GetControlSpecifiedEulerAngle(ControlElement);
							Rotation = FRotator(Vector.Y, Vector.Z, Vector.X);
						}
						else
						{
							Rotation = Transform->GetRotation().Rotator();
						}
						Scale = Transform->GetScale3D();
					}
				}
				
					
				FVector3f CurrentVector = (FVector3f)Translation;
				bool bKeyX = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationX);
				bool bKeyY = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationY);
				bool bKeyZ = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationZ);
				if (KeyMode != ESequencerKeyMode::ManualKeyForced && (GetSequencer()->GetKeyGroupMode() == EKeyGroupMode::KeyGroup && (bKeyX || bKeyY || bKeyZ)))
				{
					bKeyX = bKeyY = bKeyZ = true;
				}
				if (!EnumHasAnyFlags(TransformMask, EMovieSceneTransformChannel::TranslationX))
				{
					bKeyX = false;
				}
				if (!EnumHasAnyFlags(TransformMask, EMovieSceneTransformChannel::TranslationY))
				{
					bKeyY = false;
				}
				if (!EnumHasAnyFlags(TransformMask, EMovieSceneTransformChannel::TranslationZ))
				{
					bKeyZ = false;
				}

				pChannelIndex->GeneratedKeyIndex = OutGeneratedKeys.Num();

				if (pChannelIndex->bDoesHaveSpace)
				{
					//for some saved dev files this could be -1 so we used the local incremented value which is almost always safe, if not a resave will fix the file.
					FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey NewKey;
					int32 RealSpaceChannelIndex = pChannelIndex->SpaceChannelIndex != -1 ? pChannelIndex->SpaceChannelIndex : SpaceChannelIndex;
					++SpaceChannelIndex;
					OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel>(RealSpaceChannelIndex, NewKey, false));
				}


				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, CurrentVector.X, bKeyX));
				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, CurrentVector.Y, bKeyY));
				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, CurrentVector.Z, bKeyZ));

				FRotator3f CurrentRotator = FRotator3f(Rotation);
				bKeyX = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationX);
				bKeyY = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationY);
				bKeyZ = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationZ);
				if (KeyMode != ESequencerKeyMode::ManualKeyForced && (GetSequencer()->GetKeyGroupMode() == EKeyGroupMode::KeyGroup && (bKeyX || bKeyY || bKeyZ)))
				{
					bKeyX = bKeyY = bKeyZ = true;
				}
				if (!EnumHasAnyFlags(TransformMask, EMovieSceneTransformChannel::RotationX))
				{
					bKeyX = false;
				}
				if (!EnumHasAnyFlags(TransformMask, EMovieSceneTransformChannel::RotationY))
				{
					bKeyY = false;
				}
				if (!EnumHasAnyFlags(TransformMask, EMovieSceneTransformChannel::RotationZ))
				{
					bKeyZ = false;
				}

				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, CurrentRotator.Roll, bKeyX));
				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, CurrentRotator.Pitch, bKeyY));
				OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, CurrentRotator.Yaw, bKeyZ));

				if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Transform || ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
				{
					CurrentVector = (FVector3f)Scale;
					bKeyX = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleX);
					bKeyY = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleY);
					bKeyZ = bSetKey && EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleZ);
					if (KeyMode != ESequencerKeyMode::ManualKeyForced && (GetSequencer()->GetKeyGroupMode() == EKeyGroupMode::KeyGroup && (bKeyX || bKeyY || bKeyZ)))
					{
						bKeyX = bKeyY = bKeyZ = true;
					}
					if (!EnumHasAnyFlags(TransformMask, EMovieSceneTransformChannel::ScaleX))
					{
						bKeyX = false;
					}
					if (!EnumHasAnyFlags(TransformMask, EMovieSceneTransformChannel::ScaleY))
					{
						bKeyY = false;
					}
					if (!EnumHasAnyFlags(TransformMask, EMovieSceneTransformChannel::ScaleZ))
					{
						bKeyZ = false;
					}
					OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, CurrentVector.X, bKeyX));
					OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, CurrentVector.Y, bKeyY));
					OutGeneratedKeys.Add(FMovieSceneChannelValueSetter::Create<FMovieSceneFloatChannel>(ChannelIndex++, CurrentVector.Z, bKeyZ));
				}
				break;
			}
		}
		}
	}
}

FKeyPropertyResult FTLLRN_ControlRigParameterTrackEditor::AddKeysToTLLRN_ControlRigHandle(UObject* InObject, UTLLRN_ControlRig* InTLLRN_ControlRig,
	FGuid ObjectHandle, FFrameNumber KeyTime, FFrameNumber EvaluateTime, FGeneratedTrackKeys& GeneratedKeys,
	ESequencerKeyMode KeyMode, TSubclassOf<UMovieSceneTrack> TrackClass, FName TLLRN_ControlTLLRN_RigName, FName TLLRN_RigControlName)
{
	EAutoChangeMode AutoChangeMode = GetSequencer()->GetAutoChangeMode();
	EAllowEditsMode AllowEditsMode = GetSequencer()->GetAllowEditsMode();

	bool bCreateTrack =
		(KeyMode == ESequencerKeyMode::AutoKey && (AutoChangeMode == EAutoChangeMode::AutoTrack || AutoChangeMode == EAutoChangeMode::All)) ||
		KeyMode == ESequencerKeyMode::ManualKey ||
		KeyMode == ESequencerKeyMode::ManualKeyForced ||
		AllowEditsMode == EAllowEditsMode::AllowSequencerEditsOnly;

	bool bCreateSection = false;
	// we don't do this, maybe revisit if a bug occurs, but currently extends sections on autokey.
	//bCreateTrack || (KeyMode == ESequencerKeyMode::AutoKey && (AutoChangeMode != EAutoChangeMode::None));

	// Try to find an existing Track, and if one doesn't exist check the key params and create one if requested.

	FFindOrCreateTrackResult TrackResult = FindOrCreateTLLRN_ControlRigTrackForObject(ObjectHandle, InTLLRN_ControlRig, TLLRN_ControlTLLRN_RigName, bCreateTrack);
	UMovieSceneTLLRN_ControlRigParameterTrack* Track = CastChecked<UMovieSceneTLLRN_ControlRigParameterTrack>(TrackResult.Track, ECastCheckedType::NullAllowed);

	bool bTrackCreated = TrackResult.bWasCreated;

	bool bSectionCreated = false;
	FKeyPropertyResult KeyPropertyResult;

	if (Track)
	{
		float Weight = 1.0f;

		UMovieSceneSection* SectionToKey = Track->GetSectionToKey(TLLRN_RigControlName);
		if (SectionToKey)
		{
			if (SectionToKey->HasEndFrame() && SectionToKey->GetExclusiveEndFrame() < KeyTime)
			{
				SectionToKey->SetEndFrame(KeyTime);
			}
			else if(SectionToKey->HasStartFrame() && SectionToKey->GetInclusiveStartFrame() > KeyTime)
			{
				SectionToKey->SetStartFrame(KeyTime);
			}
		}

		// If there's no overlapping section to key, create one only if a track was newly created. Otherwise, skip keying altogether
		// so that the user is forced to create a section to key on.
		if (bTrackCreated && !SectionToKey)
		{
			Track->Modify();
			SectionToKey = Track->FindOrAddSection(KeyTime, bSectionCreated);
			if (bSectionCreated && GetSequencer()->GetInfiniteKeyAreas())
			{
				SectionToKey->SetRange(TRange<FFrameNumber>::All());
			}
		}

		if (SectionToKey && SectionToKey->GetRange().Contains(KeyTime))
		{
			if (!bTrackCreated)
			{
				//make sure to use weight on section to key
				Weight = MovieSceneHelpers::CalculateWeightForBlending(SectionToKey, KeyTime);
				ModifyOurGeneratedKeysByCurrentAndWeight(InObject, InTLLRN_ControlRig, TLLRN_RigControlName, Track, SectionToKey, EvaluateTime, GeneratedKeys, Weight);
			}
			const UMovieSceneTLLRN_ControlRigParameterSection* ParamSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionToKey);
			if (ParamSection && !ParamSection->GetDoNotKey())
			{
				KeyPropertyResult |= AddKeysToSection(SectionToKey, KeyTime, GeneratedKeys, KeyMode, EKeyFrameTrackEditorSetDefault::SetDefaultOnAddKeys);
			}
		}


		KeyPropertyResult.bTrackCreated |= bTrackCreated || bSectionCreated;
		//if we create a key then compensate
		if (KeyPropertyResult.bKeyCreated)
		{
			UMovieSceneTLLRN_ControlRigParameterSection* ParamSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Track->GetSectionToKey(TLLRN_ControlTLLRN_RigName));
			if (UTLLRN_ControlRig* SectionTLLRN_ControlRig = ParamSection ? ParamSection->GetTLLRN_ControlRig() : nullptr)
			{
				TOptional<FFrameNumber> OptionalKeyTime = KeyTime;

				// compensate spaces
				FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::CompensateIfNeeded(
					SectionTLLRN_ControlRig, GetSequencer().Get(), ParamSection,
					OptionalKeyTime, true /*comp previous*/);

				// compensate constraints
				const uint32 ControlHash = UTLLRN_TransformableControlHandle::ComputeHash(SectionTLLRN_ControlRig, TLLRN_RigControlName);
				FMovieSceneConstraintChannelHelper::CompensateIfNeeded(GetSequencer(), ParamSection, OptionalKeyTime, true /*bCompPreviousTick*/, ControlHash);
			}
		}
	}
	return KeyPropertyResult;
}

FKeyPropertyResult FTLLRN_ControlRigParameterTrackEditor::AddKeysToTLLRN_ControlRig(
	UObject* InObject, UTLLRN_ControlRig* InTLLRN_ControlRig, FFrameNumber KeyTime, FFrameNumber EvaluateTime, FGeneratedTrackKeys& GeneratedKeys,
	ESequencerKeyMode KeyMode, TSubclassOf<UMovieSceneTrack> TrackClass, FName TLLRN_ControlTLLRN_RigName, FName TLLRN_RigControlName)
{
	FKeyPropertyResult KeyPropertyResult;
	EAutoChangeMode AutoChangeMode = GetSequencer()->GetAutoChangeMode();
	EAllowEditsMode AllowEditsMode = GetSequencer()->GetAllowEditsMode();
	bool bCreateHandle =
		(KeyMode == ESequencerKeyMode::AutoKey && (AutoChangeMode == EAutoChangeMode::All)) ||
		KeyMode == ESequencerKeyMode::ManualKey ||
		KeyMode == ESequencerKeyMode::ManualKeyForced ||
		AllowEditsMode == EAllowEditsMode::AllowSequencerEditsOnly;

	FFindOrCreateHandleResult HandleResult = FindOrCreateHandleToObject(InObject, InTLLRN_ControlRig);
	FGuid ObjectHandle = HandleResult.Handle;
	KeyPropertyResult.bHandleCreated = HandleResult.bWasCreated;
	KeyPropertyResult |= AddKeysToTLLRN_ControlRigHandle(InObject, InTLLRN_ControlRig, ObjectHandle, KeyTime, EvaluateTime, GeneratedKeys, KeyMode, TrackClass, TLLRN_ControlTLLRN_RigName, TLLRN_RigControlName);

	return KeyPropertyResult;
}

void FTLLRN_ControlRigParameterTrackEditor::AddControlKeys(
	UObject* InObject,
	UTLLRN_ControlRig* InTLLRN_ControlRig,
	FName TLLRN_ControlTLLRN_RigName,
	FName TLLRN_RigControlName,
	ETLLRN_ControlRigContextChannelToKey ChannelsToKey,
	ESequencerKeyMode KeyMode,
	float InLocalTime,
	const bool bInConstraintSpace)
{
	if (KeyMode == ESequencerKeyMode::ManualKey || (GetSequencer().IsValid() && !GetSequencer()->IsAllowedToChange()))
	{
		return;
	}
	bool bCreateTrack = false;
	bool bCreateHandle = false;
	FFindOrCreateHandleResult HandleResult = FindOrCreateHandleToObject(InObject, InTLLRN_ControlRig);
	FGuid ObjectHandle = HandleResult.Handle;
	FFindOrCreateTrackResult TrackResult = FindOrCreateTLLRN_ControlRigTrackForObject(ObjectHandle, InTLLRN_ControlRig, TLLRN_ControlTLLRN_RigName, bCreateTrack);
	UMovieSceneTLLRN_ControlRigParameterTrack* Track = CastChecked<UMovieSceneTLLRN_ControlRigParameterTrack>(TrackResult.Track, ECastCheckedType::NullAllowed);
	UMovieSceneTLLRN_ControlRigParameterSection* ParamSection = nullptr;
	if (Track)
	{
		//track editors use a hidden time so we need to set it if we are using non sequencer times when keying.
		if (InLocalTime != FLT_MAX)
		{
			//convert from frame time since conversion may give us one frame less, e.g 1.53333330 * 24000.0/1.0 = 36799.999199999998
			FFrameTime LocalFrameTime = GetSequencer()->GetFocusedTickResolution().AsFrameTime((double)InLocalTime);
			BeginKeying(LocalFrameTime.RoundToFrame());
		}
		const FFrameNumber FrameTime = GetTimeForKey();
		UMovieSceneSection* Section = Track->GetSectionToKey(TLLRN_RigControlName);
		ParamSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Section);

		if (ParamSection && ParamSection->GetDoNotKey())
		{
			return;
		}
	}

	if (!ParamSection)
	{
		return;
	}

	TSharedRef<FGeneratedTrackKeys> GeneratedKeys = MakeShared<FGeneratedTrackKeys>();
	GetTLLRN_ControlRigKeys(InTLLRN_ControlRig, TLLRN_RigControlName, ChannelsToKey, KeyMode, ParamSection, *GeneratedKeys, bInConstraintSpace);
	
	TGuardValue<bool> Guard(bIsDoingSelection, true);

	auto OnKeyProperty = [=, this](FFrameNumber Time) -> FKeyPropertyResult
	{
		FFrameNumber LocalTime = Time;
		//for modify weights we evaluate so need to make sure we use the evaluated time
		FFrameNumber EvaluateTime = GetSequencer()->GetLastEvaluatedLocalTime().RoundToFrame();
		//if InLocalTime is specified that means time value was set with SetControlValue, so we don't use sequencer times at all, but this time instead
		if (InLocalTime != FLT_MAX)
		{
			//convert from frame time since conversion may give us one frame less, e.g 1.53333330 * 24000.0/1.0 = 36799.999199999998
			FFrameTime LocalFrameTime = GetSequencer()->GetFocusedTickResolution().AsFrameTime((double)InLocalTime);
			LocalTime = LocalFrameTime.RoundToFrame();
			EvaluateTime = LocalTime;
		}
		
		return this->AddKeysToTLLRN_ControlRig(InObject, InTLLRN_ControlRig, LocalTime, EvaluateTime, *GeneratedKeys, KeyMode, UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), TLLRN_ControlTLLRN_RigName, TLLRN_RigControlName);
	};

	AnimatablePropertyChanged(FOnKeyProperty::CreateLambda(OnKeyProperty));
	EndKeying(); //fine even if we didn't BeginKeying
}

bool FTLLRN_ControlRigParameterTrackEditor::ModifyOurGeneratedKeysByCurrentAndWeight(UObject* Object, UTLLRN_ControlRig* InTLLRN_ControlRig, FName TLLRN_RigControlName, UMovieSceneTrack* Track, UMovieSceneSection* SectionToKey, FFrameNumber EvaluateTime, FGeneratedTrackKeys& GeneratedTotalKeys, float Weight) const
{
	FFrameRate TickResolution = GetSequencer()->GetFocusedTickResolution();
	FMovieSceneEvaluationTrack EvalTrack = CastChecked<UMovieSceneTLLRN_ControlRigParameterTrack>(Track)->GenerateTrackTemplate(Track);

	FMovieSceneInterrogationData InterrogationData;
	GetSequencer()->GetEvaluationTemplate().CopyActuators(InterrogationData.GetAccumulator());

	//use the EvaluateTime to do the evaluation, may be different than the actually time we key
	FMovieSceneContext Context(FMovieSceneEvaluationRange(EvaluateTime, GetSequencer()->GetFocusedTickResolution()));
	EvalTrack.Interrogate(Context, InterrogationData, Object);
	TArray<FTLLRN_RigControlElement*> Controls = InTLLRN_ControlRig->AvailableControls();
	UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(SectionToKey);
	FMovieSceneChannelProxy& Proxy = SectionToKey->GetChannelProxy();
	int32 ChannelIndex = 0;
	FTLLRN_ChannelMapInfo* pChannelIndex = nullptr;
	for (FTLLRN_RigControlElement* ControlElement : Controls)
	{
		if (!InTLLRN_ControlRig->GetHierarchy()->IsAnimatable(ControlElement))
		{
			continue;
		}
		switch (ControlElement->Settings.ControlType)
		{
		case ETLLRN_RigControlType::Float:
		case ETLLRN_RigControlType::ScaleFloat:
		{
			for (const FFloatInterrogationData& Val : InterrogationData.Iterate<FFloatInterrogationData>(UMovieSceneTLLRN_ControlRigParameterSection::GetFloatInterrogationKey()))
			{
				if ((Val.ParameterName == ControlElement->GetFName()))
				{
					pChannelIndex = Section->ControlChannelMap.Find(ControlElement->GetFName());
					if (pChannelIndex && pChannelIndex->GeneratedKeyIndex != INDEX_NONE)
					{
						ChannelIndex = pChannelIndex->GeneratedKeyIndex;
						float FVal = (float)Val.Val;
						GeneratedTotalKeys[ChannelIndex]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void *)&FVal, Weight);
					}
					break;
				}
			}
			break;
		}
		//no blending of bools,ints/enums
		case ETLLRN_RigControlType::Bool:
		case ETLLRN_RigControlType::Integer:
		{

			break;
		}
		case ETLLRN_RigControlType::Vector2D:
		{
			for (const FVector2DInterrogationData& Val : InterrogationData.Iterate<FVector2DInterrogationData>(UMovieSceneTLLRN_ControlRigParameterSection::GetVector2DInterrogationKey()))
			{
				if ((Val.ParameterName == ControlElement->GetFName()))
				{
					pChannelIndex = Section->ControlChannelMap.Find(ControlElement->GetFName());
					if (pChannelIndex && pChannelIndex->GeneratedKeyIndex != INDEX_NONE)
					{
						ChannelIndex = pChannelIndex->GeneratedKeyIndex;
						float FVal = (float)Val.Val.X;
						GeneratedTotalKeys[ChannelIndex]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void *)&FVal, Weight);
						FVal = (float)Val.Val.Y;
						GeneratedTotalKeys[ChannelIndex + 1]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void *)&FVal, Weight);
					}
					break;
				}
			}
			break;
		}
		case ETLLRN_RigControlType::Position:
		case ETLLRN_RigControlType::Scale:
		case ETLLRN_RigControlType::Rotator:
		{
			for (const FVectorInterrogationData& Val : InterrogationData.Iterate<FVectorInterrogationData>(UMovieSceneTLLRN_ControlRigParameterSection::GetVectorInterrogationKey()))
			{
				if ((Val.ParameterName == ControlElement->GetFName()))
				{
					pChannelIndex = Section->ControlChannelMap.Find(ControlElement->GetFName());
					if (pChannelIndex && pChannelIndex->GeneratedKeyIndex != INDEX_NONE)
					{
						ChannelIndex = pChannelIndex->GeneratedKeyIndex;
						float FVal = (float)Val.Val.X;
						GeneratedTotalKeys[ChannelIndex]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void*)&FVal, Weight);
						FVal = (float)Val.Val.Y;
						GeneratedTotalKeys[ChannelIndex + 1]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void*)&FVal, Weight);
						FVal = (float)Val.Val.Z;
						GeneratedTotalKeys[ChannelIndex + 2]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void*)&FVal, Weight);			
					}
					break;
				}
			}
			break;
		}

		case ETLLRN_RigControlType::Transform:
		case ETLLRN_RigControlType::TransformNoScale:
		case ETLLRN_RigControlType::EulerTransform:
		{
			for (const FEulerTransformInterrogationData& Val : InterrogationData.Iterate<FEulerTransformInterrogationData>(UMovieSceneTLLRN_ControlRigParameterSection::GetTransformInterrogationKey()))
			{

				if ((Val.ParameterName == ControlElement->GetFName()))
				{
					pChannelIndex = Section->ControlChannelMap.Find(ControlElement->GetFName());
					if (pChannelIndex && pChannelIndex->GeneratedKeyIndex != INDEX_NONE)
					{
						ChannelIndex = pChannelIndex->GeneratedKeyIndex;

						if (pChannelIndex->bDoesHaveSpace)
						{
							++ChannelIndex;
						}
						
						FVector3f CurrentPos = (FVector3f)Val.Val.GetLocation();
						FRotator3f CurrentRot = FRotator3f(Val.Val.Rotator());
						GeneratedTotalKeys[ChannelIndex]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void*)&CurrentPos.X, Weight);
						GeneratedTotalKeys[ChannelIndex + 1]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void*)&CurrentPos.Y, Weight);
						GeneratedTotalKeys[ChannelIndex + 2]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void*)&CurrentPos.Z, Weight);

						GeneratedTotalKeys[ChannelIndex + 3]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void*)&CurrentRot.Roll, Weight);
						GeneratedTotalKeys[ChannelIndex + 4]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void*)&CurrentRot.Pitch, Weight);
						GeneratedTotalKeys[ChannelIndex + 5]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void*)&CurrentRot.Yaw, Weight);

						if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Transform || ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
						{
							FVector3f CurrentScale = (FVector3f)Val.Val.GetScale3D();
							GeneratedTotalKeys[ChannelIndex + 6]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void*)&CurrentScale.X, Weight);
							GeneratedTotalKeys[ChannelIndex + 7]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void*)&CurrentScale.Y, Weight);
							GeneratedTotalKeys[ChannelIndex + 8]->ModifyByCurrentAndWeight(Proxy, EvaluateTime, (void*)&CurrentScale.Z, Weight);
						}
					}
					break;
				}
			}
			break;
		}
		}
	}
	return true;
}

void FTLLRN_ControlRigParameterTrackEditor::BuildTrackContextMenu(FMenuBuilder& MenuBuilder, UMovieSceneTrack* InTrack)
{
	bool bSectionAdded;
	UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast< UMovieSceneTLLRN_ControlRigParameterTrack>(InTrack);
	if (!Track || Track->GetTLLRN_ControlRig() == nullptr)
	{
		return;
	}

	UMovieSceneTLLRN_ControlRigParameterSection* SectionToKey = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Track->GetSectionToKey());
	if (SectionToKey == nullptr)
	{
		SectionToKey = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Track->FindOrAddSection(0, bSectionAdded));
	}
	if (!SectionToKey)
	{
		return;
	}

	
	// Check if the selected element is a section of the track
	bool bIsSection = Track->GetAllSections().Num() > 1;
	if (bIsSection)
	{
		TArray<TWeakObjectPtr<UObject>> TrackSections;
		for (UE::Sequencer::TViewModelPtr<UE::Sequencer::ITrackExtension> TrackExtension : GetSequencer()->GetViewModel()->GetSelection()->Outliner.Filter<UE::Sequencer::ITrackExtension>())
		{
			for (UMovieSceneSection* Section : TrackExtension->GetSections())
			{
				TrackSections.Add(Section);
			}
		}
		bIsSection = TrackSections.Num() > 0;
	}
	
	TArray<FRigControlFBXNodeAndChannels>* NodeAndChannels = Track->GetNodeAndChannelMappings(SectionToKey);

	MenuBuilder.BeginSection("TLLRN Control Rig IO", LOCTEXT("TLLRN_ControlRigIO", "TLLRN Control Rig I/O"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ImportTLLRN_ControlRigFBX", "Import Control Rig FBX"),
			LOCTEXT("ImportTLLRN_ControlRigFBXTooltip", "Import Control Rig animation from FBX"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigParameterTrackEditor::ImportFBX, Track, SectionToKey, NodeAndChannels)));

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ExportTLLRN_ControlRigFBX", "Export Control Rig FBX"),
			LOCTEXT("ExportTLLRN_ControlRigFBXTooltip", "Export Control Rig animation to FBX"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigParameterTrackEditor::ExportFBX, Track, SectionToKey)));
	}
	MenuBuilder.EndSection();

	if (!bIsSection)
	{
		MenuBuilder.BeginSection("TLLRN Control Rig", LOCTEXT("TLLRN_ControlRig", "TLLRN Control Rig"));
		{
			MenuBuilder.AddWidget(
				SNew(SSpinBox<int32>)
				.MinValue(0)
				.Font(FAppStyle::GetFontStyle(TEXT("MenuItem.Font")))
				.ToolTipText(LOCTEXT("OrderTooltip", "Order for this Control Rig to evaluate compared to others on the same binding, higher number means earlier evaluation"))
				.Value_Lambda([Track]() { return Track->GetPriorityOrder(); })
				.OnValueChanged_Lambda([Track](int32 InValue) { Track->SetPriorityOrder(InValue); })
				,
				LOCTEXT("Order", "Order")
			);

			if (CVarEnableAdditiveTLLRN_ControlRigs->GetBool())
			{
				MenuBuilder.AddMenuEntry(
					   LOCTEXT("ConvertIsLayeredTLLRN_ControlRig", "Convert To Layered"),
					   LOCTEXT("ConvertIsLayeredTLLRN_ControlRigToolTip", "Converts the Control Rig from an Absolute rig to a Layered rig"),
					   FSlateIcon(),
					   FUIAction(
						   FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigParameterTrackEditor::ConvertIsLayered, Track),
						   FCanExecuteAction(),
						   FIsActionChecked::CreateRaw(this, &FTLLRN_ControlRigParameterTrackEditor::IsLayered, Track)
					   ),
					   NAME_None,
					   EUserInterfaceActionType::ToggleButton);
			}
		}
		MenuBuilder.EndSection();
	}

	MenuBuilder.AddMenuSeparator();

	if (UFKTLLRN_ControlRig* AutoRig = Cast<UFKTLLRN_ControlRig>(Track->GetTLLRN_ControlRig()))
	{
		MenuBuilder.BeginSection("FK Control Rig", LOCTEXT("FKTLLRN_ControlRig", "FK Control Rig"));
		{

			MenuBuilder.AddMenuEntry(
				LOCTEXT("SelectBonesToAnimate", "Select Bones Or Curves To Animate"),
				LOCTEXT("SelectBonesToAnimateToolTip", "Select which bones or curves you want to directly animate"),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigParameterTrackEditor::SelectFKBonesToAnimate, AutoRig, Track)));
		}
		MenuBuilder.EndSection();

		MenuBuilder.AddMenuSeparator();
	}
	else if (UTLLRN_ControlRig* LayeredRig = Cast<UTLLRN_ControlRig>(Track->GetTLLRN_ControlRig()))
	{
		if (LayeredRig->IsAdditive())
		{
			MenuBuilder.BeginSection("Layered Control Rig", LOCTEXT("LayeredTLLRN_ControlRig", "Layered Control Rig"));
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("Bake Inverted Pose", "Bake Inverted Pose"),
					LOCTEXT("BakeInvertedPoseToolTip", "Bake inversion of the input pose into the rig"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigParameterTrackEditor::BakeInvertedPose, LayeredRig, Track)));
			}
			MenuBuilder.EndSection();
			MenuBuilder.AddMenuSeparator();
		}
	}

}

bool FTLLRN_ControlRigParameterTrackEditor::HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid)
{
	if (!Asset->IsA<UTLLRN_ControlRigBlueprint>())
	{
		return false;
	}

	UMovieScene* MovieScene = GetSequencer()->GetFocusedMovieSceneSequence()->GetMovieScene();
	if (!MovieScene)
	{
		return false;
	}

	UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(Asset);
	URigVMBlueprintGeneratedClass* RigClass = TLLRN_ControlRigBlueprint->GetRigVMBlueprintGeneratedClass();
	if (!RigClass)
	{
		return false;
	}

	USkeletalMesh* SkeletalMesh = TLLRN_ControlRigBlueprint->GetPreviewMesh();
	if (!SkeletalMesh)
	{
		FNotificationInfo Info(LOCTEXT("NoPreviewMesh", "Control rig has no preview mesh to create a spawnable skeletal mesh actor from"));
		Info.ExpireDuration = 5.0f;
		FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddTLLRN_ControlRigAsset", "Add Control Rig"));

	UE::Sequencer::FCreateBindingParams CreateBindingParams;
	CreateBindingParams.bSpawnable = true;
	CreateBindingParams.bAllowCustomBinding = true;
	FGuid NewGuid = GetSequencer()->CreateBinding(*ASkeletalMeshActor::StaticClass(), CreateBindingParams);

	// CreateBinding can fail if spawnables are not allowed
	if (!NewGuid.IsValid())
	{
		return false;
	}
	
	ASkeletalMeshActor* SpawnedSkeletalMeshActor = Cast<ASkeletalMeshActor>(GetSequencer()->FindSpawnedObjectOrTemplate(NewGuid));
	if (!ensure(SpawnedSkeletalMeshActor))
	{
		return false;
	}

	SpawnedSkeletalMeshActor->GetSkeletalMeshComponent()->SetSkeletalMesh(SkeletalMesh);

	FString NewName = MovieSceneHelpers::MakeUniqueSpawnableName(MovieScene, FName::NameToDisplayString(SkeletalMesh->GetName(), false));
	SpawnedSkeletalMeshActor->SetActorLabel(NewName, false);
	
	// Save Spawnable state as the default (with new name and skeletal mesh asset)
	{
		GetSequencer()->GetSpawnRegister().SaveDefaultSpawnableState(NewGuid, GetSequencer()->GetFocusedTemplateID(), GetSequencer()->GetSharedPlaybackState());
	}

	UMovieSceneTLLRN_ControlRigParameterTrack* Track = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(MovieScene->FindTrack(UMovieSceneTLLRN_ControlRigParameterTrack::StaticClass(), NewGuid, NAME_None));
	if (Track == nullptr)
	{
		UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(RigClass->GetDefaultObject(true /* create if needed */));
		check(CDO);

		AddTLLRN_ControlRig(CDO->GetClass(), SpawnedSkeletalMeshActor->GetSkeletalMeshComponent(), NewGuid);
	}

	return true;
}

void FTLLRN_ControlRigParameterTrackEditor::ImportFBX(UMovieSceneTLLRN_ControlRigParameterTrack* InTrack, UMovieSceneTLLRN_ControlRigParameterSection* InSection,
	TArray<FRigControlFBXNodeAndChannels>* NodeAndChannels)
{
	if (NodeAndChannels)
	{
		// NodeAndChannels will be deleted later
		MovieSceneToolHelpers::ImportFBXIntoControlRigChannelsWithDialog(GetSequencer().ToSharedRef(), NodeAndChannels);
	}
}

void FTLLRN_ControlRigParameterTrackEditor::ExportFBX(UMovieSceneTLLRN_ControlRigParameterTrack* InTrack, UMovieSceneTLLRN_ControlRigParameterSection* InSection)
{
	if (InTrack && InTrack->GetTLLRN_ControlRig())
	{
		// ControlComponentTransformsMapping will be deleted later
		MovieSceneToolHelpers::ExportFBXFromControlRigChannelsWithDialog(GetSequencer().ToSharedRef(), InTrack);
	}
}



class SFKTLLRN_ControlTLLRN_RigBoneSelect : public SCompoundWidget, public FGCObject
{
public:

	SLATE_BEGIN_ARGS(SFKTLLRN_ControlTLLRN_RigBoneSelect) {}
	SLATE_ATTRIBUTE(UFKTLLRN_ControlRig*, AutoRig)
		SLATE_ATTRIBUTE(UMovieSceneTLLRN_ControlRigParameterTrack*, Track)
		SLATE_ATTRIBUTE(ISequencer*, Sequencer)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
	{
		AutoRig = InArgs._AutoRig.Get();
		Track = InArgs._Track.Get();
		Sequencer = InArgs._Sequencer.Get();

		this->ChildSlot[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f, 4.0f, 8.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SFKTLLRN_ControlTLLRN_RigBoneSelectDescription", "Select Bones You Want To Be Active On The FK Control Rig"))
				]
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f, 4.0f, 8.0f, 4.0f)
				[
					SNew(SSeparator)
				]
			+ SVerticalBox::Slot()
				.Padding(8.0f, 4.0f, 8.0f, 4.0f)
				[
					SNew(SBorder)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
				[
					//Save this widget so we can populate it later with check boxes
					SAssignNew(CheckBoxContainer, SVerticalBox)
				]
					]
				]
			+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				.Padding(8.0f, 4.0f, 8.0f, 4.0f)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
				.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
				.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
				.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
				.OnClicked(this, &SFKTLLRN_ControlTLLRN_RigBoneSelect::ChangeAllOptions, true)
				.Text(LOCTEXT("FKRigSelectAll", "Select All"))
				]
			+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
				.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
				.OnClicked(this, &SFKTLLRN_ControlTLLRN_RigBoneSelect::ChangeAllOptions, false)
				.Text(LOCTEXT("FKRigDeselectAll", "Deselect All"))
				]

				]
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f, 4.0f, 8.0f, 4.0f)
				[
					SNew(SSeparator)
				]
			+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(8.0f, 4.0f, 8.0f, 4.0f)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
				.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
				.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
				.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
				.OnClicked(this, &SFKTLLRN_ControlTLLRN_RigBoneSelect::OnButtonClick, true)
				.Text(LOCTEXT("FKRigeOk", "OK"))
				]
			+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
				.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
				.OnClicked(this, &SFKTLLRN_ControlTLLRN_RigBoneSelect::OnButtonClick, false)
				.Text(LOCTEXT("FKRigCancel", "Cancel"))
				]
				]
		];
	}


	/**
	* Creates a Slate check box
	*
	* @param	Label		Text label for the check box
	* @param	ButtonId	The ID for the check box
	* @return				The created check box widget
	*/
	TSharedRef<SWidget> CreateCheckBox(const FString& Label, int32 ButtonId)
	{
		return
			SNew(SCheckBox)
			.IsChecked(this, &SFKTLLRN_ControlTLLRN_RigBoneSelect::IsCheckboxChecked, ButtonId)
			.OnCheckStateChanged(this, &SFKTLLRN_ControlTLLRN_RigBoneSelect::OnCheckboxChanged, ButtonId)
			[
				SNew(STextBlock).Text(FText::FromString(Label))
			];
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(AutoRig);
	}
	virtual FString GetReferencerName() const override
	{
		return TEXT("SFKTLLRN_ControlTLLRN_RigBoneSelect");
	}

	/**
	* Returns the state of the check box
	*
	* @param	ButtonId	The ID for the check box
	* @return				The status of the check box
	*/
	ECheckBoxState IsCheckboxChecked(int32 ButtonId) const
	{
		return CheckBoxInfoMap.FindChecked(ButtonId).bActive ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	/**
	* Handler for all check box clicks
	*
	* @param	NewCheckboxState	The new state of the check box
	* @param	CheckboxThatChanged	The ID of the radio button that has changed.
	*/
	void OnCheckboxChanged(ECheckBoxState NewCheckboxState, int32 CheckboxThatChanged)
	{
		FFKBoneCheckInfo& Info = CheckBoxInfoMap.FindChecked(CheckboxThatChanged);
		Info.bActive = !Info.bActive;
	}

	/**
	* Handler for the Select All and Deselect All buttons
	*
	* @param	bNewCheckedState	The new state of the check boxes
	*/
	FReply ChangeAllOptions(bool bNewCheckedState)
	{
		for (TPair<int32, FFKBoneCheckInfo>& Pair : CheckBoxInfoMap)
		{
			FFKBoneCheckInfo& Info = Pair.Value;
			Info.bActive = bNewCheckedState;
		}
		return FReply::Handled();
	}

	/**
	* Populated the dialog with multiple check boxes, each corresponding to a bone
	*
	* @param	BoneInfos	The list of Bones to populate the dialog with
	*/
	void PopulateOptions(TArray<FFKBoneCheckInfo>& BoneInfos)
	{
		for (FFKBoneCheckInfo& Info : BoneInfos)
		{
			CheckBoxInfoMap.Add(Info.BoneID, Info);

			CheckBoxContainer->AddSlot()
				.AutoHeight()
				[
					CreateCheckBox(Info.BoneName.GetPlainNameString(), Info.BoneID)
				];
		}
	}


private:

	/**
	* Handles when a button is pressed, should be bound with appropriate EResult Key
	*
	* @param ButtonID - The return type of the button which has been pressed.
	*/
	FReply OnButtonClick(bool bValid)
	{
		TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(AsShared());

		if (Window.IsValid())
		{
			Window->RequestDestroyWindow();
		}
		//if ok selected bValid == true
		if (bValid && AutoRig)
		{
			TArray<FFKBoneCheckInfo> BoneCheckArray;
			BoneCheckArray.SetNumUninitialized(CheckBoxInfoMap.Num());
			int32 Index = 0;
			for (TPair<int32, FFKBoneCheckInfo>& Pair : CheckBoxInfoMap)
			{
				FFKBoneCheckInfo& Info = Pair.Value;
				BoneCheckArray[Index++] = Info;

			}
			if (Track && Sequencer)
			{
				TArray<UMovieSceneSection*> Sections = Track->GetAllSections();
				for (UMovieSceneSection* IterSection : Sections)
				{
					UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast< UMovieSceneTLLRN_ControlRigParameterSection>(IterSection);
					if (Section)
					{
						for (const FFKBoneCheckInfo& Info : BoneCheckArray)
						{
							Section->SetControlNameMask(Info.BoneName, Info.bActive);
						}
					}
				}
				Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
			}
			AutoRig->SetControlActive(BoneCheckArray);
		}
		return bValid ? FReply::Handled() : FReply::Unhandled();
	}

	/** The slate container that the bone check boxes get added to */
	TSharedPtr<SVerticalBox>	 CheckBoxContainer;
	/** Store the check box state for each bone */
	TMap<int32, FFKBoneCheckInfo> CheckBoxInfoMap;

	TObjectPtr<UFKTLLRN_ControlRig> AutoRig;
	UMovieSceneTLLRN_ControlRigParameterTrack* Track;
	ISequencer* Sequencer;
};

void FTLLRN_ControlRigParameterTrackEditor::SelectFKBonesToAnimate(UFKTLLRN_ControlRig* AutoRig, UMovieSceneTLLRN_ControlRigParameterTrack* Track)
{
	if (AutoRig)
	{
		const FText TitleText = LOCTEXT("SelectBonesOrCurvesToAnimate", "Select Bones Or Curves To Animate");

		// Create the window to choose our options
		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(TitleText)
			.HasCloseButton(true)
			.SizingRule(ESizingRule::UserSized)
			.ClientSize(FVector2D(400.0f, 200.0f))
			.AutoCenter(EAutoCenter::PreferredWorkArea)
			.SupportsMinimize(false);

		TSharedRef<SFKTLLRN_ControlTLLRN_RigBoneSelect> DialogWidget = SNew(SFKTLLRN_ControlTLLRN_RigBoneSelect)
			.AutoRig(AutoRig)
			.Track(Track)
			.Sequencer(GetSequencer().Get());

		TArray<FName> TLLRN_ControlTLLRN_RigNames = AutoRig->GetControlNames();
		TArray<FFKBoneCheckInfo> BoneInfos;
		for (int32 Index = 0; Index < TLLRN_ControlTLLRN_RigNames.Num(); ++Index)
		{
			FFKBoneCheckInfo Info;
			Info.BoneID = Index;
			Info.BoneName = TLLRN_ControlTLLRN_RigNames[Index];
			Info.bActive = AutoRig->GetControlActive(Index);
			BoneInfos.Add(Info);
		}

		DialogWidget->PopulateOptions(BoneInfos);

		Window->SetContent(DialogWidget);
		FSlateApplication::Get().AddWindow(Window);
	}

	//reconstruct all channel proxies TODO or not to do that is the question
}


TOptional<FBakingAnimationKeySettings> SCollapseControlsWidget::CollapseControlsSettings;

void SCollapseControlsWidget::Construct(const FArguments& InArgs)
{
	Sequencer = InArgs._Sequencer;

	if (CollapseControlsSettings.IsSet() == false)
	{
		TSharedPtr<ISequencer> SequencerPtr = Sequencer.Pin();
		CollapseControlsSettings = FBakingAnimationKeySettings();
		const FFrameRate TickResolution = SequencerPtr->GetFocusedTickResolution();
		const FFrameTime FrameTime = SequencerPtr->GetLocalTime().ConvertTo(TickResolution);
		FFrameNumber CurrentTime = FrameTime.GetFrame();

		TRange<FFrameNumber> Range = SequencerPtr->GetFocusedMovieSceneSequence()->GetMovieScene()->GetPlaybackRange();
		TArray<FFrameNumber> Keys;
		TArray < FKeyHandle> KeyHandles;

		CollapseControlsSettings.GetValue().StartFrame = Range.GetLowerBoundValue();
		CollapseControlsSettings.GetValue().EndFrame = Range.GetUpperBoundValue();
	}


	Settings = MakeShared<TStructOnScope<FBakingAnimationKeySettings>>();
	Settings->InitializeAs<FBakingAnimationKeySettings>(CollapseControlsSettings.GetValue());

	FStructureDetailsViewArgs StructureViewArgs;
	StructureViewArgs.bShowObjects = true;
	StructureViewArgs.bShowAssets = true;
	StructureViewArgs.bShowClasses = true;
	StructureViewArgs.bShowInterfaces = true;

	FDetailsViewArgs ViewArgs;
	ViewArgs.bAllowSearch = false;
	ViewArgs.bHideSelectionTip = false;
	ViewArgs.bShowObjectLabel = false;

	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	DetailsView = PropertyEditor.CreateStructureDetailView(ViewArgs, StructureViewArgs, TSharedPtr<FStructOnScope>());
	TSharedPtr<INumericTypeInterface<double>> NumericTypeInterface = Sequencer.Pin()->GetNumericTypeInterface();
	DetailsView->GetDetailsView()->RegisterInstancedCustomPropertyTypeLayout("FrameNumber",
		FOnGetPropertyTypeCustomizationInstance::CreateLambda([=]() {return MakeShared<FFrameNumberDetailsCustomization>(NumericTypeInterface); }));
	DetailsView->SetStructureData(Settings);

	ChildSlot
		[
			SNew(SBorder)
			.Visibility(EVisibility::Visible)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f)
				[
					DetailsView->GetWidget().ToSharedRef()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(16.f)
				[

					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Fill)
					[
						SNew(SSpacer)
					]

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Right)
					.Padding(0.f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
						.Text(LOCTEXT("OK", "OK"))
						.OnClicked_Lambda([this, InArgs]()
							{
								Collapse();
								CloseDialog();
								return FReply::Handled();

							})
						.IsEnabled_Lambda([this]()
							{
								return (Settings.IsValid());
							})
					]
				]
			]
		];
}

void  SCollapseControlsWidget::Collapse()
{
	FBakingAnimationKeySettings* BakeSettings = Settings->Get();
	TSharedPtr<ISequencer> SequencerPtr = Sequencer.Pin();
	CollapseCB.ExecuteIfBound(SequencerPtr, *BakeSettings);
	CollapseControlsSettings = *BakeSettings;
}

class SCollapseControlsWidgetWindow : public SWindow
{
};

FReply SCollapseControlsWidget::OpenDialog(bool bModal)
{
	check(!DialogWindow.IsValid());

	const FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();

	TSharedRef<SCollapseControlsWidgetWindow> Window = SNew(SCollapseControlsWidgetWindow)
		.Title(LOCTEXT("CollapseControls", "Collapse Controls"))
		.CreateTitleBar(true)
		.Type(EWindowType::Normal)
		.SizingRule(ESizingRule::Autosized)
		.ScreenPosition(CursorPos)
		.FocusWhenFirstShown(true)
		.ActivationPolicy(EWindowActivationPolicy::FirstShown)
		[
			AsShared()
		];

	Window->SetWidgetToFocusOnActivate(AsShared());

	DialogWindow = Window;

	Window->MoveWindowTo(CursorPos);

	if (bModal)
	{
		GEditor->EditorAddModalWindow(Window);
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window);
	}

	return FReply::Handled();
}

void SCollapseControlsWidget::CloseDialog()
{
	if (DialogWindow.IsValid())
	{
		DialogWindow.Pin()->RequestDestroyWindow();
		DialogWindow.Reset();
	}
}
///////////////
//end of SCollapseControlsWidget
/////////////////////
struct FKeyAndValuesAtFrame
{
	FFrameNumber Frame;
	TArray<FMovieSceneFloatValue> KeyValues;
	float FinalValue;
};

bool CollapseAllLayersPerKey(TSharedPtr<ISequencer>& SequencerPtr, UMovieSceneTrack* OwnerTrack, const FBakingAnimationKeySettings& InSettings)
{
	if (SequencerPtr.IsValid() && OwnerTrack)
	{
		TArray<UMovieSceneSection*> Sections = OwnerTrack->GetAllSections();
		return MovieSceneToolHelpers::CollapseSection(SequencerPtr, OwnerTrack, Sections, InSettings);
	}
	return false;
}

bool FTLLRN_ControlRigParameterTrackEditor::CollapseAllLayers(TSharedPtr<ISequencer>&SequencerPtr, UMovieSceneTrack * OwnerTrack, const FBakingAnimationKeySettings &InSettings)
{
	if (InSettings.BakingKeySettings == EBakingKeySettings::KeysOnly)
	{
		return CollapseAllLayersPerKey(SequencerPtr, OwnerTrack, InSettings);
	}
	else
	{
		if (SequencerPtr.IsValid() && OwnerTrack)
		{
			TArray<UMovieSceneSection*> Sections = OwnerTrack->GetAllSections();
			//make sure right type
			if (Sections.Num() < 1)
			{
				UE_LOG(LogTLLRN_ControlRigEditor, Log, TEXT("CollapseAllSections::No sections on track"));
				return false;
			}
			if (UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(Sections[0]))
			{
				if (ParameterSection->GetBlendType().Get() == EMovieSceneBlendType::Absolute)
				{
					FScopedTransaction Transaction(LOCTEXT("CollapseAllSections", "Collapse All Sections"));
					ParameterSection->Modify();
					UTLLRN_ControlRig* TLLRN_ControlRig = ParameterSection->GetTLLRN_ControlRig();
					FMovieSceneSequenceTransform RootToLocalTransform = SequencerPtr->GetFocusedMovieSceneSequenceTransform();

					FFrameNumber StartFrame = InSettings.StartFrame;
					FFrameNumber EndFrame = InSettings.EndFrame;
					TRange<FFrameNumber> Range(StartFrame, EndFrame);
					const FFrameRate& FrameRate = SequencerPtr->GetFocusedDisplayRate();
					const FFrameRate& TickResolution = SequencerPtr->GetFocusedTickResolution();

					//frames and (optional) tangents
					TArray<TPair<FFrameNumber, TArray<FMovieSceneTangentData>>> StoredTangents; //store tangents so we can reset them
					TArray<FFrameNumber> Frames;
					FFrameNumber FrameRateInFrameNumber = TickResolution.AsFrameNumber(FrameRate.AsInterval());
					FrameRateInFrameNumber.Value *= InSettings.FrameIncrement;
					for (FFrameNumber& Frame = StartFrame; Frame <= EndFrame; Frame += FrameRateInFrameNumber)
					{
						Frames.Add(Frame);
					}

					//Store transforms
					TArray<TPair<FName, TArray<FEulerTransform>>> ControlLocalTransforms;
					TArray<FTLLRN_RigControlElement*> Controls;
					TLLRN_ControlRig->GetControlsInOrder(Controls);

					for (FTLLRN_RigControlElement* ControlElement : Controls)
					{
						if (!TLLRN_ControlRig->GetHierarchy()->IsAnimatable(ControlElement))
						{
							continue;
						}
						TPair<FName, TArray<FEulerTransform>> NameTransforms;
						NameTransforms.Key = ControlElement->GetFName();
						NameTransforms.Value.SetNum(Frames.Num());
						ControlLocalTransforms.Add(NameTransforms);
					}

					FMovieSceneInverseSequenceTransform LocalToRootTransform = RootToLocalTransform.Inverse();

					//get all of the local 
					int32 Index = 0;
					for (Index = 0; Index < Frames.Num(); ++Index)
					{
						const FFrameNumber& FrameNumber = Frames[Index];
						FFrameTime GlobalTime = LocalToRootTransform.TryTransformTime(FrameNumber).Get(FrameNumber);

						FMovieSceneContext Context = FMovieSceneContext(FMovieSceneEvaluationRange(GlobalTime, TickResolution), SequencerPtr->GetPlaybackStatus()).SetHasJumped(true);

						SequencerPtr->GetEvaluationTemplate().EvaluateSynchronousBlocking(Context);
						TLLRN_ControlRig->Evaluate_AnyThread();
						for (TPair<FName, TArray<FEulerTransform>>& TrailControlTransform : ControlLocalTransforms)
						{
							FEulerTransform EulerTransform(TLLRN_ControlRig->GetControlLocalTransform(TrailControlTransform.Key));
							FTLLRN_RigElementKey ControlKey(TrailControlTransform.Key, ETLLRN_RigElementType::Control);
							EulerTransform.Rotation = TLLRN_ControlRig->GetHierarchy()->GetControlPreferredRotator(ControlKey);
							TrailControlTransform.Value[Index] = EulerTransform;
						}
					}
					//delete other sections
					OwnerTrack->Modify();
					for (Index = Sections.Num() - 1; Index >= 0; --Index)
					{
						if (Sections[Index] != ParameterSection)
						{
							OwnerTrack->RemoveSectionAt(Index);
						}
					}

					//remove all keys, except Space Channels, from the Section.
					ParameterSection->RemoveAllKeys(false /*bIncludedSpaceKeys*/);

					FTLLRN_RigControlModifiedContext Context;
					Context.SetKey = ETLLRN_ControlRigSetKey::Always;

					FScopedSlowTask Feedback(Frames.Num(), LOCTEXT("CollapsingSections", "Collapsing Sections"));
					Feedback.MakeDialog(true);

					const EMovieSceneKeyInterpolation InterpMode = SequencerPtr->GetSequencerSettings()->GetKeyInterpolation();
					Index = 0;
					for (Index = 0; Index < Frames.Num(); ++Index)
					{
						Feedback.EnterProgressFrame(1, LOCTEXT("CollapsingSections", "Collapsing Sections"));
						const FFrameNumber& FrameNumber = Frames[Index];
						Context.LocalTime = TickResolution.AsSeconds(FFrameTime(FrameNumber));
						//need to do the twice hack since controls aren't really in order
						for (int32 TwiceHack = 0; TwiceHack < 2; ++TwiceHack)
						{
							for (TPair<FName, TArray<FEulerTransform>>& TrailControlTransform : ControlLocalTransforms)
							{
								FTLLRN_RigElementKey ControlKey(TrailControlTransform.Key, ETLLRN_RigElementType::Control);
								TLLRN_ControlRig->GetHierarchy()->SetControlPreferredRotator(ControlKey, TrailControlTransform.Value[Index].Rotation);
								FTransform Transform(TrailControlTransform.Value[Index].ToFTransform());
								TLLRN_ControlRig->SetControlLocalTransform(TrailControlTransform.Key, Transform, false, Context, false /*undo*/, true/* bFixEulerFlips*/);
								TLLRN_ControlRig->GetHierarchy()->SetControlPreferredRotator(ControlKey, TrailControlTransform.Value[Index].Rotation);

							}
						}
						TLLRN_ControlRig->Evaluate_AnyThread();
						ParameterSection->RecordTLLRN_ControlRigKey(FrameNumber, true, InterpMode);

						if (Feedback.ShouldCancel())
						{
							Transaction.Cancel();
							SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
							return false;
						}
					}
					if (InSettings.bReduceKeys)
					{
						FKeyDataOptimizationParams Params;
						Params.bAutoSetInterpolation = true;
						Params.Tolerance = InSettings.Tolerance;
						FMovieSceneChannelProxy& ChannelProxy = ParameterSection->GetChannelProxy();
						TArrayView<FMovieSceneFloatChannel*> FloatChannels = ChannelProxy.GetChannels<FMovieSceneFloatChannel>();

						for (FMovieSceneFloatChannel* Channel : FloatChannels)
						{
							Channel->Optimize(Params); //should also auto tangent
						}
					}
					//reset everything back
					SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
					return true;
				}
				else
				{
					UE_LOG(LogTLLRN_ControlRigEditor, Log, TEXT("CollapseAllSections:: First section is not additive"));
					return false;
				}
			}
			else
			{
				UE_LOG(LogTLLRN_ControlRigEditor, Log, TEXT("CollapseAllSections:: No Control Rig section"));
				return false;
			}
		}
		UE_LOG(LogTLLRN_ControlRigEditor, Log, TEXT("CollapseAllSections:: Sequencer or track is invalid"));
	}
	return false;
}

void FTLLRN_ControlRigParameterSection::CollapseAllLayers()
{
	if (UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection = CastChecked<UMovieSceneTLLRN_ControlRigParameterSection>(WeakSection.Get()))
	{
		UMovieSceneTrack* OwnerTrack = ParameterSection->GetTypedOuter<UMovieSceneTrack>();
		FCollapseControlsCB CollapseCB = FCollapseControlsCB::CreateLambda([this,OwnerTrack](TSharedPtr<ISequencer>& InSequencer, const FBakingAnimationKeySettings& InSettings)
		{
			FTLLRN_ControlRigParameterTrackEditor::CollapseAllLayers(InSequencer, OwnerTrack, InSettings);
		});

		TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
		TSharedRef<SCollapseControlsWidget> BakeWidget =
			SNew(SCollapseControlsWidget)
			.Sequencer(Sequencer);

		BakeWidget->SetCollapseCB(CollapseCB);
		BakeWidget->OpenDialog(false);
	}
}

void FTLLRN_ControlRigParameterSection::KeyZeroValue()
{
	UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection = CastChecked<UMovieSceneTLLRN_ControlRigParameterSection>(WeakSection.Get());
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
	FScopedTransaction Transaction(LOCTEXT("KeyZeroValue", "Key Zero Value"));
	ParameterSection->Modify();
	FFrameTime Time = SequencerPtr->GetLocalTime().Time;
	EMovieSceneKeyInterpolation DefaultInterpolation = SequencerPtr->GetKeyInterpolation();
	ParameterSection->KeyZeroValue(Time.GetFrame(), DefaultInterpolation, true);
	SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
}

void FTLLRN_ControlRigParameterSection::KeyWeightValue(float Val)
{
	UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection = CastChecked<UMovieSceneTLLRN_ControlRigParameterSection>(WeakSection.Get());
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
	FScopedTransaction Transaction(LOCTEXT("KeyWeightZero", "Key Weight Zero"));
	ParameterSection->Modify();
	EMovieSceneTransformChannel Channels = ParameterSection->GetTransformMask().GetChannels();
	if ((Channels & EMovieSceneTransformChannel::Weight) == EMovieSceneTransformChannel::None)
	{
		ParameterSection->SetTransformMask(ParameterSection->GetTransformMask().GetChannels() | EMovieSceneTransformChannel::Weight);
		SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
	}
	FFrameTime Time = SequencerPtr->GetLocalTime().Time;
	EMovieSceneKeyInterpolation DefaultInterpolation = SequencerPtr->GetKeyInterpolation();
	ParameterSection->KeyWeightValue(Time.GetFrame(), DefaultInterpolation, Val);
	SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChanged);
}

void FTLLRN_ControlRigParameterSection::BuildSectionContextMenu(FMenuBuilder& MenuBuilder, const FGuid& InObjectBinding)
{
	UMovieSceneTLLRN_ControlRigParameterSection* const ParameterSection = CastChecked<UMovieSceneTLLRN_ControlRigParameterSection>(WeakSection.Get());
	if (!IsValid(ParameterSection))
	{
		return;
	}

	UTLLRN_ControlRig* const TLLRN_ControlRig = ParameterSection->GetTLLRN_ControlRig();
	if (!IsValid(TLLRN_ControlRig))
	{
		return;
	}

	UFKTLLRN_ControlRig* AutoRig = Cast<UFKTLLRN_ControlRig>(TLLRN_ControlRig);
	if (AutoRig || TLLRN_ControlRig->SupportsEvent(FTLLRN_RigUnit_InverseExecution::EventName))
	{
		UObject* BoundObject = nullptr;
		USkeleton* Skeleton = AcquireSkeletonFromObjectGuid(InObjectBinding, &BoundObject, WeakSequencer.Pin());

		if (Skeleton)
		{
			// Load the asset registry module
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

			// Collect a full list of assets with the specified class
			TArray<FAssetData> AssetDataList;
			AssetRegistryModule.Get().GetAssetsByClass(UAnimSequenceBase::StaticClass()->GetClassPathName(), AssetDataList, true);

			if (AssetDataList.Num())
			{
				MenuBuilder.AddSubMenu(
					LOCTEXT("ImportAnimSequenceIntoThisSection", "Import Anim Sequence Into This Section"), NSLOCTEXT("Sequencer", "ImportAnimSequenceIntoThisSectionTP", "Import Anim Sequence Into This Section"),
					FNewMenuDelegate::CreateRaw(this, &FTLLRN_ControlRigParameterSection::AddAnimationSubMenuForFK, InObjectBinding, Skeleton, ParameterSection)
				);
			}
		}
	}
	TArray<FTLLRN_RigControlElement*> Controls;
	TLLRN_ControlRig->GetControlsInOrder(Controls);

	auto MakeUIAction = [this, InObjectBinding](EMovieSceneTransformChannel ChannelsToToggle)
	{
		return FUIAction(
			FExecuteAction::CreateLambda([this, InObjectBinding, ChannelsToToggle]
				{
					const TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
					if (!SequencerPtr)
					{
						return;
					}

					UMovieSceneTLLRN_ControlRigParameterSection* const ParameterSection = CastChecked<UMovieSceneTLLRN_ControlRigParameterSection>(WeakSection.Get());
					if (!IsValid(ParameterSection))
					{
						return;
					}

					FScopedTransaction Transaction(LOCTEXT("SetActiveChannelsTransaction", "Set Active Channels"));
					ParameterSection->Modify();
					EMovieSceneTransformChannel Channels = ParameterSection->GetTransformMask().GetChannels();

					if (EnumHasAllFlags(Channels, ChannelsToToggle) || (Channels & ChannelsToToggle) == EMovieSceneTransformChannel::None)
					{
						ParameterSection->SetTransformMask(ParameterSection->GetTransformMask().GetChannels() ^ ChannelsToToggle);
					}
					else
					{
						ParameterSection->SetTransformMask(ParameterSection->GetTransformMask().GetChannels() | ChannelsToToggle);
					}

					// Restore pre-animated state for the bound objects so that inactive channels will return to their default values.
					for (TWeakObjectPtr<> WeakObject : SequencerPtr->FindBoundObjects(InObjectBinding, SequencerPtr->GetFocusedTemplateID()))
					{
						if (UObject* Object = WeakObject.Get())
						{
							SequencerPtr->RestorePreAnimatedState();
						}
					}

					SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
				}
			),
			FCanExecuteAction(),
			FGetActionCheckState::CreateLambda([this, ChannelsToToggle]
				{
					const UMovieSceneTLLRN_ControlRigParameterSection* const ParameterSection = CastChecked<UMovieSceneTLLRN_ControlRigParameterSection>(WeakSection.Get());
					if (!IsValid(ParameterSection))
					{
						return ECheckBoxState::Unchecked;
					}

					const EMovieSceneTransformChannel Channels = ParameterSection->GetTransformMask().GetChannels();
					if (EnumHasAllFlags(Channels, ChannelsToToggle))
					{
						return ECheckBoxState::Checked;
					}
					else if (EnumHasAnyFlags(Channels, ChannelsToToggle))
					{
						return ECheckBoxState::Undetermined;
					}
					return ECheckBoxState::Unchecked;
				})
			);
	};
	
	UMovieSceneTLLRN_ControlRigParameterTrack* Track = ParameterSection->GetTypedOuter<UMovieSceneTLLRN_ControlRigParameterTrack>();
	if (Track)
	{
		TArray<UMovieSceneSection*> Sections = Track->GetAllSections();
		//If Base Absolute section 
		if (ParameterSection->GetBlendType().Get() == EMovieSceneBlendType::Absolute && Sections[0] == ParameterSection)
		{
			MenuBuilder.BeginSection(NAME_None, LOCTEXT("AnimationLayers", "Animation Layers"));
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("CollapseAllSections", "Collapse All Sections"),
					LOCTEXT("CollapseAllSections_ToolTip", "Collapse all sections onto this section"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([this] { CollapseAllLayers(); }))
				);
			}
		}
		if (ParameterSection->GetBlendType().Get() == EMovieSceneBlendType::Additive)
		{
			MenuBuilder.BeginSection(NAME_None, LOCTEXT("AnimationLayers", "Animation Layers"));
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("KeyZeroValue", "Key Zero Value"),
					LOCTEXT("KeyZeroValue_Tooltip", "Set zero key on all controls in this section"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([this] { KeyZeroValue(); }))
				);
			}

			MenuBuilder.AddMenuEntry(
				LOCTEXT("KeyWeightZero", "Key Weight Zero"),
				LOCTEXT("KeyWeightZero_Tooltip", "Key a zero value on the Weight channel"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this] { KeyWeightValue(0.0f); }))
			);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("KeyWeightOne", "Key Weight One"),
				LOCTEXT("KeyWeightOne_Tooltip", "Key a one value on the Weight channel"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this] { KeyWeightValue(1.0f); }))
			);
		}
	}
	MenuBuilder.BeginSection(NAME_None, LOCTEXT("RigSectionActiveChannels", "Active Channels"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SetFromSelectedControls", "Set From Selected Controls"),
			LOCTEXT("SetFromSelectedControls_ToolTip", "Set active channels from the current control selection"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this] { ShowSelectedControlsChannels(); }),
				FCanExecuteAction::CreateLambda([TLLRN_ControlRig] { return TLLRN_ControlRig->CurrentControlSelection().Num() > 0; } )
			)
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowAllControls", "Show All Controls"),
			LOCTEXT("ShowAllControls_ToolTip", "Set active channels from all controls"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this] { return ShowAllControlsChannels(); }))
		);

		MenuBuilder.AddSubMenu(
			LOCTEXT("AllTranslation", "Translation"), LOCTEXT("AllTranslation_ToolTip", "Causes this section to affect the translation of rig control transforms"),
			FNewMenuDelegate::CreateLambda([=](FMenuBuilder& SubMenuBuilder) {
				SubMenuBuilder.AddMenuEntry(
					LOCTEXT("TranslationX", "X"), LOCTEXT("TranslationX_ToolTip", "Causes this section to affect the X channel of the transform's translation"),
					FSlateIcon(), MakeUIAction(EMovieSceneTransformChannel::TranslationX), NAME_None, EUserInterfaceActionType::ToggleButton);
				SubMenuBuilder.AddMenuEntry(
					LOCTEXT("TranslationY", "Y"), LOCTEXT("TranslationY_ToolTip", "Causes this section to affect the Y channel of the transform's translation"),
					FSlateIcon(), MakeUIAction(EMovieSceneTransformChannel::TranslationY), NAME_None, EUserInterfaceActionType::ToggleButton);
				SubMenuBuilder.AddMenuEntry(
					LOCTEXT("TranslationZ", "Z"), LOCTEXT("TranslationZ_ToolTip", "Causes this section to affect the Z channel of the transform's translation"),
					FSlateIcon(), MakeUIAction(EMovieSceneTransformChannel::TranslationZ), NAME_None, EUserInterfaceActionType::ToggleButton);
				}),
			MakeUIAction(EMovieSceneTransformChannel::Translation),
					NAME_None,
					EUserInterfaceActionType::ToggleButton);

		MenuBuilder.AddSubMenu(
			LOCTEXT("AllRotation", "Rotation"), LOCTEXT("AllRotation_ToolTip", "Causes this section to affect the rotation of the rig control transform"),
			FNewMenuDelegate::CreateLambda([=](FMenuBuilder& SubMenuBuilder) {
				SubMenuBuilder.AddMenuEntry(
					LOCTEXT("RotationX", "Roll (X)"), LOCTEXT("RotationX_ToolTip", "Causes this section to affect the roll (X) channel the transform's rotation"),
					FSlateIcon(), MakeUIAction(EMovieSceneTransformChannel::RotationX), NAME_None, EUserInterfaceActionType::ToggleButton);
				SubMenuBuilder.AddMenuEntry(
					LOCTEXT("RotationY", "Pitch (Y)"), LOCTEXT("RotationY_ToolTip", "Causes this section to affect the pitch (Y) channel the transform's rotation"),
					FSlateIcon(), MakeUIAction(EMovieSceneTransformChannel::RotationY), NAME_None, EUserInterfaceActionType::ToggleButton);
				SubMenuBuilder.AddMenuEntry(
					LOCTEXT("RotationZ", "Yaw (Z)"), LOCTEXT("RotationZ_ToolTip", "Causes this section to affect the yaw (Z) channel the transform's rotation"),
					FSlateIcon(), MakeUIAction(EMovieSceneTransformChannel::RotationZ), NAME_None, EUserInterfaceActionType::ToggleButton);
				}),
			MakeUIAction(EMovieSceneTransformChannel::Rotation),
					NAME_None,
					EUserInterfaceActionType::ToggleButton);

		MenuBuilder.AddSubMenu(
			LOCTEXT("AllScale", "Scale"), LOCTEXT("AllScale_ToolTip", "Causes this section to affect the scale of the rig control transform"),
			FNewMenuDelegate::CreateLambda([=](FMenuBuilder& SubMenuBuilder) {
				SubMenuBuilder.AddMenuEntry(
					LOCTEXT("ScaleX", "X"), LOCTEXT("ScaleX_ToolTip", "Causes this section to affect the X channel of the transform's scale"),
					FSlateIcon(), MakeUIAction(EMovieSceneTransformChannel::ScaleX), NAME_None, EUserInterfaceActionType::ToggleButton);
				SubMenuBuilder.AddMenuEntry(
					LOCTEXT("ScaleY", "Y"), LOCTEXT("ScaleY_ToolTip", "Causes this section to affect the Y channel of the transform's scale"),
					FSlateIcon(), MakeUIAction(EMovieSceneTransformChannel::ScaleY), NAME_None, EUserInterfaceActionType::ToggleButton);
				SubMenuBuilder.AddMenuEntry(
					LOCTEXT("ScaleZ", "Z"), LOCTEXT("ScaleZ_ToolTip", "Causes this section to affect the Z channel of the transform's scale"),
					FSlateIcon(), MakeUIAction(EMovieSceneTransformChannel::ScaleZ), NAME_None, EUserInterfaceActionType::ToggleButton);
				}),
			MakeUIAction(EMovieSceneTransformChannel::Scale),
					NAME_None,
					EUserInterfaceActionType::ToggleButton);

		//mz todo h
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Weight", "Weight"), LOCTEXT("Weight_ToolTip", "Causes this section to be applied with a user-specified weight curve"),
			FSlateIcon(), MakeUIAction(EMovieSceneTransformChannel::Weight), NAME_None, EUserInterfaceActionType::ToggleButton);
	}
	MenuBuilder.EndSection();
}

void FTLLRN_ControlRigParameterSection::ShowSelectedControlsChannels()
{
	UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection = CastChecked<UMovieSceneTLLRN_ControlRigParameterSection>(WeakSection.Get());
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
	UTLLRN_ControlRig* TLLRN_ControlRig = ParameterSection ? ParameterSection->GetTLLRN_ControlRig() : nullptr;

	if (ParameterSection && TLLRN_ControlRig && SequencerPtr.IsValid())
	{
		FScopedTransaction Transaction(LOCTEXT("ShowSelecedControlChannels", "Show Selected Control Channels"));
		ParameterSection->Modify();
		ParameterSection->FillControlNameMask(false);

		TArray<FTLLRN_RigControlElement*> Controls;
		TLLRN_ControlRig->GetControlsInOrder(Controls);
		for (const FTLLRN_RigControlElement* TLLRN_RigControl : Controls)
		{
			const FName TLLRN_RigName = TLLRN_RigControl->GetFName();
			if (TLLRN_ControlRig->IsControlSelected(TLLRN_RigName))
			{
				FTLLRN_ChannelMapInfo* pChannelIndex = ParameterSection->ControlChannelMap.Find(TLLRN_RigName);
				if (pChannelIndex)
				{
					ParameterSection->SetControlNameMask(TLLRN_RigName, true);
				}
			}
		}
		SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
	}
}

void FTLLRN_ControlRigParameterSection::ShowAllControlsChannels()
{
	UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection = CastChecked<UMovieSceneTLLRN_ControlRigParameterSection>(WeakSection.Get());
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
	if (ParameterSection && SequencerPtr.IsValid())
	{
		FScopedTransaction Transaction(LOCTEXT("ShowAllControlChannels", "Show All Control Channels"));
		ParameterSection->Modify();
		ParameterSection->FillControlNameMask(true);
		SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
	}
}

//mz todo
bool FTLLRN_ControlRigParameterSection::RequestDeleteCategory(const TArray<FName>& CategoryNamePaths)
{
	UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection = CastChecked<UMovieSceneTLLRN_ControlRigParameterSection>(WeakSection.Get());
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();

	if (ParameterSection && SequencerPtr)
	{
		const FName& ChannelName = CategoryNamePaths.Last();
		const int32 Index = ParameterSection->GetConstraintsChannels().IndexOfByPredicate([ChannelName](const FConstraintAndActiveChannel& InChannel)
			{
				return InChannel.GetConstraint().Get() ? InChannel.GetConstraint()->GetFName() == ChannelName : false;
			});
		// remove constraint channel if there are no keys
		const FConstraintAndActiveChannel* ConstraintChannel = Index != INDEX_NONE ? &(ParameterSection->GetConstraintsChannels()[Index]): nullptr;
		if (ConstraintChannel && ConstraintChannel->ActiveChannel.GetNumKeys() == 0)
		{
			if (ParameterSection->TryModify())
			{
				ParameterSection->RemoveConstraintChannel(ConstraintChannel->GetConstraint().Get());
				SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
				return true;
			}
		}
	}
	
	/*
	const FScopedTransaction Transaction(LOCTEXT("DeleteTransformCategory", "Delete transform category"));

	if (ParameterSection->TryModify())
	{
	FName CategoryName = CategoryNamePaths[CategoryNamePaths.Num() - 1];

	EMovieSceneTransformChannel Channel = ParameterSection->GetTransformMask().GetChannels();
	EMovieSceneTransformChannel ChannelToRemove = ParameterSection->GetTransformMaskByName(CategoryName).GetChannels();

	Channel = Channel ^ ChannelToRemove;

	ParameterSection->SetTransformMask(Channel);

	SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
	return true;
	}
	*/
	return false;
}

bool FTLLRN_ControlRigParameterSection::RequestDeleteKeyArea(const TArray<FName>& KeyAreaNamePaths)
{
	UMovieSceneTLLRN_ControlRigParameterSection* ParameterSection = CastChecked<UMovieSceneTLLRN_ControlRigParameterSection>(WeakSection.Get());
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();

	/*
	const FScopedTransaction Transaction(LOCTEXT("DeleteTransformChannel", "Delete transform channel"));

	if (ParameterSection->TryModify())
	{
	// Only delete the last key area path which is the channel. ie. TranslationX as opposed to Translation
	FName KeyAreaName = KeyAreaNamePaths[KeyAreaNamePaths.Num() - 1];

	EMovieSceneTransformChannel Channel = ParameterSection->GetTransformMask().GetChannels();
	EMovieSceneTransformChannel ChannelToRemove = ParameterSection->GetTransformMaskByName(KeyAreaName).GetChannels();

	Channel = Channel ^ ChannelToRemove;

	ParameterSection->SetTransformMask(Channel);

	SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
	return true;
	}
	*/
	return true;
}


void FTLLRN_ControlRigParameterSection::AddAnimationSubMenuForFK(FMenuBuilder& MenuBuilder, FGuid ObjectBinding, USkeleton* Skeleton, UMovieSceneTLLRN_ControlRigParameterSection* Section)
{
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
	UMovieSceneSequence* Sequence = SequencerPtr.IsValid() ? SequencerPtr->GetFocusedMovieSceneSequence() : nullptr;

	FAssetPickerConfig AssetPickerConfig;
	{
		AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateRaw(this, &FTLLRN_ControlRigParameterSection::OnAnimationAssetSelectedForFK, ObjectBinding, Section);
		AssetPickerConfig.OnAssetEnterPressed = FOnAssetEnterPressed::CreateRaw(this, &FTLLRN_ControlRigParameterSection::OnAnimationAssetEnterPressedForFK, ObjectBinding, Section);
		AssetPickerConfig.bAllowNullSelection = false;
		AssetPickerConfig.bAddFilterUI = true;
		AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
		AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateRaw(this, &FTLLRN_ControlRigParameterSection::ShouldFilterAssetForFK);
		AssetPickerConfig.Filter.bRecursiveClasses = true;
		AssetPickerConfig.Filter.ClassPaths.Add(UAnimSequenceBase::StaticClass()->GetClassPathName());
		AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateUObject(Skeleton, &USkeleton::ShouldFilterAsset, TEXT("Skeleton"));
		AssetPickerConfig.SaveSettingsName = TEXT("SequencerAssetPicker");
		AssetPickerConfig.AdditionalReferencingAssets.Add(FAssetData(Sequence));
	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	const float WidthOverride = SequencerPtr.IsValid() ? SequencerPtr->GetSequencerSettings()->GetAssetBrowserWidth() : 500.f;
	const float HeightOverride = SequencerPtr.IsValid() ? SequencerPtr->GetSequencerSettings()->GetAssetBrowserHeight() : 400.f;

	TSharedPtr<SBox> MenuEntry = SNew(SBox)
		.WidthOverride(WidthOverride)
		.HeightOverride(HeightOverride)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		];

	MenuBuilder.AddWidget(MenuEntry.ToSharedRef(), FText::GetEmpty(), true);
}


void FTLLRN_ControlRigParameterSection::OnAnimationAssetSelectedForFK(const FAssetData& AssetData, FGuid ObjectBinding, UMovieSceneTLLRN_ControlRigParameterSection* Section)
{
	FSlateApplication::Get().DismissAllMenus();

	UObject* SelectedObject = AssetData.GetAsset();
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();

	if (SelectedObject && SelectedObject->IsA(UAnimSequence::StaticClass()) && SequencerPtr.IsValid())
	{
		UAnimSequence* AnimSequence = Cast<UAnimSequence>(AssetData.GetAsset());
		UObject* BoundObject = nullptr;
		AcquireSkeletonFromObjectGuid(ObjectBinding, &BoundObject, SequencerPtr);
		USkeletalMeshComponent* SkelMeshComp = AcquireSkeletalMeshFromObject(BoundObject, SequencerPtr);

		if (AnimSequence && SkelMeshComp && AnimSequence->GetDataModel()->GetNumBoneTracks() > 0)
		{
			FBakeToControlDelegate BakeCallback = FBakeToControlDelegate::CreateLambda([this, Section,
				AnimSequence, SkelMeshComp]
				(bool bKeyReduce, float KeyReduceTolerance, FFrameRate BakeFrameRate, bool bResetControls)
				{
					if (TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin())
					{
						FScopedTransaction Transaction(LOCTEXT("BakeAnimation_Transaction", "Bake Animation To FK Control Rig"));
						Section->Modify();
						FFrameNumber StartFrame = SequencerPtr->GetLocalTime().Time.GetFrame();
						FSmartReduceParams SmartReduce;
						SmartReduce.TolerancePercentage = KeyReduceTolerance;
						SmartReduce.SampleRate = BakeFrameRate;
						if (!FTLLRN_ControlRigParameterTrackEditor::LoadAnimationIntoSection(SequencerPtr, AnimSequence, SkelMeshComp, StartFrame,
							bKeyReduce, SmartReduce, bResetControls, Section))
						{
							Transaction.Cancel();
						}
						SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
					}
					
				});

			FOnWindowClosed BakeClosedCallback = FOnWindowClosed::CreateLambda([](const TSharedRef<SWindow>&) {});
			BakeToTLLRN_ControlRigDialog::GetBakeParams(BakeCallback, BakeClosedCallback);
		}
	}
}

bool FTLLRN_ControlRigParameterSection::ShouldFilterAssetForFK(const FAssetData& AssetData)
{
	// we don't want 

	if (AssetData.AssetClassPath == UAnimMontage::StaticClass()->GetClassPathName())
	{
		return true;
	}

	const FString EnumString = AssetData.GetTagValueRef<FString>(GET_MEMBER_NAME_CHECKED(UAnimSequence, AdditiveAnimType));
	if (EnumString.IsEmpty())
	{
		return false;
	}

	UEnum* AdditiveTypeEnum = StaticEnum<EAdditiveAnimationType>();
	return ((EAdditiveAnimationType)AdditiveTypeEnum->GetValueByName(*EnumString) != AAT_None);

}

void FTLLRN_ControlRigParameterSection::OnAnimationAssetEnterPressedForFK(const TArray<FAssetData>& AssetData, FGuid  ObjectBinding, UMovieSceneTLLRN_ControlRigParameterSection* Section)
{
	if (AssetData.Num() > 0)
	{
		OnAnimationAssetSelectedForFK(AssetData[0].GetAsset(), ObjectBinding, Section);
	}
}

FEditorModeTools* FTLLRN_ControlRigParameterTrackEditor::GetEditorModeTools() const
{
	TSharedPtr<ISequencer> SharedSequencer = GetSequencer();
	if (SharedSequencer.IsValid())
	{
		TSharedPtr<IToolkitHost> ToolkitHost = SharedSequencer->GetToolkitHost();
		if (ToolkitHost.IsValid())
		{
			return &ToolkitHost->GetEditorModeManager();
		}
	}

	return nullptr;
}

FTLLRN_ControlRigEditMode* FTLLRN_ControlRigParameterTrackEditor::GetEditMode(bool bForceActivate /*= false*/) const
{
	if (FEditorModeTools* EditorModetools = GetEditorModeTools())
	{
		if (bForceActivate && !EditorModetools->IsModeActive(FTLLRN_ControlRigEditMode::ModeName))
		{
			EditorModetools->ActivateMode(FTLLRN_ControlRigEditMode::ModeName);
			if (bForceActivate)
			{
				FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(EditorModetools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
				if (EditMode && EditMode->GetToolkit().IsValid() == false)
				{
					EditMode->Enter();
				}
			}
		}

		return static_cast<FTLLRN_ControlRigEditMode*>(EditorModetools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
	}

	return nullptr;
}


#undef LOCTEXT_NAMESPACE
