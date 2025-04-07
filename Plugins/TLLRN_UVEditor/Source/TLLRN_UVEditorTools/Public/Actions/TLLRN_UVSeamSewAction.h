// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Actions/TLLRN_TLLRN_UVToolAction.h"
#include "GeometryBase.h"
#include "IndexTypes.h"

#include "Actions/TLLRN_TLLRN_UVToolAction.h"

#include "TLLRN_UVSeamSewAction.generated.h"

PREDECLARE_GEOMETRY(class FTLLRN_UVEditorDynamicMeshSelection);
PREDECLARE_GEOMETRY(class FDynamicMesh3);
class APreviewGeometryActor;
class ULineSetComponent;

UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVSeamSewAction : public UTLLRN_TLLRN_UVToolAction
{	
	GENERATED_BODY()

	using FDynamicMesh3 = UE::Geometry::FDynamicMesh3;

public:
	virtual bool CanExecuteAction() const override;
	virtual bool ExecuteAction() override;

	static int32 FindSewEdgeOppositePairing(const FDynamicMesh3& UnwrapMesh, 
		const FDynamicMesh3& AppliedMesh, int32 UVLayerIndex, int32 UnwrapEid, 
		bool& bWouldPreferOppositeOrderOut);
};