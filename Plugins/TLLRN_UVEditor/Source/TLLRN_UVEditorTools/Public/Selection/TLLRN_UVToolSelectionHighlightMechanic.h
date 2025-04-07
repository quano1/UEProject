// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GeometryBase.h"
#include "InteractionMechanic.h"

#include "TLLRN_UVToolSelectionHighlightMechanic.generated.h"

PREDECLARE_GEOMETRY(class FDynamicMesh3);
PREDECLARE_GEOMETRY(class FUVToolSelection);

class APreviewGeometryActor;
class UMaterialInstanceDynamic;
class UTLLRN_Basic2DPointSetComponent;
class UTLLRN_Basic3DPointSetComponent;
class UTLLRN_Basic2DLineSetComponent;
class UTLLRN_Basic3DLineSetComponent;
class UTLLRN_Basic2DTriangleSetComponent;
class UTLLRN_UVEditorToolMeshInput;
class UWorld;

/**
 * Mechanic for highlighting mesh elements (usually selection) in the UV editor.
 */
UCLASS()
class TLLRN_UVEDITORTOOLS_API UTLLRN_UVToolSelectionHighlightMechanic : public UInteractionMechanic
{
	GENERATED_BODY()

	using FUVToolSelection = UE::Geometry::FUVToolSelection;

public:

	void Initialize(UWorld* UnwrapWorld, UWorld* LivePreviewWorld);
	void Shutdown() override;

	void SetIsVisible(bool bUnwrapHighlightVisible, bool bLivePreviewHighlightVisible);

	/**
	 * Rebuilds the unwrap mesh highlights.
	 * 
	 * @param StartTransform The transform to consider as the start transform, so that the highlights
	 *   can be repositioned easily later via SetUnwrapHighlightTransform().
	 * @param bUsePreviews If true, the selected element positions will be gotten from the UnwrapPreview
	 *   of each selection target rather than UnwrapCanonical.
	 */
	void RebuildUnwrapHighlight(const TArray<FUVToolSelection>& Selections, 
		const FTransform& StartTransform, bool bUsePreviews = false);

	/**
	 * Set the transform of the unwrap mesh highlights without rebuilding them, likely to 
	 * follow the translation of the underlying elements via gizmo.
	 * 
	 * @param bRebuildStaticPairedEdges If true, and paired edge highlighting is enabled,
	 *   unselected paired edges are rebuilt in the highlight. This is usually necessary
	 *   because such edges may be attached to moving selected edges.
	 * @param bUsePreviews If bRebuildStaticPairedEdges is true, this will prefer to use the
	 *   previews rather than the unwrap canonical.
	 */
	void SetUnwrapHighlightTransform(const FTransform& Transform, bool bRebuildStaticPairedEdges = true, 
		bool bUsePreviews = false);

	/**
	 * Get the current transform of the unwrap highlight.
	 */
	FTransform GetUnwrapHighlightTransform();

	/**
	 * Convert the unwrap selections to elements in the applied mesh, then highlight those there.
	 * 
	 * @param bUsePreviews If true, the previews are used rather than the canonicals both for converting
	 *   over to applied mesh elements and to finding the highlight positions.
	 */
	void RebuildAppliedHighlightFromUnwrapSelection(const TArray<FUVToolSelection>& UnwrapSelections, bool bUsePreviews = false);

	/**
	 * Adds additional highlighting elements based on selection information. This is used to extend selection
	 * line sets for the unset selections if they are needed. 
	 *
	 * @param bUsePreviews If true, the previews are used rather than the canonicals for finding the highlight positions.
	 */
	void AppendAppliedHighlight(const TArray<FUVToolSelection>& AppliedSelections, bool bUsePreviews = false);

	/**
	 * When true, edge selections also highlight the edge pairings (i.e. edges that could weld with the
	 * selected edges). Note that the call itself controls visibility and the next rebuild call, but 
	 * won't cause an immediate rebuild on its own.
	 */
	void SetEnablePairedEdgeHighlights(bool bEnable);

	/**
	 * Directly set the Color property of the selection line and point set.
	 */
	void SetColor(const FColor Color) const;

	/**
	 * Directly set the Size/Thickness property of the selection line set.
	 */
	void SetLineThickness(const float LineSize) const;

	/**
	 * directly set the Size property of the selection point set.
	 */
	void SetPointSize(const float PointSize) const;

protected:

	UPROPERTY()
	TObjectPtr<APreviewGeometryActor> UnwrapGeometryActor = nullptr;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> TriangleSetMaterial = nullptr;

	TWeakObjectPtr<UTLLRN_Basic2DTriangleSetComponent> UnwrapTriangleSet = nullptr;
	TWeakObjectPtr<UTLLRN_Basic2DLineSetComponent> UnwrapLineSet = nullptr;
	TWeakObjectPtr<UTLLRN_Basic2DLineSetComponent> SewEdgePairingLeftLineSet = nullptr;
	TWeakObjectPtr<UTLLRN_Basic2DLineSetComponent> SewEdgePairingRightLineSet = nullptr;
	TWeakObjectPtr<UTLLRN_Basic2DPointSetComponent> UnwrapPointSet = nullptr;
	TWeakObjectPtr<UTLLRN_Basic2DLineSetComponent> UnwrapPairedEdgeLineSet = nullptr;

	UPROPERTY()
	TObjectPtr<APreviewGeometryActor> UnwrapStationaryGeometryActor = nullptr;
	TWeakObjectPtr<UTLLRN_Basic2DLineSetComponent> SewEdgeUnselectedPairingLineSet = nullptr;

	// Per mesh, gives a list of endpoint vid pairs for the unselected edges that
	// are part of edge pairs, for rebuilding when necessary.
	TArray<TPair<TWeakObjectPtr<UTLLRN_UVEditorToolMeshInput>,
		TArray<TPair<int32, int32>>>> StaticPairedEdgeVidsPerMesh;

	UPROPERTY()
	TObjectPtr<APreviewGeometryActor> LivePreviewGeometryActor = nullptr;
	TWeakObjectPtr<UTLLRN_Basic3DLineSetComponent> LivePreviewLineSet = nullptr;
	TWeakObjectPtr<UTLLRN_Basic3DPointSetComponent> LivePreviewPointSet = nullptr;

	bool bPairedEdgeHighlightsEnabled = true;
};
