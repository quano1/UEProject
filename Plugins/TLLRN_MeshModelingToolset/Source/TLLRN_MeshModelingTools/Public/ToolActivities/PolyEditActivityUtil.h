// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UInteractiveTool;
class UTLLRN_PolyEditActivityContext;
class UTLLRN_PolyEditTLLRN_PreviewMesh;

namespace UE {
namespace Geometry {
namespace PolyEditActivityUtil {

	enum class EPreviewMaterialType
	{
		SourceMaterials, PreviewMaterial, UVMaterial
	};

	UTLLRN_PolyEditTLLRN_PreviewMesh* CreateTLLRN_PolyEditTLLRN_PreviewMesh(UInteractiveTool& Tool, const UTLLRN_PolyEditActivityContext& ActivityContext);

	void UpdatePolyEditPreviewMaterials(UInteractiveTool& Tool, const UTLLRN_PolyEditActivityContext& ActivityContext,
		UTLLRN_PolyEditTLLRN_PreviewMesh& EditTLLRN_PreviewMesh, EPreviewMaterialType MaterialType);

}}}