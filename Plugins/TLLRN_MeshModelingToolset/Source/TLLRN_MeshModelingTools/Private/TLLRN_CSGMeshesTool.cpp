// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_CSGMeshesTool.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "ToolSetupUtil.h"
#include "ModelingToolTargetUtil.h"
#include "BaseGizmos/TransformProxy.h"
#include "CompositionOps/TLLRN_BooleanMeshesOp.h"
#include "DynamicMesh/DynamicMesh3.h"

#include "TargetInterfaces/MeshDescriptionProvider.h"
#include "TargetInterfaces/PrimitiveComponentBackedTarget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_CSGMeshesTool)

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UTLLRN_CSGMeshesTool"


void UTLLRN_CSGMeshesTool::EnableTrimMode()
{
	check(OriginalDynamicMeshes.Num() == 0);		// must not have been initialized!
	bTrimMode = true;
}

void UTLLRN_CSGMeshesTool::SetupProperties()
{
	Super::SetupProperties();

	if (bTrimMode)
	{
		TrimProperties = NewObject<UTLLRN_TrimMeshesToolProperties>(this);
		TrimProperties->RestoreProperties(this);
		AddToolPropertySource(TrimProperties);

		TrimProperties->WatchProperty(TrimProperties->WhichMesh, [this](ETLLRN_TrimOperation)
		{
			UpdateGizmoVisibility();
			UpdatePreviewsVisibility();
		});
		TrimProperties->WatchProperty(TrimProperties->bShowTrimmingMesh, [this](bool)
		{
			UpdatePreviewsVisibility();
		});
		TrimProperties->WatchProperty(TrimProperties->ColorOfTrimmingMesh, [this](FLinearColor)
		{
			UpdatePreviewsMaterial();
		});
		TrimProperties->WatchProperty(TrimProperties->OpacityOfTrimmingMesh, [this](float)
		{
			UpdatePreviewsMaterial();
		});

		SetToolDisplayName(LOCTEXT("TrimMeshesToolName", "Trim"));
		GetToolManager()->DisplayMessage(
			LOCTEXT("OnStartTrimTool", "Trim one mesh with another. Use the transform gizmos to tweak the positions of the input objects (can help to resolve errors/failures)"),
			EToolMessageLevel::UserNotification);
	}
	else
	{
		CSGProperties = NewObject<UTLLRN_CSGMeshesToolProperties>(this);
		CSGProperties->RestoreProperties(this);
		AddToolPropertySource(CSGProperties);

		CSGProperties->WatchProperty(CSGProperties->Operation, [this](ETLLRN_CSGOperation)
		{
			UpdateGizmoVisibility();
			UpdatePreviewsVisibility();
		});
		CSGProperties->WatchProperty(CSGProperties->bShowSubtractedMesh, [this](bool)
		{
			UpdatePreviewsVisibility();
		});
		CSGProperties->WatchProperty(CSGProperties->SubtractedMeshColor, [this](FLinearColor)
		{
			UpdatePreviewsMaterial();
		});
		CSGProperties->WatchProperty(CSGProperties->SubtractedMeshOpacity, [this](float)
		{
			UpdatePreviewsMaterial();
		});

		SetToolDisplayName(LOCTEXT("TLLRN_CSGMeshesToolName", "Mesh Boolean"));
		GetToolManager()->DisplayMessage(
			LOCTEXT("OnStartTool", "Perform Boolean operations on the input meshes; any interior faces will be removed. Use the transform gizmos to modify the position and orientation of the input objects."),
			EToolMessageLevel::UserNotification);
	}
}

void UTLLRN_CSGMeshesTool::UpdatePreviewsMaterial()
{
	if (!PreviewsGhostMaterial)
	{
		PreviewsGhostMaterial = ToolSetupUtil::GetSimpleCustomMaterial(GetToolManager(), FLinearColor::Black, .2);
	}
	FLinearColor Color;
	float Opacity;
	if (bTrimMode)
	{
		Color = TrimProperties->ColorOfTrimmingMesh;
		Opacity = TrimProperties->OpacityOfTrimmingMesh;
	}
	else
	{
		Color = CSGProperties->SubtractedMeshColor;
		Opacity = CSGProperties->SubtractedMeshOpacity;
	}
	PreviewsGhostMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	PreviewsGhostMaterial->SetScalarParameterValue(TEXT("Opacity"), Opacity);
}

void UTLLRN_CSGMeshesTool::UpdatePreviewsVisibility()
{
	int32 ShowPreviewIdx = -1;
	if (bTrimMode && TrimProperties->bShowTrimmingMesh)
	{
		ShowPreviewIdx = TrimProperties->WhichMesh == ETLLRN_TrimOperation::TrimA ? OriginalMeshPreviews.Num() - 1 : 0;
	}
	else if (!bTrimMode && CSGProperties->bShowSubtractedMesh)
	{
		if (CSGProperties->Operation == ETLLRN_CSGOperation::DifferenceAB)
		{
			ShowPreviewIdx = OriginalMeshPreviews.Num() - 1;
		}
		else if (CSGProperties->Operation == ETLLRN_CSGOperation::DifferenceBA)
		{
			ShowPreviewIdx = 0;
		}
	}
	for (int32 MeshIdx = 0; MeshIdx < OriginalMeshPreviews.Num(); MeshIdx++)
	{
		OriginalMeshPreviews[MeshIdx]->SetVisible(ShowPreviewIdx == MeshIdx);
	}
}

int32 UTLLRN_CSGMeshesTool::GetHiddenGizmoIndex() const
{
	int32 ParentHiddenIndex = Super::GetHiddenGizmoIndex();
	if (ParentHiddenIndex != -1)
	{
		return ParentHiddenIndex;
	}
	if (bTrimMode)
	{
		return TrimProperties->WhichMesh == ETLLRN_TrimOperation::TrimA ? 0 : 1;
	}
	else if (CSGProperties->Operation == ETLLRN_CSGOperation::DifferenceAB)
	{
		return 0;
	}
	else if (CSGProperties->Operation == ETLLRN_CSGOperation::DifferenceBA)
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

void UTLLRN_CSGMeshesTool::SaveProperties()
{
	Super::SaveProperties();
	if (bTrimMode)
	{
		TrimProperties->SaveProperties(this);
	}
	else
	{
		CSGProperties->SaveProperties(this);
	}
}


void UTLLRN_CSGMeshesTool::ConvertInputsAndSetPreviewMaterials(bool bSetTLLRN_PreviewMesh)
{
	OriginalDynamicMeshes.SetNum(Targets.Num());
	FComponentMaterialSet AllMaterialSet;
	TMap<UMaterialInterface*, int> KnownMaterials;
	TArray<TArray<int>> MaterialRemap; MaterialRemap.SetNum(Targets.Num());

	if (bTrimMode || !CSGProperties->bUseFirstMeshMaterials)
	{
		for (int ComponentIdx = 0; ComponentIdx < Targets.Num(); ComponentIdx++)
		{
			const FComponentMaterialSet ComponentMaterialSet = UE::ToolTarget::GetMaterialSet(Targets[ComponentIdx]);
			for (UMaterialInterface* Mat : ComponentMaterialSet.Materials)
			{
				int* FoundMatIdx = KnownMaterials.Find(Mat);
				int MatIdx;
				if (FoundMatIdx)
				{
					MatIdx = *FoundMatIdx;
				}
				else
				{
					MatIdx = AllMaterialSet.Materials.Add(Mat);
					KnownMaterials.Add(Mat, MatIdx);
				}
				MaterialRemap[ComponentIdx].Add(MatIdx);
			}
		}
	}
	else
	{
		AllMaterialSet = UE::ToolTarget::GetMaterialSet(Targets[0]);
		for (int MatIdx = 0; MatIdx < AllMaterialSet.Materials.Num(); MatIdx++)
		{
			MaterialRemap[0].Add(MatIdx);
		}
		for (int ComponentIdx = 1; ComponentIdx < Targets.Num(); ComponentIdx++)
		{
			MaterialRemap[ComponentIdx].Init(0, Cast<IMaterialProvider>(Targets[ComponentIdx])->GetNumMaterials());
		}
	}

	UpdatePreviewsMaterial();
	for (int ComponentIdx = 0; ComponentIdx < Targets.Num(); ComponentIdx++)
	{
		OriginalDynamicMeshes[ComponentIdx] = MakeShared<FDynamicMesh3, ESPMode::ThreadSafe>();
		*OriginalDynamicMeshes[ComponentIdx] = UE::ToolTarget::GetDynamicMeshCopy(Targets[ComponentIdx]);

		// ensure materials and attributes are always enabled
		OriginalDynamicMeshes[ComponentIdx]->EnableAttributes();
		OriginalDynamicMeshes[ComponentIdx]->Attributes()->EnableMaterialID();
		FDynamicMeshMaterialAttribute* MaterialIDs = OriginalDynamicMeshes[ComponentIdx]->Attributes()->GetMaterialID();
		for (int TID : OriginalDynamicMeshes[ComponentIdx]->TriangleIndicesItr())
		{
			MaterialIDs->SetValue(TID, MaterialRemap[ComponentIdx][MaterialIDs->GetValue(TID)]);
		}

		if (bSetTLLRN_PreviewMesh)
		{
			UTLLRN_PreviewMesh* OriginalMeshPreview = OriginalMeshPreviews.Add_GetRef(NewObject<UTLLRN_PreviewMesh>());
			OriginalMeshPreview->CreateInWorld(GetTargetWorld(), (FTransform) UE::ToolTarget::GetLocalToWorldTransform(Targets[ComponentIdx]));
			OriginalMeshPreview->UpdatePreview(OriginalDynamicMeshes[ComponentIdx].Get());

			OriginalMeshPreview->SetMaterial(0, PreviewsGhostMaterial);
			OriginalMeshPreview->SetVisible(false);
			TransformProxies[ComponentIdx]->AddComponent(OriginalMeshPreview->GetRootComponent());
		}
	}
	Preview->ConfigureMaterials(AllMaterialSet.Materials, ToolSetupUtil::GetDefaultWorkingMaterial(GetToolManager()));
}


void UTLLRN_CSGMeshesTool::SetPreviewCallbacks()
{	
	DrawnLineSet = NewObject<UTLLRN_LineSetComponent>(Preview->TLLRN_PreviewMesh->GetRootComponent());
	DrawnLineSet->SetupAttachment(Preview->TLLRN_PreviewMesh->GetRootComponent());
	DrawnLineSet->SetLineMaterial(ToolSetupUtil::GetDefaultLineComponentMaterial(GetToolManager()));
	DrawnLineSet->RegisterComponent();

	Preview->OnOpCompleted.AddLambda(
		[this](const FDynamicMeshOperator* Op)
		{
			const FTLLRN_BooleanMeshesOp* BooleanOp = (const FTLLRN_BooleanMeshesOp*)(Op);
			CreatedBoundaryEdges = BooleanOp->GetCreatedBoundaryEdges();
		}
	);
	Preview->OnMeshUpdated.AddLambda(
		[this](const UTLLRN_MeshOpPreviewWithBackgroundCompute*)
		{
			GetToolManager()->PostInvalidation();
			UpdateVisualization();
		}
	);
}


void UTLLRN_CSGMeshesTool::UpdateVisualization()
{
	FColor BoundaryEdgeColor(240, 15, 15);
	float BoundaryEdgeThickness = 2.0;
	float BoundaryEdgeDepthBias = 2.0f;

	const FDynamicMesh3* TargetMesh = Preview->TLLRN_PreviewMesh->GetPreviewDynamicMesh();
	FVector3d A, B;

	DrawnLineSet->Clear();
	if (!bTrimMode && CSGProperties->bShowNewBoundaries)
	{
		for (int EID : CreatedBoundaryEdges)
		{
			TargetMesh->GetEdgeV(EID, A, B);
			DrawnLineSet->AddLine((FVector)A, (FVector)B, BoundaryEdgeColor, BoundaryEdgeThickness, BoundaryEdgeDepthBias);
		}
	}
}


TUniquePtr<FDynamicMeshOperator> UTLLRN_CSGMeshesTool::MakeNewOperator()
{
	TUniquePtr<FTLLRN_BooleanMeshesOp> BooleanOp = MakeUnique<FTLLRN_BooleanMeshesOp>();
	
	BooleanOp->bTrimMode = bTrimMode;
	if (bTrimMode)
	{
		BooleanOp->WindingThreshold = TrimProperties->WindingThreshold;
		BooleanOp->TLLRN_TrimOperation = TrimProperties->WhichMesh;
		BooleanOp->TLLRN_TrimSide = TrimProperties->TLLRN_TrimSide;
		BooleanOp->bAttemptFixHoles = false;
		BooleanOp->bTryCollapseExtraEdges = false;
	}
	else
	{
		BooleanOp->WindingThreshold = CSGProperties->WindingThreshold;
		BooleanOp->TLLRN_CSGOperation = CSGProperties->Operation;
		BooleanOp->bAttemptFixHoles = CSGProperties->bTryFixHoles;
		BooleanOp->bTryCollapseExtraEdges = CSGProperties->bTryCollapseEdges;
	}

	check(OriginalDynamicMeshes.Num() == 2);
	check(Targets.Num() == 2);
	BooleanOp->Transforms.SetNum(2);
	BooleanOp->Meshes.SetNum(2);
	for (int Idx = 0; Idx < 2; Idx++)
	{
		BooleanOp->Meshes[Idx] = OriginalDynamicMeshes[Idx];
		BooleanOp->Transforms[Idx] = TransformProxies[Idx]->GetTransform();
	}

	return BooleanOp;
}



void UTLLRN_CSGMeshesTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	if (Property && (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_CSGMeshesToolProperties, bUseFirstMeshMaterials)))
	{
		if (!AreAllTargetsValid())
		{
			GetToolManager()->DisplayMessage(LOCTEXT("InvalidTargets", "Target meshes are no longer valid"), EToolMessageLevel::UserWarning);
			return;
		}
		ConvertInputsAndSetPreviewMaterials(false);
		Preview->InvalidateResult();
	}
	else if (Property && (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_CSGMeshesToolProperties, bShowNewBoundaries)))
	{
		GetToolManager()->PostInvalidation();
		UpdateVisualization();
	}
	else
	{
		//TODO: UTLLRN_BaseCreateFromSelectedTool::OnPropertyModified below calls Preview->InvalidateResult() which triggers
		//expensive recompute of the boolean operator. We should rethink which code is responsible for invalidating the 
		//preview and make it consistent since some of the code calls Preview->InvalidateResult() manually.
		bool bVisualUpdate = false;
		if (bTrimMode)
		{
			bVisualUpdate = Property && (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_TrimMeshesToolProperties, bShowTrimmingMesh) ||
										 Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_TrimMeshesToolProperties, OpacityOfTrimmingMesh) ||
										 Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_TrimMeshesToolProperties, ColorOfTrimmingMesh) ||
										 Property->GetFName() == FName("R") ||
										 Property->GetFName() == FName("G") ||
										 Property->GetFName() == FName("B") ||
										 Property->GetFName() == FName("A"));
		}
		else 
		{	
			bVisualUpdate = Property && (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_CSGMeshesToolProperties, bShowSubtractedMesh) ||
										 Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_CSGMeshesToolProperties, SubtractedMeshColor) ||
										 Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_CSGMeshesToolProperties, SubtractedMeshOpacity) ||
										 Property->GetFName() == FName("R") ||
										 Property->GetFName() == FName("G") ||
										 Property->GetFName() == FName("B") ||
										 Property->GetFName() == FName("A"));
		}

		// If the property that was changed only affects the visuals, we do not need to recalculate the output 
		// of FTLLRN_BooleanMeshesOp 
		if (bVisualUpdate == false)
		{
			Super::OnPropertyModified(PropertySet, Property);
		}
	}
}


FString UTLLRN_CSGMeshesTool::GetCreatedAssetName() const
{
	if (bTrimMode)
	{
		return TEXT("Trim");
	}
	else
	{
		return TEXT("Boolean");
	}
}


FText UTLLRN_CSGMeshesTool::GetActionName() const
{
	if (bTrimMode)
	{
		return LOCTEXT("TrimActionName", "Trim Meshes");
	}
	else
	{
		return LOCTEXT("BooleanActionName", "Boolean Meshes");
	}
}



void UTLLRN_CSGMeshesTool::OnShutdown(EToolShutdownType ShutdownType)
{
	Super::OnShutdown(ShutdownType);

	for (UTLLRN_PreviewMesh* MeshPreview : OriginalMeshPreviews)
	{
		MeshPreview->SetVisible(false);
		MeshPreview->Disconnect();
		MeshPreview = nullptr;
	}
}



#undef LOCTEXT_NAMESPACE

