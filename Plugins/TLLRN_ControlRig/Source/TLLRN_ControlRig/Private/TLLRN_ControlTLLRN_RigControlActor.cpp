// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlTLLRN_RigControlActor.h"
#include "Components/StaticMeshComponent.h"
#include "TLLRN_ControlRigGizmoLibrary.h"
#include "ITLLRN_ControlRigObjectBinding.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "TLLRN_ControlRig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlTLLRN_RigControlActor)

#define LOCTEXT_NAMESPACE "TLLRN_ControlTLLRN_RigControlActor"

ATLLRN_ControlTLLRN_RigControlActor::ATLLRN_ControlTLLRN_RigControlActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bRefreshOnTick(true)
	, bIsSelectable(true)
	, ColorParameter(TEXT("Color"))
	, bCastShadows(false)
{
	ActorRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent0"));
	SetRootComponent(ActorRootComponent);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	PrimaryActorTick.bStartWithTickEnabled = true;
	bAllowTickBeforeBeginPlay = true;
	
	SetActorEnableCollision(false);
	

	Refresh();
}

ATLLRN_ControlTLLRN_RigControlActor::~ATLLRN_ControlTLLRN_RigControlActor()
{
	RemoveUnbindDelegate();
}

void ATLLRN_ControlTLLRN_RigControlActor::RemoveUnbindDelegate() const
{
	if (TLLRN_ControlRig)
	{
		if (TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = TLLRN_ControlRig->GetObjectBinding())
		{
			Binding->OnTLLRN_ControlRigUnbind().RemoveAll(this);
		}
	}
}

void ATLLRN_ControlTLLRN_RigControlActor::HandleTLLRN_ControlRigUnbind()
{
	Clear();
	Refresh();
}

#if WITH_EDITOR

void ATLLRN_ControlTLLRN_RigControlActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property &&
		(	PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ATLLRN_ControlTLLRN_RigControlActor, ActorToTrack) ||
			PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ATLLRN_ControlTLLRN_RigControlActor, TLLRN_ControlRigClass) ||
			PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ATLLRN_ControlTLLRN_RigControlActor, MaterialOverride) ||
			PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ATLLRN_ControlTLLRN_RigControlActor, ColorParameter) ||
			PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ATLLRN_ControlTLLRN_RigControlActor, bCastShadows)))
	{
		ResetControlActor();
		Refresh();
	}
}

#endif

void ATLLRN_ControlTLLRN_RigControlActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bRefreshOnTick)
	{
		Refresh();
	}
}

void ATLLRN_ControlTLLRN_RigControlActor::Clear()
{
	if (ActorRootComponent)
	{
		TArray<USceneComponent*> ChildComponents;
		ActorRootComponent->GetChildrenComponents(true, ChildComponents);
		for (USceneComponent* Child : ChildComponents)
		{
			if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Child))
			{
				if (IsValid(StaticMeshComponent))
				{
					Components.AddUnique(StaticMeshComponent);	
				}
			}
		}

		for (UStaticMeshComponent* Component : Components)
		{
			if (IsValid(Component))
			{
				Component->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
				Component->UnregisterComponent();
				Component->DestroyComponent();	
			}
		}
	}

	ControlNames.Reset();
	ShapeTransforms.Reset();
	Components.Reset();
	Materials.Reset();
}

void ATLLRN_ControlTLLRN_RigControlActor::ResetControlActor()
{
	RemoveUnbindDelegate();
	TLLRN_ControlRig = nullptr;
	Clear();
}

void ATLLRN_ControlTLLRN_RigControlActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetControlActor();
	Super::EndPlay(EndPlayReason);
}

void ATLLRN_ControlTLLRN_RigControlActor::BeginDestroy()
{
	// since end play is not always called, we have to clear the delegate here
	// clearing it at destructor might be too late as in some cases, the control rig was already GCed
	ResetControlActor();
	Super::BeginDestroy();
}

void ATLLRN_ControlTLLRN_RigControlActor::Refresh()
{
	if (ActorToTrack == nullptr)
	{
		return;
	}

	// workaround to make sure we get a clean reset when the object was initialized from an existing template instance but some arrays didn't get populated somehow...
	if (ControlNames.IsEmpty() || ControlNames.Num() != Components.Num() || ControlNames.Num() != Materials.Num() || ControlNames.Num() != ShapeTransforms.Num())
	{
		ResetControlActor();
	}

	if (TLLRN_ControlRig == nullptr)
	{
		TArray<UTLLRN_ControlRig*> Rigs = UTLLRN_ControlRig::FindTLLRN_ControlRigs(ActorToTrack, TLLRN_ControlRigClass);
		if(Rigs.Num() > 0)
		{
			TLLRN_ControlRig = Rigs[0];
		}
		
		if (TLLRN_ControlRig == nullptr)
		{
			return;
		}

		RemoveUnbindDelegate();
		if (const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = TLLRN_ControlRig->GetObjectBinding())
		{
			Binding->OnTLLRN_ControlRigUnbind().AddUObject(this, &ATLLRN_ControlTLLRN_RigControlActor::HandleTLLRN_ControlRigUnbind);
		}

		const TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>>& ShapeLibraries = TLLRN_ControlRig->GetShapeLibraries();
		if(ShapeLibraries.IsEmpty())
		{
			return;
		}
		
		// disable collision again
		SetActorEnableCollision(false);

		for(const TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>& ShapeLibrary : ShapeLibraries)
		{
			ShapeLibrary->DefaultMaterial.LoadSynchronous();
		}

		UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
		Hierarchy->ForEach<FTLLRN_RigControlElement>([this, ShapeLibraries, Hierarchy](FTLLRN_RigControlElement* ControlElement) -> bool
        {
			if (!ControlElement->Settings.SupportsShape())
			{
				return true;
			}

			switch (ControlElement->Settings.ControlType)
			{
				case ETLLRN_RigControlType::Float:
				case ETLLRN_RigControlType::ScaleFloat:
				case ETLLRN_RigControlType::Integer:
				case ETLLRN_RigControlType::Vector2D:
				case ETLLRN_RigControlType::Position:
				case ETLLRN_RigControlType::Scale:
				case ETLLRN_RigControlType::Rotator:
				case ETLLRN_RigControlType::Transform:
				case ETLLRN_RigControlType::TransformNoScale:
				case ETLLRN_RigControlType::EulerTransform:
				{
					if (const FTLLRN_ControlRigShapeDefinition* ShapeDef = UTLLRN_ControlRigShapeLibrary::GetShapeByName(ControlElement->Settings.ShapeName, ShapeLibraries, TLLRN_ControlRig->ShapeLibraryNameMap))
					{
						UMaterialInterface* BaseMaterial = nullptr;
						if (MaterialOverride && !ColorParameter.IsEmpty())
						{
							BaseMaterial = MaterialOverride;
							ColorParameterName = *ColorParameter;
						}
						else
						{
							if(!ShapeDef->Library.IsValid())
							{
								return true;
							}

							if(ShapeDef->Library->DefaultMaterial.IsValid())
							{
								BaseMaterial = ShapeDef->Library->DefaultMaterial.Get();
							}
							else
							{
								BaseMaterial = ShapeDef->Library->DefaultMaterial.LoadSynchronous();
							}
							ColorParameterName = ShapeDef->Library->MaterialColorParameter;
						}

						UStaticMesh* StaticMesh = ShapeDef->StaticMesh.LoadSynchronous();
						UStaticMeshComponent* Component = NewObject< UStaticMeshComponent>(ActorRootComponent);
						Component->SetStaticMesh(StaticMesh);
						Component->SetupAttachment(ActorRootComponent);
						Component->RegisterComponent();

						Component->bCastStaticShadow = bCastShadows;
						Component->bCastDynamicShadow = bCastShadows;

						UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, Component);
						for (int32 i=0; i<Component->GetNumMaterials(); ++i)
						{
							Component->SetMaterial(i, MaterialInstance);
						}

						ControlNames.Add(ControlElement->GetFName());
						ShapeTransforms.Add(ShapeDef->Transform * Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal));
						Components.Add(Component);
						Materials.Add(MaterialInstance);
					}
				}
				default:
				{
					break;
				}
			}
			return true;
		});
	}

	for (int32 GizmoIndex = 0; GizmoIndex < ControlNames.Num(); GizmoIndex++)
	{
		UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();

		UStaticMeshComponent* Component = Components[GizmoIndex];
		const FTLLRN_RigElementKey ControlKey(ControlNames[GizmoIndex], ETLLRN_RigElementType::Control);
		const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(ControlKey);
		const bool bVisible = ControlElement && ControlElement->Settings.IsVisible();

		if (Component && Component->IsVisible() != bVisible)
		{
			Component->SetVisibility(bVisible);
		}
		if (Component == nullptr || ControlElement == nullptr)
		{
			continue;
		}

		FTransform ControlTransform = TLLRN_ControlRig->GetControlGlobalTransform(ControlNames[GizmoIndex]);
		Component->SetRelativeTransform(ShapeTransforms[GizmoIndex] * ControlTransform);
		Materials[GizmoIndex]->SetVectorParameterValue(ColorParameterName, FVector(ControlElement->Settings.ShapeColor));
	}
}


#undef LOCTEXT_NAMESPACE

