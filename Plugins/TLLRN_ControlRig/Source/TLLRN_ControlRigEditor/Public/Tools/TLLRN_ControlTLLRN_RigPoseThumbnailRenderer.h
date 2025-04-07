// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * This thumbnail renderer displays a given Control Rig Pose Asset
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ThumbnailRendering/SkeletalMeshThumbnailRenderer.h"
#include "Tools/TLLRN_ControlTLLRN_RigPose.h"
#include "TLLRN_ControlTLLRN_RigPoseThumbnailRenderer.generated.h"

class FCanvas;
class FRenderTarget;
class FTLLRN_ControlTLLRN_RigPoseThumbnailScene;

UCLASS(config = Editor, MinimalAPI)
class UTLLRN_ControlTLLRN_RigPoseThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
	GENERATED_UCLASS_BODY()

	// Begin UThumbnailRenderer Object
	virtual bool CanVisualizeAsset(UObject* Object) override;
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas, bool bAdditionalViewFamily) override;
	// End UThumbnailRenderer Object

	// UObject Implementation
	virtual void BeginDestroy() override;
	// End UObject Implementation

private:
	FTLLRN_ControlTLLRN_RigPoseThumbnailScene* ThumbnailScene;
protected:
	UTLLRN_ControlTLLRN_RigPoseAsset* TLLRN_ControlTLLRN_RigPoseAsset;
};
