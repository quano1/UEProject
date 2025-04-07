// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/ObjectMacros.h"
#include "ThumbnailRendering/SkeletalMeshThumbnailRenderer.h"
#include "TLLRN_IKRetargeterThumbnailRenderer.generated.h"

class FCanvas;
class FRenderTarget;
class ASkeletalMeshActor;
enum class ETLLRN_RetargetSourceOrTarget : uint8;

class TLLRN_IKRIGEDITOR_API FTLLRN_IKRetargeterThumbnailScene : public FThumbnailPreviewScene
{
public:
	
	FTLLRN_IKRetargeterThumbnailScene();

	// sets the retargeter to use in the next CreateView()
	void SetSkeletalMeshes(USkeletalMesh* SourceMesh, USkeletalMesh* TargetMesh) const;

protected:
	// FThumbnailPreviewScene implementation
	virtual void GetViewMatrixParameters(const float InFOVDegrees, FVector& OutOrigin, float& OutOrbitPitch, float& OutOrbitYaw, float& OutOrbitZoom) const override;

private:
	// the skeletal mesh actors used to display all skeletal mesh thumbnails
	ASkeletalMeshActor* SourceActor;
	ASkeletalMeshActor* TargetActor;
};


// this thumbnail renderer displays a given IK Rig in the asset icon
UCLASS(config=Editor, MinimalAPI)
class UTLLRN_IKRetargeterThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
	GENERATED_BODY()

	// Begin UThumbnailRenderer Object
	TLLRN_IKRIGEDITOR_API virtual bool CanVisualizeAsset(UObject* Object) override;
	TLLRN_IKRIGEDITOR_API virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas, bool bAdditionalViewFamily) override;
	TLLRN_IKRIGEDITOR_API virtual EThumbnailRenderFrequency GetThumbnailRenderFrequency(UObject* Object) const override;
	// End UThumbnailRenderer Object
	
	// UObject implementation
	virtual void BeginDestroy() override;
	// End UObject implementation

	USkeletalMesh* GetPreviewMeshFromAsset(UObject* Object, ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	bool HasSourceOrTargetMesh(UObject* Object) const;

protected:
	TObjectInstanceThumbnailScene<FTLLRN_IKRetargeterThumbnailScene, 128> ThumbnailSceneCache;
};

