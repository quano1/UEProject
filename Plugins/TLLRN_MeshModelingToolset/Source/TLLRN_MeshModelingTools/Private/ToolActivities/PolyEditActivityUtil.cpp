// Copyright Epic Games, Inc. All Rights Reserved.

#include "ToolActivities/PolyEditActivityUtil.h"

#include "Drawing/TLLRN_PolyEditTLLRN_PreviewMesh.h"
#include "InteractiveTool.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TLLRN_MeshOpPreviewHelpers.h"
#include "ToolActivities/TLLRN_PolyEditActivityContext.h"
#include "ToolSetupUtil.h"

using namespace UE::Geometry;

UTLLRN_PolyEditTLLRN_PreviewMesh* PolyEditActivityUtil::CreateTLLRN_PolyEditTLLRN_PreviewMesh(UInteractiveTool& Tool, const UTLLRN_PolyEditActivityContext& ActivityContext)
{
	UTLLRN_PolyEditTLLRN_PreviewMesh* EditPreview = NewObject<UTLLRN_PolyEditTLLRN_PreviewMesh>(&Tool);
	EditPreview->CreateInWorld(Tool.GetWorld(), FTransform::Identity);
	ToolSetupUtil::ApplyRenderingConfigurationToPreview(EditPreview, nullptr); 
	UpdatePolyEditPreviewMaterials(Tool, ActivityContext, *EditPreview, EPreviewMaterialType::PreviewMaterial);
	EditPreview->EnableWireframe(true);

	return EditPreview;
}

void PolyEditActivityUtil::UpdatePolyEditPreviewMaterials(UInteractiveTool& Tool, const UTLLRN_PolyEditActivityContext& ActivityContext,
	UTLLRN_PolyEditTLLRN_PreviewMesh& EditPreview, EPreviewMaterialType MaterialType)
{
	if (MaterialType == EPreviewMaterialType::SourceMaterials)
	{
		TArray<UMaterialInterface*> Materials;
		ActivityContext.Preview->TLLRN_PreviewMesh->GetMaterials(Materials);

		EditPreview.ClearOverrideRenderMaterial();
		EditPreview.SetMaterials(Materials);
	}
	else if (MaterialType == EPreviewMaterialType::PreviewMaterial)
	{
		EditPreview.ClearOverrideRenderMaterial();
		EditPreview.SetMaterial(
			ToolSetupUtil::GetSelectionMaterial(FLinearColor(0.8f, 0.75f, 0.0f), Tool.GetToolManager()));
	}
	else if (MaterialType == EPreviewMaterialType::UVMaterial)
	{
		UMaterial* CheckerMaterialBase = LoadObject<UMaterial>(nullptr, TEXT("/TLLRN_MeshModelingToolsetExp/Materials/CheckerMaterial"));
		if (CheckerMaterialBase != nullptr)
		{
			UMaterialInstanceDynamic* CheckerMaterial = UMaterialInstanceDynamic::Create(CheckerMaterialBase, NULL);
			CheckerMaterial->SetScalarParameterValue("Density", 1);
			EditPreview.SetOverrideRenderMaterial(CheckerMaterial);
		}
	}
}
