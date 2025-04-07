// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/STLLRN_ControlRigAnimAttributeView.h"
#include "Editor/TLLRN_ControlRigEditor.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "Engine/Console.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "SAnimAttributeView.h"

#define LOCTEXT_NAMESPACE "STLLRN_ControlRigAnimAttributeView"

static const FName IncomingSnapshotName(TEXT("Incoming Anim Attributes"));
static const FName OutgoingSnapshotName(TEXT("Outgoing Anim Attributes"));
static const TArray<FName> SnapshotNames = {IncomingSnapshotName, OutgoingSnapshotName};

STLLRN_ControlRigAnimAttributeView::~STLLRN_ControlRigAnimAttributeView()
{
	StopObservingCurrentTLLRN_ControlRig();
	
	TLLRN_ControlRigBlueprint->OnSetObjectBeingDebugged().RemoveAll(this);
}

void STLLRN_ControlRigAnimAttributeView::Construct(const FArguments& InArgs, TSharedRef<FTLLRN_ControlRigEditor> InTLLRN_ControlRigEditor)
{
	SAssignNew(AttributeView, SAnimAttributeView);
	
	ChildSlot
	[
		AttributeView.ToSharedRef()
	];

	TLLRN_ControlRigEditor = InTLLRN_ControlRigEditor;
	TLLRN_ControlRigBlueprint = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRigBlueprint();
	TLLRN_ControlRigBlueprint->OnSetObjectBeingDebugged().AddRaw(this, &STLLRN_ControlRigAnimAttributeView::HandleSetObjectBeingDebugged);

	if (UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(TLLRN_ControlRigBlueprint->GetObjectBeingDebugged()))
	{
		StartObservingNewTLLRN_ControlRig(TLLRN_ControlRig);
	}
}

void STLLRN_ControlRigAnimAttributeView::HandleSetObjectBeingDebugged(UObject* InObject)
{
	if(TLLRN_ControlRigBeingDebuggedPtr.Get() == InObject)
	{
		return;
	}

	StopObservingCurrentTLLRN_ControlRig();
	
	if(UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(InObject))
	{
		StartObservingNewTLLRN_ControlRig(TLLRN_ControlRig);
	}
}


void STLLRN_ControlRigAnimAttributeView::StartObservingNewTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig) 
{
	if (InTLLRN_ControlRig)
	{
		InTLLRN_ControlRig->OnPostForwardsSolve_AnyThread().RemoveAll(this);
		InTLLRN_ControlRig->OnPostForwardsSolve_AnyThread().AddRaw(this, &STLLRN_ControlRigAnimAttributeView::HandleTLLRN_ControlRigPostForwardSolve);
		InTLLRN_ControlRig->SetEnableAnimAttributeTrace(true);
	}
	
	TLLRN_ControlRigBeingDebuggedPtr = InTLLRN_ControlRig;
}

void STLLRN_ControlRigAnimAttributeView::StopObservingCurrentTLLRN_ControlRig()
{
	if(TLLRN_ControlRigBeingDebuggedPtr.IsValid())
	{
		if(UTLLRN_ControlRig* TLLRN_ControlRig = TLLRN_ControlRigBeingDebuggedPtr.Get())
		{
			if(!URigVMHost::IsGarbageOrDestroyed(TLLRN_ControlRig))
			{
				TLLRN_ControlRig->OnPostForwardsSolve_AnyThread().RemoveAll(this);
				TLLRN_ControlRig->SetEnableAnimAttributeTrace(false);
			}
		}
	}
	
	TLLRN_ControlRigBeingDebuggedPtr.Reset();
}

void STLLRN_ControlRigAnimAttributeView::HandleTLLRN_ControlRigPostForwardSolve(
	UTLLRN_ControlRig* InTLLRN_ControlRig,
	const FName& InEventName) const
{
	if (IsValid(InTLLRN_ControlRig) && ensure(InTLLRN_ControlRig == TLLRN_ControlRigBeingDebuggedPtr))
	{
		if (InEventName == FTLLRN_RigUnit_BeginExecution::EventName)
		{
			if (const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(InTLLRN_ControlRig->OuterSceneComponent.Get()))
			{
				const UE::Anim::FHeapAttributeContainer& InputSnapshot = InTLLRN_ControlRig->InputAnimAttributeSnapshot;
				const UE::Anim::FHeapAttributeContainer& OutputSnapshot = InTLLRN_ControlRig->OutputAnimAttributeSnapshot;

				const TArray<TTuple<FName, const UE::Anim::FHeapAttributeContainer&>> Snapshots =
				{
					{TEXT("Input") , InputSnapshot  },
					{TEXT("Output"), OutputSnapshot }
				};
				
				AttributeView->DisplayNewAttributeContainerSnapshots(Snapshots, SkeletalMeshComponent);
				return;
			}
		}
	}

	AttributeView->ClearListView();
}

#undef LOCTEXT_NAMESPACE
