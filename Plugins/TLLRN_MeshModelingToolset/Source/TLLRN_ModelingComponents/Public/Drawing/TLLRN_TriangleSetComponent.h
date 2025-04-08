// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Components/MeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "IndexTypes.h"

#include "TLLRN_TriangleSetComponent.generated.h"

USTRUCT()
struct FTLLRN_RenderableTriangleVertex
{
	GENERATED_BODY()

	FTLLRN_RenderableTriangleVertex()
		: Position(ForceInitToZero)
		, UV(ForceInitToZero)
		, Normal(ForceInitToZero)
		, Color(ForceInitToZero)
	{}

	FTLLRN_RenderableTriangleVertex( const FVector& InPosition, const FVector2D& InUV, const FVector& InNormal, const FColor& InColor )
		: Position( InPosition ),
		  UV( InUV ),
		  Normal( InNormal ),
		  Color( InColor )
	{
	}

	UPROPERTY()
	FVector Position;

	UPROPERTY()
	FVector2D UV;

	UPROPERTY()
	FVector Normal;

	UPROPERTY()
	FColor Color;
};


USTRUCT()
struct FTLLRN_RenderableTriangle
{
	GENERATED_BODY()

	FTLLRN_RenderableTriangle()
		: Material(nullptr)
		, Vertex0()
		, Vertex1()
		, Vertex2()
	{}

	FTLLRN_RenderableTriangle( UMaterialInterface* InMaterial, const FTLLRN_RenderableTriangleVertex& InVertex0, 
		const FTLLRN_RenderableTriangleVertex& InVertex1, const FTLLRN_RenderableTriangleVertex& InVertex2 )
		: Material( InMaterial ),
		  Vertex0( InVertex0 ),
		  Vertex1( InVertex1 ),
		  Vertex2( InVertex2 )
	{
	}

	UPROPERTY()
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY()
	FTLLRN_RenderableTriangleVertex Vertex0;

	UPROPERTY()
	FTLLRN_RenderableTriangleVertex Vertex1;

	UPROPERTY()
	FTLLRN_RenderableTriangleVertex Vertex2;
};

/**
* A component for rendering an arbitrary assortment of triangles. Suitable, for instance, for rendering highlighted faces.
*/
UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_TriangleSetComponent : public UMeshComponent
{
	GENERATED_BODY()

public:

	UTLLRN_TriangleSetComponent();

	/** Clear the triangle set */
	void Clear();

	/** Reserve enough memory for up to the given ID (for inserting via ID) */
	void ReserveTriangles(const int32 MaxID);

	/** Add a triangle to be rendered using the component. */
	int32 AddTriangle(const FTLLRN_RenderableTriangle& OverlayTriangle);

	/** Insert a triangle with the given ID to be rendered using the component. */
	void InsertTriangle(const int32 ID, const FTLLRN_RenderableTriangle& OverlayTriangle);

	/** Remove a triangle from the component. */
	void RemoveTriangle(const int32 ID);

	/** Queries whether a triangle with the given ID exists in the component. */
	bool IsTriangleValid(const int32 ID) const;

	/** Sets the color of all existing triangles */
	void SetAllTrianglesColor(const FColor& NewColor);

	/** Sets the materials of all existing triangles */
	void SetAllTrianglesMaterial(UMaterialInterface* Material);

	/**
	 * Add a triangle with the given vertices, normal, Color, and Material
	 * @return ID of the triangle created
	 */
	int32 AddTriangle(const FVector& A, const FVector& B, const FVector& C, const FVector& Normal, const FColor& Color, UMaterialInterface* Material);

	/**
	 * Add a Quad (two triangles) with the given vertices, normal, Color, and Material
	 * @return ID of the two triangles created
	 */
	UE::Geometry::FIndex2i AddQuad(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Normal, const FColor& Color, UMaterialInterface* Material);

	// utility construction functions

	/**
	 * Add a set of triangles for each index in a sequence
	 * @param NumIndices iterate from 0...NumIndices and call TriangleGenFunc() for each value
	 * @param TriangleGenFunc called to fetch the triangles for an index, callee filles TrianglesOut array (reset before each call)
	 * @param TrianglesPerIndexHint if > 0, will reserve space for NumIndices*TrianglesPerIndexHint new triangles
	 */
	void AddTriangles(
		int32 NumIndices,
		TFunctionRef<void(int32 Index, TArray<FTLLRN_RenderableTriangle>& TrianglesOut)> TriangleGenFunc,
		int32 TrianglesPerIndexHint = -1,
		bool bDeferRenderStateDirty = false);

private:

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	//~ End UMeshComponent Interface.

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End USceneComponent Interface.

	int32 FindOrAddMaterialIndex(UMaterialInterface* Material);

	UPROPERTY()
	mutable FBoxSphereBounds Bounds;

	UPROPERTY()
	mutable bool bBoundsDirty;

	TSparseArray<TTuple<int32, int32>> Triangles;
	TSparseArray<TSparseArray<FTLLRN_RenderableTriangle>> TrianglesByMaterial;
	TMap<UMaterialInterface*, int32> MaterialToIndex;

	int32 AddTriangleInternal(const FTLLRN_RenderableTriangle& OverlayTriangle);

	friend class FTriangleSetSceneProxy;
};