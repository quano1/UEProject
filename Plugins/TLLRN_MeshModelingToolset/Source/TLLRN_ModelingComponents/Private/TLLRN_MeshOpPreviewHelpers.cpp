// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_MeshOpPreviewHelpers.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_MeshOpPreviewHelpers)


#define LOCTEXT_NAMESPACE "TLLRN_MeshOpPreviewHelpers"

namespace UE::Private::MeshOpPreviewLocal
{
	static void DisplayCriticalWarningMessage(const FText& InMessage, float ExpireDuration = 5.0f)
	{
#if WITH_EDITOR
		FNotificationInfo Info(InMessage);
		Info.ExpireDuration = ExpireDuration;
		FSlateNotificationManager::Get().AddNotification(Info);
#endif

		UE_LOG(LogGeometry, Warning, TEXT("%s"), *InMessage.ToString());
	}

	static TAutoConsoleVariable<int32> CVarOverrideMaxBackgroundTasks(
		TEXT("modeling.MaxBackgroundTasksOverride"), 0,
		TEXT("Optional override for maximum allowed background tasks when generating preview results in tools. 0 to use default values. [def: 0]"));

	int32 MaxActiveBackgroundTasksWithOverride(int32 MaxWithoutOverride)
	{
		int32 Override = CVarOverrideMaxBackgroundTasks.GetValueOnAnyThread();
		return Override > 0 ? Override : MaxWithoutOverride;
	}
}


using namespace UE::Geometry;

void UTLLRN_MeshOpPreviewWithBackgroundCompute::Setup(UWorld* InWorld)
{
	TLLRN_PreviewMesh = NewObject<UTLLRN_PreviewMesh>(this, TEXT("TLLRN_PreviewMesh"));
	TLLRN_PreviewMesh->CreateInWorld(InWorld, FTransform::Identity);
	PreviewWorld = InWorld;
	bResultValid = false;
	bMeshInitialized = false;
}

void UTLLRN_MeshOpPreviewWithBackgroundCompute::Setup(UWorld* InWorld, IDynamicMeshOperatorFactory* OpGenerator)
{
	Setup(InWorld);
	BackgroundCompute = MakeUnique<FBackgroundDynamicMeshComputeSource>(OpGenerator);
	BackgroundCompute->MaxActiveTaskCount = UE::Private::MeshOpPreviewLocal::MaxActiveBackgroundTasksWithOverride(MaxActiveBackgroundTasks);
}

void UTLLRN_MeshOpPreviewWithBackgroundCompute::ChangeOpFactory(IDynamicMeshOperatorFactory* OpGenerator)
{
	CancelCompute();
	BackgroundCompute = MakeUnique<FBackgroundDynamicMeshComputeSource>(OpGenerator);
	BackgroundCompute->MaxActiveTaskCount = UE::Private::MeshOpPreviewLocal::MaxActiveBackgroundTasksWithOverride(MaxActiveBackgroundTasks);
	bResultValid = false;
	bMeshInitialized = false;
}

void UTLLRN_MeshOpPreviewWithBackgroundCompute::ClearOpFactory()
{
	CancelCompute();
	BackgroundCompute = nullptr;
	bResultValid = false;
	bMeshInitialized = false;
}


FDynamicMeshOpResult UTLLRN_MeshOpPreviewWithBackgroundCompute::Shutdown()
{
	CancelCompute();

	FDynamicMeshOpResult Result;
	Result.Mesh = TLLRN_PreviewMesh->ExtractTLLRN_PreviewMesh();
	Result.Transform = FTransformSRT3d(TLLRN_PreviewMesh->GetTransform());

	TLLRN_PreviewMesh->SetVisible(false);
	TLLRN_PreviewMesh->Disconnect();
	TLLRN_PreviewMesh = nullptr;

	PreviewWorld = nullptr;

	return Result;
}


void UTLLRN_MeshOpPreviewWithBackgroundCompute::CancelCompute()
{
	if (BackgroundCompute)
	{
		BackgroundCompute->CancelActiveCompute();
	}
}

void UTLLRN_MeshOpPreviewWithBackgroundCompute::Cancel()
{
	CancelCompute();

	TLLRN_PreviewMesh->SetVisible(false);
	TLLRN_PreviewMesh->Disconnect();
	TLLRN_PreviewMesh = nullptr;
}


void UTLLRN_MeshOpPreviewWithBackgroundCompute::Tick(float DeltaTime)
{
	if (BackgroundCompute)
	{
		BackgroundCompute->Tick(DeltaTime);
		UpdateResults();
	}

	if (!IsUsingWorkingMaterial())
	{
		if (OverrideMaterial != nullptr)
		{
			TLLRN_PreviewMesh->SetOverrideRenderMaterial(OverrideMaterial);
		}
		else
		{
			TLLRN_PreviewMesh->ClearOverrideRenderMaterial();
		}

		if (SecondaryMaterial != nullptr)
		{
			TLLRN_PreviewMesh->SetSecondaryRenderMaterial(SecondaryMaterial);
		}
		else
		{
			TLLRN_PreviewMesh->ClearSecondaryRenderMaterial();
		}
	}
	else
	{
		TLLRN_PreviewMesh->SetOverrideRenderMaterial(WorkingMaterial);
		TLLRN_PreviewMesh->ClearSecondaryRenderMaterial();
	}
}


void UTLLRN_MeshOpPreviewWithBackgroundCompute::SetMaxActiveBackgroundTasks(int32 InMaxActiveBackgroundTasks)
{
	MaxActiveBackgroundTasks = InMaxActiveBackgroundTasks;
	if (BackgroundCompute)
	{
		BackgroundCompute->MaxActiveTaskCount = UE::Private::MeshOpPreviewLocal::MaxActiveBackgroundTasksWithOverride(MaxActiveBackgroundTasks);
	}
}


void UTLLRN_MeshOpPreviewWithBackgroundCompute::UpdateResults()
{
	if (BackgroundCompute == nullptr)
	{
		LastComputeStatus = EBackgroundComputeTaskStatus::NotComputing;
		return;
	}

	FBackgroundDynamicMeshComputeSource::FStatus Status = BackgroundCompute->CheckStatus();	
	LastComputeStatus = Status.TaskStatus; 

	if (LastComputeStatus == EBackgroundComputeTaskStatus::ValidResultAvailable
		|| (bAllowDirtyResultUpdates && LastComputeStatus == EBackgroundComputeTaskStatus::DirtyResultAvailable))
	{
		TUniquePtr<FDynamicMeshOperator> MeshOp = BackgroundCompute->ExtractResult();
		OnOpCompleted.Broadcast(MeshOp.Get());

		TUniquePtr<FDynamicMesh3> ResultMesh = MeshOp->ExtractResult();
		TLLRN_PreviewMesh->SetTransform((FTransform)MeshOp->GetResultTransform());

		UTLLRN_PreviewMesh::ERenderUpdateMode UpdateType = (bMeshTopologyIsConstant && bMeshInitialized) ?
			UTLLRN_PreviewMesh::ERenderUpdateMode::FastUpdate
			: UTLLRN_PreviewMesh::ERenderUpdateMode::FullUpdate;
		
		TLLRN_PreviewMesh->UpdatePreview(MoveTemp(*ResultMesh), UpdateType, ChangingAttributeFlags);
		bMeshInitialized = true;

		TLLRN_PreviewMesh->SetVisible(bVisible);
		bResultValid = (LastComputeStatus == EBackgroundComputeTaskStatus::ValidResultAvailable);
		ValidResultComputeTimeSeconds = Status.ElapsedTime;

		OnMeshUpdated.Broadcast(this);

		bWaitingForBackgroundTasks = false;
	}
	else if (int WaitingTaskCount; BackgroundCompute->IsWaitingForBackgroundTasks(WaitingTaskCount))
	{
		if (!bWaitingForBackgroundTasks)
		{
			UE::Private::MeshOpPreviewLocal::DisplayCriticalWarningMessage(LOCTEXT("TooManyBackgroundTasks", "Too many background tasks: Cancelling earlier tasks before generating new preview."));
			bWaitingForBackgroundTasks = true;
		}
	}
	else
	{
		bWaitingForBackgroundTasks = false;
	}
}


void UTLLRN_MeshOpPreviewWithBackgroundCompute::InvalidateResult()
{
	if (BackgroundCompute)
	{
		BackgroundCompute->NotifyActiveComputeInvalidated();
	}
	bResultValid = false;
}


bool UTLLRN_MeshOpPreviewWithBackgroundCompute::GetCurrentResultCopy(FDynamicMesh3& MeshOut, bool bOnlyIfValid)
{
	if ( HaveValidResult() || bOnlyIfValid == false)
	{
		TLLRN_PreviewMesh->ProcessMesh([&](const FDynamicMesh3& ReadMesh)
		{
			MeshOut = ReadMesh;
		});
		return true;
	}
	return false;
}

bool UTLLRN_MeshOpPreviewWithBackgroundCompute::ProcessCurrentMesh(TFunctionRef<void(const UE::Geometry::FDynamicMesh3&)> ProcessFunc, bool bOnlyIfValid)
{
	if (HaveValidResult() || bOnlyIfValid == false)
	{
		TLLRN_PreviewMesh->ProcessMesh(ProcessFunc);
		return true;
	}
	return false;
}


void UTLLRN_MeshOpPreviewWithBackgroundCompute::ConfigureMaterials(UMaterialInterface* StandardMaterialIn, UMaterialInterface* WorkingMaterialIn)
{
	TArray<UMaterialInterface*> Materials;
	Materials.Add(StandardMaterialIn);
	ConfigureMaterials(Materials, WorkingMaterialIn);
}


void UTLLRN_MeshOpPreviewWithBackgroundCompute::ConfigureMaterials(
	TArray<UMaterialInterface*> StandardMaterialsIn, 
	UMaterialInterface* WorkingMaterialIn,
	UMaterialInterface* SecondaryMaterialIn)
{
	this->StandardMaterials = StandardMaterialsIn;
	this->WorkingMaterial = WorkingMaterialIn;
	this->SecondaryMaterial = SecondaryMaterialIn;

	if (TLLRN_PreviewMesh != nullptr)
	{
		TLLRN_PreviewMesh->SetMaterials(this->StandardMaterials);
	}
}

void UTLLRN_MeshOpPreviewWithBackgroundCompute::ConfigurePreviewMaterials(
	UMaterialInterface* InProgressMaterialIn,
	UMaterialInterface* SecondaryMaterialIn)
{
	this->WorkingMaterial = InProgressMaterialIn;
	this->SecondaryMaterial = SecondaryMaterialIn;
}

void UTLLRN_MeshOpPreviewWithBackgroundCompute::DisablePreviewMaterials()
{
	this->WorkingMaterial = nullptr;
	this->SecondaryMaterial = nullptr;
}

void UTLLRN_MeshOpPreviewWithBackgroundCompute::SetVisibility(bool bVisibleIn)
{
	bVisible = bVisibleIn;
	TLLRN_PreviewMesh->SetVisible(bVisible);
}

void UTLLRN_MeshOpPreviewWithBackgroundCompute::SetIsMeshTopologyConstant(bool bOn, EMeshRenderAttributeFlags ChangingAttributesIn)
{
	bMeshTopologyIsConstant = bOn;
	ChangingAttributeFlags = ChangingAttributesIn;
}


bool UTLLRN_MeshOpPreviewWithBackgroundCompute::IsUsingWorkingMaterial()
{
	return WorkingMaterial && BackgroundCompute 
		&& LastComputeStatus == EBackgroundComputeTaskStatus::InProgress
		&& BackgroundCompute->GetElapsedComputeTime() > SecondsBeforeWorkingMaterial;
}

#undef LOCTEXT_NAMESPACE
