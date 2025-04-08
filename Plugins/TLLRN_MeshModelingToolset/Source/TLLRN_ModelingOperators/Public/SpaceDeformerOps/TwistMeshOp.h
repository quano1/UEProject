// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MeshSpaceDeformerOp.h"

namespace UE
{
namespace Geometry
{

class TLLRN_MODELINGOPERATORS_API FTwistMeshOp : public FMeshSpaceDeformerOp
{
public:
	virtual void CalculateResult(FProgressCancel* Progress) override;

	double TwistDegrees = 180;
	bool bLockBottom = false;
protected:

};

} // end namespace UE::Geometry
} // end namespace UE
