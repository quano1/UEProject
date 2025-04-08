// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveToolObjects.h"
#include "Drawing/TLLRN_TriangleSetComponent.h"
#include "Drawing/TLLRN_LineSetComponent.h"
#include "Drawing/TLLRN_PointSetComponent.h"

#include "TLLRN_PreviewGeometryActor.generated.h"

/**
 * An actor suitable for attaching components used to draw preview elements, such as TLLRN_LineSetComponent and TLLRN_TriangleSetComponent.
 */
UCLASS(Transient, NotPlaceable, Hidden, NotBlueprintable, NotBlueprintType)
class TLLRN_MODELINGCOMPONENTS_API ATLLRN_PreviewGeometryActor : public AInternalToolFrameworkActor
{
	GENERATED_BODY()
private:
	ATLLRN_PreviewGeometryActor()
	{
#if WITH_EDITORONLY_DATA
		// hide this actor in the scene outliner
		bListedInSceneOutliner = false;
#endif
	}

public:
};


/**
 * UTLLRN_PreviewGeometry creates and manages an ATLLRN_PreviewGeometryActor and a set of preview geometry Components.
 * Preview geometry Components are identified by strings.
 */
UCLASS(Transient)
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_PreviewGeometry : public UObject
{
	GENERATED_BODY()

public:
	virtual ~UTLLRN_PreviewGeometry();

	/**
	 * Create preview mesh in the World with the given transform
	 */
	UFUNCTION()
	void CreateInWorld(UWorld* World, const FTransform& WithTransform);

	/**
	 * Remove and destroy preview mesh
	 */
	UFUNCTION()
	void Disconnect();

	/** @return the preview geometry actor created by this class */
	UFUNCTION()
	ATLLRN_PreviewGeometryActor* GetActor() const { return ParentActor;  }



	/**
	 * Get the current transform on the preview 
	 */
	FTransform GetTransform() const;

	/**
	 * Set the transform on the preview mesh
	 */
	void SetTransform(const FTransform& UseTransform);


	/**
	 * Set visibility state of the preview mesh
	 */
	void SetAllVisible(bool bVisible);


	//
	// Triangle Sets
	//

	/** Create a new triangle set with the given TriangleSetIdentifier and return it */
	UFUNCTION()
	UTLLRN_TriangleSetComponent* AddTriangleSet(const FString& TriangleSetIdentifier);

	/** @return the TLLRN_TriangleSetComponent with the given TriangleSetIdentifier, or nullptr if not found */
	UFUNCTION()
	UTLLRN_TriangleSetComponent* FindTriangleSet(const FString& TriangleSetIdentifier);


	//
	// Triangle Set Utilities
	//

	/**
	 * Find the identified triangle set and call UpdateFuncType(UTLLRN_TriangleSetComponent*)
	 */
	template<typename UpdateFuncType>
	void UpdateTriangleSet(const FString& TriangleSetIdentifier, UpdateFuncType UpdateFunc)
	{
		UTLLRN_TriangleSetComponent* TriangleSet = FindTriangleSet(TriangleSetIdentifier);
		if (TriangleSet)
		{
			UpdateFunc(TriangleSet);
		}
	}

	/**
	 * call UpdateFunc for all existing Triangle Sets
	 */
	template<typename UpdateFuncType>
	void UpdateAllTriangleSets(UpdateFuncType UpdateFunc)
	{
		for (TPair<FString, TObjectPtr<UTLLRN_TriangleSetComponent>> Entry : TriangleSets)
		{
			UpdateFunc(Entry.Value);
		}
	}

	/**
	 * Add a set of triangles produced by calling TriangleGenFunc for each index in range [0,NumIndices)
	 * @return the created or updated triangle set
	 */
	UTLLRN_TriangleSetComponent* CreateOrUpdateTriangleSet(const FString& TriangleSetIdentifier, int32 NumIndices,
		TFunctionRef<void(int32 Index, TArray<FTLLRN_RenderableTriangle>& TrianglesOut)> TriangleGenFunc,
		int32 TrianglesPerIndexHint = -1);

	/**
	 * Remove the TLLRN_TriangleSetComponent with the given TriangleSetIdentifier
	 * @param bDestroy if true, component will unregistered and destroyed.
	 * @return true if the TLLRN_TriangleSetComponent was found and removed
	 */
	UFUNCTION()
	bool RemoveTriangleSet(const FString& TriangleSetIdentifier, bool bDestroy = true);

	/**
	 * Remove all TLLRN_TriangleSetComponents
	 * @param bDestroy if true, the components will unregistered and destroyed.
	 */
	UFUNCTION()
	void RemoveAllTriangleSets(bool bDestroy = true);


	//
	// Line Sets
	//

	/** Create a new line set with the given LineSetIdentifier and return it */
	UFUNCTION()
	UTLLRN_LineSetComponent* AddLineSet(const FString& LineSetIdentifier);

	/** @return the TLLRN_LineSetComponent with the given LineSetIdentifier, or nullptr if not found */
	UFUNCTION()
	UTLLRN_LineSetComponent* FindLineSet(const FString& LineSetIdentifier);

	/** 
	 * Remove the TLLRN_LineSetComponent with the given LineSetIdentifier
	 * @param bDestroy if true, component will unregistered and destroyed. 
	 * @return true if the TLLRN_LineSetComponent was found and removed
	 */
	UFUNCTION()
	bool RemoveLineSet(const FString& LineSetIdentifier, bool bDestroy = true);

	/**
	 * Remove all TLLRN_LineSetComponents
	 * @param bDestroy if true, the components will unregistered and destroyed.
	 */
	UFUNCTION()
	void RemoveAllLineSets(bool bDestroy = true);

	/**
	 * Set the visibility of the TLLRN_LineSetComponent with the given LineSetIdentifier
	 * @return true if the TLLRN_LineSetComponent was found and updated
	 */
	UFUNCTION()
	bool SetLineSetVisibility(const FString& LineSetIdentifier, bool bVisible);

	/**
	 * Set the Material of the TLLRN_LineSetComponent with the given LineSetIdentifier
	 * @return true if the TLLRN_LineSetComponent was found and updated
	 */
	UFUNCTION()
	bool SetLineSetMaterial(const FString& LineSetIdentifier, UMaterialInterface* NewMaterial);

	/**
	 * Set the Material of all TLLRN_LineSetComponents
	 */
	UFUNCTION()
	void SetAllLineSetsMaterial(UMaterialInterface* Material);



	//
	// Line Set Utilities
	//

	/**
	 * Find the identified line set and call UpdateFuncType(UTLLRN_LineSetComponent*)
	 */
	template<typename UpdateFuncType>
	void UpdateLineSet(const FString& LineSetIdentifier, UpdateFuncType UpdateFunc)
	{
		UTLLRN_LineSetComponent* LineSet = FindLineSet(LineSetIdentifier);
		if (LineSet)
		{
			UpdateFunc(LineSet);
		}
	}

	/**
	 * call UpdateFuncType(UTLLRN_LineSetComponent*) for all existing Line Sets
	 */
	template<typename UpdateFuncType>
	void UpdateAllLineSets(UpdateFuncType UpdateFunc)
	{
		for (TPair<FString, TObjectPtr<UTLLRN_LineSetComponent>> Entry : LineSets)
		{
			UpdateFunc(Entry.Value);
		}
	}

	/**
	 * Add a set of lines produced by calling LineGenFunc for each index in range [0,NumIndices)
	 * @return the created or updated line set
	 */
	UTLLRN_LineSetComponent* CreateOrUpdateLineSet(const FString& LineSetIdentifier, int32 NumIndices,
		TFunctionRef<void(int32 Index, TArray<FRenderableLine>& LinesOut)> LineGenFunc,
		int32 LinesPerIndexHint = -1);

	//
	// Point Sets
	//

	/** Create a new point set with the given PointSetIdentifier and return it */
	UFUNCTION()
	UTLLRN_PointSetComponent* AddPointSet(const FString& PointSetIdentifier);

	/** @return the TLLRN_PointSetComponent with the given PointSetIdentifier, or nullptr if not found */
	UFUNCTION()
	UTLLRN_PointSetComponent* FindPointSet(const FString& PointSetIdentifier);

	/** 
	 * Remove the TLLRN_PointSetComponent with the given PointSetIdentifier
	 * @param bDestroy if true, component will unregistered and destroyed. 
	 * @return true if the TLLRN_PointSetComponent was found and removed
	 */
	UFUNCTION()
	bool RemovePointSet(const FString& PointSetIdentifier, bool bDestroy = true);

	/**
	 * Remove all TLLRN_PointSetComponents
	 * @param bDestroy if true, the components will unregistered and destroyed.
	 */
	UFUNCTION()
	void RemoveAllPointSets(bool bDestroy = true);

	/**
	 * Set the visibility of the TLLRN_PointSetComponent with the given PointSetIdentifier
	 * @return true if the TLLRN_PointSetComponent was found and updated
	 */
	UFUNCTION()
	bool SetPointSetVisibility(const FString& PointSetIdentifier, bool bVisible);

	/**
	 * Set the Material of the TLLRN_PointSetComponent with the given PointSetIdentifier
	 * @return true if the TLLRN_PointSetComponent was found and updated
	 */
	UFUNCTION()
	bool SetPointSetMaterial(const FString& PointSetIdentifier, UMaterialInterface* NewMaterial);

	/**
	 * Set the Material of all TLLRN_PointSetComponents
	 */
	UFUNCTION()
	void SetAllPointSetsMaterial(UMaterialInterface* Material);



	//
	// Point Set Utilities
	//

	/**
	 * Find the identified point set and call UpdateFuncType(UTLLRN_PointSetComponent*)
	 */
	template<typename UpdateFuncType>
	void UpdatePointSet(const FString& PointSetIdentifier, UpdateFuncType UpdateFunc)
	{
		UTLLRN_PointSetComponent* PointSet = FindPointSet(PointSetIdentifier);
		if (PointSet)
		{
			UpdateFunc(PointSet);
		}
	}

	/**
	 * call UpdateFuncType(UTLLRN_PointSetComponent*) for all existing Point Sets
	 */
	template<typename UpdateFuncType>
	void UpdateAllPointSets(UpdateFuncType UpdateFunc)
	{
		for (TPair<FString, TObjectPtr<UTLLRN_PointSetComponent>> Entry : PointSets)
		{
			UpdateFunc(Entry.Value);
		}
	}

	/**
	 * Add a set of points produced by calling PointGenFunc for each index in range [0,NumIndices)
	 */
	void CreateOrUpdatePointSet(const FString& PointSetIdentifier, int32 NumIndices,
		TFunctionRef<void(int32 Index, TArray<FRenderablePoint>& PointsOut)> PointGenFunc,
		int32 PointsPerIndexHint = -1);


public:

	/**
	 * Actor created and managed by the UTLLRN_PreviewGeometry
	 */
	UPROPERTY()
	TObjectPtr<ATLLRN_PreviewGeometryActor> ParentActor = nullptr;

	/**
	 * TLLRN_TriangleSetComponents created and owned by the UTLLRN_PreviewGeometry, and added as child components of the ParentActor
	 */
	UPROPERTY()
	TMap<FString, TObjectPtr<UTLLRN_TriangleSetComponent>> TriangleSets;

	/**
	 * TLLRN_LineSetComponents created and owned by the UTLLRN_PreviewGeometry, and added as child components of the ParentActor
	 */
	UPROPERTY()
	TMap<FString, TObjectPtr<UTLLRN_LineSetComponent>> LineSets;

	/**
	 * TLLRN_PointSetComponents created and owned by the UTLLRN_PreviewGeometry, and added as child components of the ParentActor
	 */
	UPROPERTY()
	TMap<FString, TObjectPtr<UTLLRN_PointSetComponent>> PointSets;

protected:

	// called at the end of CreateInWorld() to allow subclasses to add additional setup
	virtual void OnCreated() {}

	// called at the beginning of Disconnect() to allow subclasses to perform additional cleanup
	virtual void OnDisconnected() {}
};