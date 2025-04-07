// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTemplate.h"
#include "Evaluation/Blending/MovieSceneMultiChannelBlending.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"
#include "TLLRN_ControlRig.h"
#include "Evaluation/MovieSceneEvaluation.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneCommonHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "AnimCustomInstanceHelper.h"
#include "Sequencer/TLLRN_ControlRigLayerInstance.h"
#include "ITLLRN_ControlRigObjectBinding.h"
#include "Animation/AnimData/BoneMaskFilter.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "Evaluation/Blending/BlendableTokenStack.h"
#include "Evaluation/Blending/MovieSceneBlendingActuatorID.h"
#include "TransformNoScale.h"
#include "TLLRN_ControlRigComponent.h"
#include "SkeletalMeshRestoreState.h"
#include "Rigs/FKTLLRN_ControlRig.h"
#include "UObject/UObjectAnnotation.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "TransformConstraint.h"
#include "TransformableHandle.h"
#include "AnimationCoreLibrary.h"
#include "Constraints/TLLRN_ControlTLLRN_RigTransformableHandle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovieSceneTLLRN_ControlRigParameterTemplate)

//#include "Particles/ParticleSystemComponent.h"

DECLARE_CYCLE_STAT(TEXT("TLLRN_ControlRig Parameter Track Evaluate"), MovieSceneEval_TLLRN_ControlRigTemplateParameter_Evaluate, STATGROUP_MovieSceneEval);
DECLARE_CYCLE_STAT(TEXT("TLLRN_ControlRig Parameter Track Token Execute"), MovieSceneEval_TLLRN_ControlRigParameterTrack_TokenExecute, STATGROUP_MovieSceneEval);

template<typename T>
struct TNameAndValue
{
	FName Name;
	T Value;
};


/**
 * Structure representing the animated value of a scalar parameter.
 */
struct FScalarParameterStringAndValue
{
	/** Creates a new FScalarParameterAndValue with a parameter name and a value. */
	FScalarParameterStringAndValue(FName InParameterName, float InValue)
	{
		ParameterName = InParameterName;
		Value = InValue;
	}

	/** The name of the scalar parameter. */
	FName ParameterName;
	/** The animated value of the scalar parameter. */
	float Value;
};

/**
 * Structure representing the animated value of a bool parameter.
 */
struct FBoolParameterStringAndValue
{
	/** Creates a new FBoolParameterAndValue with a parameter name and a value. */
	FBoolParameterStringAndValue(FName InParameterName, bool InValue)
	{
		ParameterName = InParameterName;
		Value = InValue;
	}

	/** The name of the bool parameter. */
	FName ParameterName;
	/** The animated value of the bool parameter. */
	bool Value;
};

/**
 * Structure representing the animated value of a int parameter.
 */
struct FIntegerParameterStringAndValue
{
	FIntegerParameterStringAndValue(FName InParameterName, int32 InValue)
	{
		ParameterName = InParameterName;
		Value = InValue;
	}

	FName ParameterName;
	int32 Value;
};

struct FControlSpaceAndValue
{
	FControlSpaceAndValue(FName InControlName, const FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey& InValue)
	{
		ControlName = InControlName;
		Value = InValue;
	}
	FName ControlName;
	FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Value;
};
/**
 * Structure representing the animated value of a vector2D parameter.
 */
struct FVector2DParameterStringAndValue
{
	/** Creates a new FVector2DParameterAndValue with a parameter name and a value. */
	FVector2DParameterStringAndValue(FName InParameterName, FVector2D InValue)
	{
		ParameterName = InParameterName;
		Value = InValue;
	}

	/** The name of the vector2D parameter. */
	FName ParameterName;

	/** The animated value of the vector2D parameter. */
	FVector2D Value;
};

/**
 * Structure representing the animated value of a vector parameter.
 */
struct FVectorParameterStringAndValue
{
	/** Creates a new FVectorParameterAndValue with a parameter name and a value. */
	FVectorParameterStringAndValue(FName InParameterName, FVector InValue)
	{
		ParameterName = InParameterName;
		Value = InValue;
	}

	/** The name of the vector parameter. */
	FName ParameterName;

	/** The animated value of the vector parameter. */
	FVector Value;
};


/**
 * Structure representing the animated value of a color parameter.
 */
struct FColorParameterStringAndValue
{
	/** Creates a new FColorParameterAndValue with a parameter name and a value. */
	FColorParameterStringAndValue(FName InParameterName, FLinearColor InValue)
	{
		ParameterName = InParameterName;
		Value = InValue;
	}

	/** The name of the color parameter. */
	FName ParameterName;

	/** The animated value of the color parameter. */
	FLinearColor Value;
};

struct FEulerTransformParameterStringAndValue
{

	/** The name of the transform  parameter. */
	FName ParameterName;
	/** Transform component */
	FEulerTransform Transform;

	FEulerTransformParameterStringAndValue(FName InParameterName, const FEulerTransform& InTransform)
		: Transform(InTransform)
	{
		ParameterName = InParameterName;
	}
};

struct FConstraintAndActiveValue
{
	FConstraintAndActiveValue(TWeakObjectPtr<UTickableConstraint> InConstraint,  bool InValue)
		: Constraint(InConstraint)
		, Value(InValue)
	{}
	TWeakObjectPtr<UTickableConstraint> Constraint;
	bool Value;
};

struct FTLLRN_ControlRigAnimTypeIDs;

/** Thread-safe because objects can be destroyed on background threads */
using FTLLRN_ControlRigAnimTypeIDsPtr = TSharedPtr<FTLLRN_ControlRigAnimTypeIDs, ESPMode::ThreadSafe>;

/**
 * Control rig anim type IDs are a little complex - they require a unique type ID for every bone
 * and they must be unique per-animating control rig. To efficiently support finding these each frame,
 * We store a cache of the type IDs in a container on an object annotation for each control rig.
 */
struct FTLLRN_ControlRigAnimTypeIDs
{
	/** Get the anim type IDs for the specified section */
	static FTLLRN_ControlRigAnimTypeIDsPtr Get(const UTLLRN_ControlRig* TLLRN_ControlRig)
	{
		struct FTLLRN_ControlRigAnimTypeIDsAnnotation
		{
			// IsDefault should really have been implemented as a trait rather than a function so that this type isn't necessary
			bool IsDefault() const
			{
				return Ptr == nullptr;
			}
			FTLLRN_ControlRigAnimTypeIDsPtr Ptr;
		};

		// Function-local static so that this only gets created once it's actually required
		static FUObjectAnnotationSparse<FTLLRN_ControlRigAnimTypeIDsAnnotation, true> AnimTypeIDAnnotation;

		FTLLRN_ControlRigAnimTypeIDsAnnotation TypeIDs = AnimTypeIDAnnotation.GetAnnotation(TLLRN_ControlRig);
		if (TypeIDs.Ptr != nullptr)
		{
			return TypeIDs.Ptr;
		}

		FTLLRN_ControlRigAnimTypeIDsPtr NewPtr = MakeShared<FTLLRN_ControlRigAnimTypeIDs, ESPMode::ThreadSafe>();
		AnimTypeIDAnnotation.AddAnnotation(TLLRN_ControlRig, FTLLRN_ControlRigAnimTypeIDsAnnotation{NewPtr});
		return NewPtr;
	}

	/** Find the anim-type ID for the specified scalar parameter */
	FMovieSceneAnimTypeID FindScalar(FName InParameterName)
	{
		return FindImpl(InParameterName, ScalarAnimTypeIDsByName);
	}
	/** Find the anim-type ID for the specified Vector2D parameter */
	FMovieSceneAnimTypeID FindVector2D(FName InParameterName)
	{
		return FindImpl(InParameterName, Vector2DAnimTypeIDsByName);
	}
	/** Find the anim-type ID for the specified vector parameter */
	FMovieSceneAnimTypeID FindVector(FName InParameterName)
	{
		return FindImpl(InParameterName, VectorAnimTypeIDsByName);
	}
	/** Find the anim-type ID for the specified transform parameter */
	FMovieSceneAnimTypeID FindTransform(FName InParameterName)
	{
		return FindImpl(InParameterName, TransformAnimTypeIDsByName);
	}
private:

	/** Sorted map should give the best trade-off for lookup speed with relatively small numbers of bones (O(log n)) */
	using MapType = TSortedMap<FName, FMovieSceneAnimTypeID, FDefaultAllocator, FNameFastLess>;

	static FMovieSceneAnimTypeID FindImpl(FName InParameterName, MapType& InOutMap)
	{
		if (const FMovieSceneAnimTypeID* Type = InOutMap.Find(InParameterName))
		{
			return *Type;
		}
		FMovieSceneAnimTypeID New = FMovieSceneAnimTypeID::Unique();
		InOutMap.Add(InParameterName, FMovieSceneAnimTypeID::Unique());
		return New;
	}
	/** Array of existing type identifiers */
	MapType ScalarAnimTypeIDsByName;
	MapType Vector2DAnimTypeIDsByName;
	MapType VectorAnimTypeIDsByName;
	MapType TransformAnimTypeIDsByName;
};

/**
 * Cache structure that is stored per-section that defines bitmasks for every
 * index within each curve type. Set bits denote that the curve should be 
 * evaluated. Only ever initialized once since the template will get re-created
 * whenever the control rig section changes
 */
struct FEvaluatedTLLRN_ControlRigParameterSectionChannelMasks : IPersistentEvaluationData
{
	TBitArray<> ScalarCurveMask;
	TBitArray<> BoolCurveMask;
	TBitArray<> IntegerCurveMask;
	TBitArray<> EnumCurveMask;
	TBitArray<> Vector2DCurveMask;
	TBitArray<> VectorCurveMask;
	TBitArray<> ColorCurveMask;
	TBitArray<> TransformCurveMask;

	void Initialize(UMovieSceneTLLRN_ControlRigParameterSection* Section,
		TArrayView<const FScalarParameterNameAndCurve> Scalars,
		TArrayView<const FBoolParameterNameAndCurve> Bools,
		TArrayView<const FTLLRN_IntegerParameterNameAndCurve> Integers,
		TArrayView<const FTLLRN_EnumParameterNameAndCurve> Enums,
		TArrayView<const FVector2DParameterNameAndCurves> Vector2Ds,
		TArrayView<const FVectorParameterNameAndCurves> Vectors,
		TArrayView<const FColorParameterNameAndCurves> Colors,
		TArrayView<const FTransformParameterNameAndCurves> Transforms
		)
	{
		const FTLLRN_ChannelMapInfo* ChannelInfo = nullptr;

		ScalarCurveMask.Add(false, Scalars.Num());
		BoolCurveMask.Add(false, Bools.Num());
		IntegerCurveMask.Add(false, Integers.Num());
		EnumCurveMask.Add(false, Enums.Num());
		Vector2DCurveMask.Add(false, Vector2Ds.Num());
		VectorCurveMask.Add(false, Vectors.Num());
		ColorCurveMask.Add(false, Colors.Num());
		TransformCurveMask.Add(false, Transforms.Num());
		
		for (int32 Index = 0; Index < Scalars.Num(); ++Index)
		{
			const FScalarParameterNameAndCurve& Scalar = Scalars[Index];
			ChannelInfo = Section->ControlChannelMap.Find(Scalar.ParameterName);
			ScalarCurveMask[Index] = (!ChannelInfo || Section->GetControlNameMask(Scalar.ParameterName));
		}
		for (int32 Index = 0; Index < Bools.Num(); ++Index)
		{
			const FBoolParameterNameAndCurve& Bool = Bools[Index];
			ChannelInfo = Section->ControlChannelMap.Find(Bool.ParameterName);
			BoolCurveMask[Index] = (!ChannelInfo || Section->GetControlNameMask(Bool.ParameterName));
		}
		for (int32 Index = 0; Index < Integers.Num(); ++Index)
		{
			const FTLLRN_IntegerParameterNameAndCurve& Integer = Integers[Index];
			ChannelInfo = Section->ControlChannelMap.Find(Integer.ParameterName);
			IntegerCurveMask[Index] = (!ChannelInfo || Section->GetControlNameMask(Integer.ParameterName));
		}
		for (int32 Index = 0; Index < Enums.Num(); ++Index)
		{
			const FTLLRN_EnumParameterNameAndCurve& Enum = Enums[Index];
			ChannelInfo = Section->ControlChannelMap.Find(Enum.ParameterName);
			EnumCurveMask[Index] = (!ChannelInfo || Section->GetControlNameMask(Enum.ParameterName));
		}
		for (int32 Index = 0; Index < Vector2Ds.Num(); ++Index)
		{
			const FVector2DParameterNameAndCurves& Vector2D = Vector2Ds[Index];
			ChannelInfo = Section->ControlChannelMap.Find(Vector2D.ParameterName);
			Vector2DCurveMask[Index] = (!ChannelInfo || Section->GetControlNameMask(Vector2D.ParameterName));
		}
		for (int32 Index = 0; Index < Vectors.Num(); ++Index)
		{
			const FVectorParameterNameAndCurves& Vector = Vectors[Index];
			ChannelInfo = Section->ControlChannelMap.Find(Vector.ParameterName);
			VectorCurveMask[Index] = (!ChannelInfo || Section->GetControlNameMask(Vector.ParameterName));
		}
		for (int32 Index = 0; Index < Colors.Num(); ++Index)
		{
			const FColorParameterNameAndCurves& Color = Colors[Index];
			ChannelInfo = Section->ControlChannelMap.Find(Color.ParameterName);
			ColorCurveMask[Index] = (!ChannelInfo || Section->GetControlNameMask(Color.ParameterName));
		}
		for (int32 Index = 0; Index < Transforms.Num(); ++Index)
		{
			const FTransformParameterNameAndCurves& Transform = Transforms[Index];
			ChannelInfo = Section->ControlChannelMap.Find(Transform.ParameterName);
			TransformCurveMask[Index] = (!ChannelInfo || Section->GetControlNameMask(Transform.ParameterName));
		}
	}
};
// Static hack because we cannot add this to the function parameters for EvaluateCurvesWithMasks due to hotfix restrictions
static FEvaluatedTLLRN_ControlRigParameterSectionChannelMasks* HACK_ChannelMasks = nullptr;

struct FEvaluatedTLLRN_ControlRigParameterSectionValues
{
	FEvaluatedTLLRN_ControlRigParameterSectionValues() = default;

	FEvaluatedTLLRN_ControlRigParameterSectionValues(FEvaluatedTLLRN_ControlRigParameterSectionValues&&) = default;
	FEvaluatedTLLRN_ControlRigParameterSectionValues& operator=(FEvaluatedTLLRN_ControlRigParameterSectionValues&&) = default;

	// Non-copyable
	FEvaluatedTLLRN_ControlRigParameterSectionValues(const FEvaluatedTLLRN_ControlRigParameterSectionValues&) = delete;
	FEvaluatedTLLRN_ControlRigParameterSectionValues& operator=(const FEvaluatedTLLRN_ControlRigParameterSectionValues&) = delete;

	/** Array of evaluated scalar values */
	TArray<FScalarParameterStringAndValue, TInlineAllocator<2>> ScalarValues;
	/** Array of evaluated bool values */
	TArray<FBoolParameterStringAndValue, TInlineAllocator<2>> BoolValues;
	/** Array of evaluated integer values */
	TArray<FIntegerParameterStringAndValue, TInlineAllocator<2>> IntegerValues;
	/** Array of evaluated Spaces */
	TArray<FControlSpaceAndValue, TInlineAllocator<2>> SpaceValues;
	/** Array of evaluated vector2d values */
	TArray<FVector2DParameterStringAndValue, TInlineAllocator<2>> Vector2DValues;
	/** Array of evaluated vector values */
	TArray<FVectorParameterStringAndValue, TInlineAllocator<2>> VectorValues;
	/** Array of evaluated color values */
	TArray<FColorParameterStringAndValue, TInlineAllocator<2>> ColorValues;
	/** Array of evaluated transform values */
	TArray<FEulerTransformParameterStringAndValue, TInlineAllocator<2>> TransformValues;
	/** Array of evaluated constraint values */
	TArray<FConstraintAndActiveValue, TInlineAllocator<2>> ConstraintsValues;
};

/** Token for control rig control parameters */
struct FTLLRN_ControlRigTrackTokenFloat
{
	FTLLRN_ControlRigTrackTokenFloat() {}
	
	FTLLRN_ControlRigTrackTokenFloat(float InValue)
		:Value(InValue)
	{}

	float Value;

};

struct FTLLRN_ControlRigTrackTokenBool
{
	FTLLRN_ControlRigTrackTokenBool() {}

	FTLLRN_ControlRigTrackTokenBool(bool InValue)
		:Value(InValue)
	{}

	bool Value;
};

struct FTLLRN_ControlRigTrackTokenVector2D
{
	FTLLRN_ControlRigTrackTokenVector2D() {}
	FTLLRN_ControlRigTrackTokenVector2D(FVector2D InValue)
		:Value(InValue)
	{}

	FVector2D Value;
};

struct FTLLRN_ControlRigTrackTokenVector
{
	FTLLRN_ControlRigTrackTokenVector() {}
	FTLLRN_ControlRigTrackTokenVector(FVector InValue)
		:Value(InValue)
	{}

	FVector Value;
};

struct FTLLRN_ControlRigTrackTokenTransform
{
	FTLLRN_ControlRigTrackTokenTransform() {}
	FTLLRN_ControlRigTrackTokenTransform(FEulerTransform InValue)
		: Value(InValue)
	{}
	FEulerTransform Value;

};



// Specify a unique runtime type identifier for rig control track tokens
template<> FMovieSceneAnimTypeID GetBlendingDataType<FTLLRN_ControlRigTrackTokenFloat>()
{
	static FMovieSceneAnimTypeID TypeId = FMovieSceneAnimTypeID::Unique();
	return TypeId;
}

template<> FMovieSceneAnimTypeID GetBlendingDataType<FTLLRN_ControlRigTrackTokenBool>()
{
	static FMovieSceneAnimTypeID TypeId = FMovieSceneAnimTypeID::Unique();
	return TypeId;
}

template<> FMovieSceneAnimTypeID GetBlendingDataType<FTLLRN_ControlRigTrackTokenVector2D>()
{
	static FMovieSceneAnimTypeID TypeId = FMovieSceneAnimTypeID::Unique();
	return TypeId;
}

template<> FMovieSceneAnimTypeID GetBlendingDataType<FTLLRN_ControlRigTrackTokenVector>()
{
	static FMovieSceneAnimTypeID TypeId = FMovieSceneAnimTypeID::Unique();
	return TypeId;
}

template<> FMovieSceneAnimTypeID GetBlendingDataType<FTLLRN_ControlRigTrackTokenTransform>()
{
	static FMovieSceneAnimTypeID TypeId = FMovieSceneAnimTypeID::Unique();
	return TypeId;
}



/** Define working data types for blending calculations  */
template<>  struct TBlendableTokenTraits<FTLLRN_ControlRigTrackTokenFloat>
{
	typedef UE::MovieScene::TMaskedBlendable<float, 1> WorkingDataType;
};

template<>  struct TBlendableTokenTraits<FTLLRN_ControlRigTrackTokenBool>
{
	typedef UE::MovieScene::TMaskedBlendable<bool, 1> WorkingDataType;
};

template<> struct TBlendableTokenTraits<FTLLRN_ControlRigTrackTokenVector2D>
{
	typedef UE::MovieScene::TMaskedBlendable<float, 2> WorkingDataType;
};

template<> struct TBlendableTokenTraits<FTLLRN_ControlRigTrackTokenVector>
{
	typedef UE::MovieScene::TMaskedBlendable<float, 3> WorkingDataType;
};

template<>  struct TBlendableTokenTraits<FTLLRN_ControlRigTrackTokenTransform>
{
	typedef UE::MovieScene::TMaskedBlendable<float, 9> WorkingDataType;
};




namespace UE
{
namespace MovieScene
{

	void MultiChannelFromData(const FTLLRN_ControlRigTrackTokenFloat& In, TMultiChannelValue<float, 1>& Out)
	{
		Out = { In.Value };
	}

	void ResolveChannelsToData(const TMultiChannelValue<float, 1>& In, FTLLRN_ControlRigTrackTokenFloat& Out)
	{
		Out.Value = In[0];
	}

	void MultiChannelFromData(const FTLLRN_ControlRigTrackTokenBool& In, TMultiChannelValue<bool, 1>& Out)
	{
		Out = { In.Value };
	}

	void ResolveChannelsToData(const TMultiChannelValue<bool, 1>& In, FTLLRN_ControlRigTrackTokenBool& Out)
	{
		Out.Value = In[0];
	}

	void MultiChannelFromData(const FTLLRN_ControlRigTrackTokenVector2D& In, TMultiChannelValue<float, 2>& Out)
	{
		Out = { In.Value.X, In.Value.Y };
	}

	void ResolveChannelsToData(const TMultiChannelValue<float, 2>& In, FTLLRN_ControlRigTrackTokenVector2D& Out)
	{
		Out.Value = FVector2D(In[0], In[1]);
	}

	void MultiChannelFromData(const FTLLRN_ControlRigTrackTokenVector& In, TMultiChannelValue<float, 3>& Out)
	{
		Out = { In.Value.X, In.Value.Y, In.Value.Z };
	}

	void ResolveChannelsToData(const TMultiChannelValue<float, 3>& In, FTLLRN_ControlRigTrackTokenVector& Out)
	{
		Out.Value = FVector(In[0], In[1], In[2]);
	}

	void MultiChannelFromData(const FTLLRN_ControlRigTrackTokenTransform& In, TMultiChannelValue<float, 9>& Out)
	{
		FVector Translation = In.Value.GetLocation();
		FVector Rotation = In.Value.Rotator().Euler();
		FVector Scale = In.Value.GetScale3D();
		Out = { Translation.X, Translation.Y, Translation.Z, Rotation.X, Rotation.Y, Rotation.Z, Scale.X, Scale.Y, Scale.Z };

	}

	void ResolveChannelsToData(const TMultiChannelValue<float, 9>& In, FTLLRN_ControlRigTrackTokenTransform& Out)
	{
		Out.Value = FEulerTransform(
			FRotator::MakeFromEuler(FVector(In[3], In[4], In[5])),
			FVector(In[0], In[1], In[2]),
			FVector(In[6], In[7], In[8])
		);
	}

} // namespace MovieScene
} // namespace UE

//since initialization can blow up selection, may need to just reselect, used in a few places
static void SelectControls(UTLLRN_ControlRig* TLLRN_ControlRig, TArray<FName>& SelectedNames)
{
	if (TLLRN_ControlRig)
	{
		TLLRN_ControlRig->ClearControlSelection();
		for (const FName& Name : SelectedNames)
		{
			TLLRN_ControlRig->SelectControl(Name, true);
		}
	}
}

void FTLLRN_ControlRigBindingHelper::BindToSequencerInstance(UTLLRN_ControlRig* TLLRN_ControlRig)
{
	if (!TLLRN_ControlRig)
	{
		return;
	}
	if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(TLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
	{
		if(SkeletalMeshComponent->GetSkeletalMeshAsset())
		{
			bool bWasCreated = false;
			if (UTLLRN_ControlRigLayerInstance* AnimInstance = FAnimCustomInstanceHelper::BindToSkeletalMeshComponent<UTLLRN_ControlRigLayerInstance>(SkeletalMeshComponent, bWasCreated))
			{
				if (bWasCreated || !AnimInstance->HasTLLRN_ControlRigTrack(TLLRN_ControlRig->GetUniqueID()))
				{
					AnimInstance->RecalcRequiredBones();
					AnimInstance->AddTLLRN_ControlRigTrack(TLLRN_ControlRig->GetUniqueID(), TLLRN_ControlRig);
					//initialization can blow up selection
					TArray<FName> SelectedControls = TLLRN_ControlRig->CurrentControlSelection();
					TLLRN_ControlRig->Initialize();
					TLLRN_ControlRig->RequestInit();
					TLLRN_ControlRig->SetBoneInitialTransformsFromSkeletalMeshComponent(SkeletalMeshComponent, true);
					TLLRN_ControlRig->Evaluate_AnyThread();
					TArray<FName> NewSelectedControls = TLLRN_ControlRig->CurrentControlSelection();
					if (SelectedControls != NewSelectedControls)
					{
						SelectControls(TLLRN_ControlRig, SelectedControls);
					}
				}
			}
		}
	}
	else if (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent = Cast<UTLLRN_ControlRigComponent>(TLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
	{
		if (TLLRN_ControlRigComponent->GetTLLRN_ControlRig() != TLLRN_ControlRig)
		{
			TLLRN_ControlRigComponent->Initialize();
			/*
			Previously with Sequencer and CR Components we would assign the CR to a Component
			that the sequencer was using, in any world. This looks like it was causing issues
			with two worlds running with pre-forward solve events so now we only do that if
			in non-game, and if in game (which includes PIE), we don't re-set the
			CR Component's CR, but instead grab the CR from it and then use that for evaluation.
			*/
			if (TLLRN_ControlRigComponent->GetWorld())
			{
				if (TLLRN_ControlRigComponent->GetWorld()->IsGameWorld() == false)
				{
					TLLRN_ControlRigComponent->SetTLLRN_ControlRig(TLLRN_ControlRig);
				}
			}
		}
	}
}

void FTLLRN_ControlRigBindingHelper::UnBindFromSequencerInstance(UTLLRN_ControlRig* TLLRN_ControlRig)
{
	check(TLLRN_ControlRig);

	if (!TLLRN_ControlRig->IsValidLowLevel() ||
	    URigVMHost::IsGarbageOrDestroyed(TLLRN_ControlRig) ||
		!IsValid(TLLRN_ControlRig))
	{
		return;
	}
	
	if (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent = Cast<UTLLRN_ControlRigComponent>(TLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
	{
		// todo: how do we reset the state?
		//TLLRN_ControlRig->Initialize();
	}
	else if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(TLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
	{
		if (!SkeletalMeshComponent->IsValidLowLevel() ||
			URigVMHost::IsGarbageOrDestroyed(SkeletalMeshComponent) ||
			!IsValid(SkeletalMeshComponent))
		{
			return;
		}

		UTLLRN_ControlRigLayerInstance* AnimInstance = Cast<UTLLRN_ControlRigLayerInstance>(SkeletalMeshComponent->GetAnimInstance());
		bool bShouldUnbind = true;
		if (AnimInstance)
		{
			if (!AnimInstance->IsValidLowLevel() ||
                URigVMHost::IsGarbageOrDestroyed(AnimInstance) ||
                !IsValid(AnimInstance))
			{
				return;
			}

			AnimInstance->ResetNodes();
			AnimInstance->RecalcRequiredBones();
			AnimInstance->RemoveTLLRN_ControlRigTrack(TLLRN_ControlRig->GetUniqueID());

			bShouldUnbind = AnimInstance->GetFirstAvailableTLLRN_ControlRig() == nullptr;
		}

		if (bShouldUnbind)
		{
			FAnimCustomInstanceHelper::UnbindFromSkeletalMeshComponent< UTLLRN_ControlRigLayerInstance>(SkeletalMeshComponent);
		}
	}
}

struct FTLLRN_ControlRigSkeletalMeshComponentBindingTokenProducer : IMovieScenePreAnimatedTokenProducer
{
	FTLLRN_ControlRigSkeletalMeshComponentBindingTokenProducer(FMovieSceneSequenceIDRef InSequenceID, UTLLRN_ControlRig* InTLLRN_ControlRig)
		: SequenceID(InSequenceID), TLLRN_ControlRigUniqueID(InTLLRN_ControlRig->GetUniqueID())
	{}

	static FMovieSceneAnimTypeID GetAnimTypeID()
	{
		return TMovieSceneAnimTypeID<FTLLRN_ControlRigSkeletalMeshComponentBindingTokenProducer>();
	}

	virtual IMovieScenePreAnimatedTokenPtr CacheExistingState(UObject& Object) const
	{
		struct FToken : IMovieScenePreAnimatedToken
		{
			FToken(FMovieSceneSequenceIDRef InSequenceID, uint32 InTLLRN_ControlRigUniqueID)
				: SequenceID(InSequenceID), TLLRN_ControlRigUniqueID(InTLLRN_ControlRigUniqueID)
			{

			}
			
			virtual void RestoreState(UObject& InObject, const UE::MovieScene::FRestoreStateParams& Params) override
			{
				if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(&InObject))
				{
					if (UTLLRN_ControlRigLayerInstance* TLLRN_ControlRigLayerInstance = Cast<UTLLRN_ControlRigLayerInstance>(SkeletalMeshComponent->GetAnimInstance()))
					{
						TLLRN_ControlRigLayerInstance->RemoveTLLRN_ControlRigTrack(TLLRN_ControlRigUniqueID);
						if (!TLLRN_ControlRigLayerInstance->GetFirstAvailableTLLRN_ControlRig())
						{
							FAnimCustomInstanceHelper::UnbindFromSkeletalMeshComponent<UTLLRN_ControlRigLayerInstance>(SkeletalMeshComponent);
						}
					}
				}
			}

			FMovieSceneSequenceID SequenceID;
			uint32 TLLRN_ControlRigUniqueID;
		};


		FToken Token(SequenceID, TLLRN_ControlRigUniqueID);
		return MoveTemp(Token);
	}

	FMovieSceneSequenceID SequenceID;
	uint32 TLLRN_ControlRigUniqueID;
};

struct FTLLRN_ControlRigParameterPreAnimatedTokenProducer : IMovieScenePreAnimatedTokenProducer
{
	FTLLRN_ControlRigParameterPreAnimatedTokenProducer(FMovieSceneSequenceIDRef InSequenceID)
		: SequenceID(InSequenceID)
	{}

	virtual IMovieScenePreAnimatedTokenPtr CacheExistingState(UObject& Object) const
	{

		struct FToken : IMovieScenePreAnimatedToken
		{
			FToken(FMovieSceneSequenceIDRef InSequenceID)
				: SequenceID(InSequenceID)
			{

			}
			void SetSkelMesh(USkeletalMeshComponent* InComponent)
			{
				SkeletalMeshRestoreState.SaveState(InComponent);
				AnimationMode = InComponent->GetAnimationMode();
			}

			virtual void RestoreState(UObject& InObject, const UE::MovieScene::FRestoreStateParams& Params) override
			{

				if (UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(&InObject))
				{
					if (TLLRN_ControlRig->GetObjectBinding())
					{
						{
							// Reduce the EvaluationMutex lock to only the absolutely necessary
							// After locking, the call to UnBindFromSequencerInstance might wait for parallel evaluations to finish
							// and some control rigs might have been queued to evaluate (but haven's started yet), so they could get stuck
							// waiting on the evaluation lock.

							
							// control rig evaluate critical section: when restoring the state, we can be poking into running instances of Control Rigs
							// on the anim thread so using a lock here to avoid this thread and anim thread both touching the rig
							// at the same time, which can lead to various issues like double-freeing some random array when doing SetControlValue
							// note: the critical section accepts recursive locking so it is ok that we
							// are calling evaluate_anythread later within the same scope
							FScopeLock EvaluateLock(&TLLRN_ControlRig->GetEvaluateMutex());
						
							//Restore control rig first
							const bool bSetupUndo = false;
							if (UTLLRN_RigHierarchy* TLLRN_RigHierarchy = TLLRN_ControlRig->GetHierarchy())
							{
								FTLLRN_RigElementKey ControlKey;
								ControlKey.Type = ETLLRN_RigElementType::Control;
								for (FControlSpaceAndValue& SpaceNameAndValue : SpaceValues)
								{
									ControlKey.Name = SpaceNameAndValue.ControlName;
									switch (SpaceNameAndValue.Value.SpaceType)
									{
										case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent:
											TLLRN_ControlRig->SwitchToParent(ControlKey, TLLRN_RigHierarchy->GetDefaultParent(ControlKey), false, true);
										break;
										case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::World:
											TLLRN_ControlRig->SwitchToParent(ControlKey, TLLRN_RigHierarchy->GetWorldSpaceReferenceKey(), false, true);
										break;
										case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig:
										{
#if WITH_EDITOR
											TLLRN_ControlRig->SwitchToParent(ControlKey, SpaceNameAndValue.Value.TLLRN_ControlTLLRN_RigElement, false, true);
#else
											TLLRN_ControlRig->SwitchToParent(ControlKey, SpaceNameAndValue.Value.TLLRN_ControlTLLRN_RigElement, false, true);
#endif	
										}
										break;
									}
								}
							

								for (TNameAndValue<float>& Value : ScalarValues)
								{
									if (TLLRN_ControlRig->FindControl(Value.Name))
									{
										TLLRN_ControlRig->SetControlValue<float>(Value.Name, Value.Value, true, FTLLRN_RigControlModifiedContext(ETLLRN_ControlRigSetKey::Never), bSetupUndo);
									}
								}

								for (TNameAndValue<bool>& Value : BoolValues)
								{
									if (TLLRN_ControlRig->FindControl(Value.Name))
									{
										TLLRN_ControlRig->SetControlValue<bool>(Value.Name, Value.Value, true, FTLLRN_RigControlModifiedContext(ETLLRN_ControlRigSetKey::Never), bSetupUndo);
									}
								}

								for (TNameAndValue<int32>& Value : IntegerValues)
								{
									if (TLLRN_ControlRig->FindControl(Value.Name))
									{
										TLLRN_ControlRig->SetControlValue<int32>(Value.Name, Value.Value, true, FTLLRN_RigControlModifiedContext(ETLLRN_ControlRigSetKey::Never), bSetupUndo);
									}
								}
								for (int32 TwiceHack = 0; TwiceHack < 2; ++TwiceHack)
								{
									for (TNameAndValue<FVector2D>& Value : Vector2DValues)
									{
										if (TLLRN_ControlRig->FindControl(Value.Name))
										{
											const FVector3f Vector3(Value.Value.X, Value.Value.Y, 0.f);
											//okay to use vector3 for 2d here
											TLLRN_ControlRig->SetControlValue<FVector3f>(Value.Name, Vector3, true, FTLLRN_RigControlModifiedContext(ETLLRN_ControlRigSetKey::Never), bSetupUndo);
										}
									}

									for (TNameAndValue<FVector>& Value : VectorValues)
									{
										if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(Value.Name))
										{
											if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
											{
												TLLRN_RigHierarchy->SetControlSpecifiedEulerAngle(ControlElement, Value.Value);
											}
											TLLRN_ControlRig->SetControlValue<FVector3f>(Value.Name, (FVector3f)Value.Value, true, FTLLRN_RigControlModifiedContext(ETLLRN_ControlRigSetKey::Never), bSetupUndo);
										}
									}

									for (TNameAndValue<FEulerTransform>& Value : TransformValues)
									{
										if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(Value.Name))
										{
											switch (ControlElement->Settings.ControlType)
											{
											case ETLLRN_RigControlType::Transform:
												{
													FVector EulerAngle(Value.Value.Rotation.Roll, Value.Value.Rotation.Pitch, Value.Value.Rotation.Yaw);
													TLLRN_RigHierarchy->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
													TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FTransform_Float>(Value.Name, Value.Value.ToFTransform(), true, FTLLRN_RigControlModifiedContext(ETLLRN_ControlRigSetKey::Never), bSetupUndo);
													break;
												}
											case ETLLRN_RigControlType::TransformNoScale:
												{
													FTransformNoScale NoScale = Value.Value.ToFTransform();
													FVector EulerAngle(Value.Value.Rotation.Roll, Value.Value.Rotation.Pitch, Value.Value.Rotation.Yaw);
													TLLRN_RigHierarchy->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
													TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FTransformNoScale_Float>(Value.Name, NoScale, true, FTLLRN_RigControlModifiedContext(ETLLRN_ControlRigSetKey::Never), bSetupUndo);
													break;
												}
											case ETLLRN_RigControlType::EulerTransform:
												{
													FEulerTransform EulerTransform = Value.Value;
													FVector EulerAngle(Value.Value.Rotation.Roll, Value.Value.Rotation.Pitch, Value.Value.Rotation.Yaw);
													TLLRN_RigHierarchy->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
													TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FEulerTransform_Float>(Value.Name, EulerTransform, true, FTLLRN_RigControlModifiedContext(ETLLRN_ControlRigSetKey::Never), bSetupUndo);
													break;
												}
											default:
												{
													break;
												}
											}
										}
									}
								}
							}
							//make sure to evaluate the control rig
							TLLRN_ControlRig->Evaluate_AnyThread();
						}

						//unbind instances and reset animbp
						FTLLRN_ControlRigBindingHelper::UnBindFromSequencerInstance(TLLRN_ControlRig);

						//do a tick and restore skel mesh
						if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(TLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
						{
							// If the skel mesh comp owner has been removed from the world, no need to restore anything
							if (SkeletalMeshComponent->IsRegistered())
							{
								// Restore pose after unbinding to force the restored pose
								SkeletalMeshComponent->SetUpdateAnimationInEditor(true);
								SkeletalMeshComponent->SetUpdateClothInEditor(true);
								if (!SkeletalMeshComponent->IsPostEvaluatingAnimation())
								{
									SkeletalMeshComponent->TickAnimation(0.f, false);
									SkeletalMeshComponent->RefreshBoneTransforms();
									SkeletalMeshComponent->RefreshFollowerComponents();
									SkeletalMeshComponent->UpdateComponentToWorld();
									SkeletalMeshComponent->FinalizeBoneTransform();
									SkeletalMeshComponent->MarkRenderTransformDirty();
									SkeletalMeshComponent->MarkRenderDynamicDataDirty();
								}
								SkeletalMeshRestoreState.RestoreState(SkeletalMeshComponent);

								if (SkeletalMeshComponent->GetAnimationMode() != AnimationMode)
								{
									SkeletalMeshComponent->SetAnimationMode(AnimationMode);
								}
							}
						}
						//only unbind if not a component
						if (Cast<UTLLRN_ControlRigComponent>(TLLRN_ControlRig->GetObjectBinding()->GetBoundObject()) == nullptr)
						{
							TLLRN_ControlRig->GetObjectBinding()->UnbindFromObject();
						}

					}
				}
			}

			FMovieSceneSequenceID SequenceID;
			TArray< FControlSpaceAndValue> SpaceValues;
			TArray< TNameAndValue<float> > ScalarValues;
			TArray< TNameAndValue<bool> > BoolValues;
			TArray< TNameAndValue<int32> > IntegerValues;
			TArray< TNameAndValue<FVector> > VectorValues;
			TArray< TNameAndValue<FVector2D> > Vector2DValues;
			TArray< TNameAndValue<FEulerTransform> > TransformValues;
			FSkeletalMeshRestoreState SkeletalMeshRestoreState;
			EAnimationMode::Type AnimationMode;
		};


		FToken Token(SequenceID);

		if (UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(&Object))
		{
			UTLLRN_RigHierarchy* TLLRN_RigHierarchy = TLLRN_ControlRig->GetHierarchy();
			TArray<FTLLRN_RigControlElement*> Controls = TLLRN_ControlRig->AvailableControls();
			FTLLRN_RigControlValue Value;
			
			for (FTLLRN_RigControlElement* ControlElement : Controls)
			{
				switch (ControlElement->Settings.ControlType)
				{
					case ETLLRN_RigControlType::Bool:
					{
						const bool Val = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<bool>();
						Token.BoolValues.Add(TNameAndValue<bool>{ ControlElement->GetFName(), Val });
						break;
					}
					case ETLLRN_RigControlType::Integer:
					{
						const int32 Val = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<int32>();
						Token.IntegerValues.Add(TNameAndValue<int32>{ ControlElement->GetFName(), Val });
						break;
					}
					case ETLLRN_RigControlType::Float:
					case ETLLRN_RigControlType::ScaleFloat:
					{
						const float Val = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<float>();
						Token.ScalarValues.Add(TNameAndValue<float>{ ControlElement->GetFName(), Val });
						break;
					}
					case ETLLRN_RigControlType::Vector2D:
					{
						const FVector3f Val = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FVector3f>();
						Token.Vector2DValues.Add(TNameAndValue<FVector2D>{ ControlElement->GetFName(), FVector2D(Val.X, Val.Y) });
						break;
					}
					case ETLLRN_RigControlType::Position:
					case ETLLRN_RigControlType::Scale:
					case ETLLRN_RigControlType::Rotator:
					{
						FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey SpaceValue;
						//@helge how can we get the correct current space here? this is for restoring it.
						//for now just using parent space
						//FTLLRN_RigElementKey DefaultParent = TLLRN_RigHierarchy->GetFirstParent(ControlElement->GetKey());
						SpaceValue.TLLRN_ControlTLLRN_RigElement = ControlElement->GetKey();
						SpaceValue.SpaceType = EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent;
						Token.SpaceValues.Add(FControlSpaceAndValue(ControlElement->GetFName(), SpaceValue));
						FVector3f Val = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FVector3f>();
						if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
						{
							FVector Vector = TLLRN_ControlRig->GetControlSpecifiedEulerAngle(ControlElement);
							Val = FVector3f(Vector.X, Vector.Y, Vector.Z);
						}
						Token.VectorValues.Add(TNameAndValue<FVector>{ ControlElement->GetFName(), (FVector)Val });
						//mz todo specify rotator special so we can do quat interps
						break;
					}
					case ETLLRN_RigControlType::Transform:
					{
						FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey SpaceValue;
						//@helge how can we get the correct current space here? this is for restoring it.
						//for now just using parent space
						//FTLLRN_RigElementKey DefaultParent = TLLRN_RigHierarchy->GetFirstParent(ControlElement->GetKey());
						SpaceValue.TLLRN_ControlTLLRN_RigElement = ControlElement->GetKey();
						SpaceValue.SpaceType = EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent;
						Token.SpaceValues.Add(FControlSpaceAndValue(ControlElement->GetFName(), SpaceValue));
						const FTransform Val = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FTLLRN_RigControlValue::FTransform_Float>().ToTransform();
						FEulerTransform EulerTransform(Val);
						FVector Vector = TLLRN_ControlRig->GetControlSpecifiedEulerAngle(ControlElement);
						EulerTransform.Rotation = FRotator(Vector.Y, Vector.Z, Vector.X);
						Token.TransformValues.Add(TNameAndValue<FEulerTransform>{ ControlElement->GetFName(), EulerTransform });
						break;
					}
					case ETLLRN_RigControlType::TransformNoScale:
					{
						const FTransformNoScale NoScale = 
							TLLRN_ControlRig
							->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FTLLRN_RigControlValue::FTransformNoScale_Float>().ToTransform();
						FEulerTransform EulerTransform(NoScale.ToFTransform());
						FVector Vector = TLLRN_ControlRig->GetControlSpecifiedEulerAngle(ControlElement);
						EulerTransform.Rotation = FRotator(Vector.Y, Vector.Z, Vector.X);
						Token.TransformValues.Add(TNameAndValue<FEulerTransform>{ ControlElement->GetFName(), EulerTransform });
						break;
					}
					case ETLLRN_RigControlType::EulerTransform:
					{
						FEulerTransform EulerTransform = 
							TLLRN_ControlRig
							->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FTLLRN_RigControlValue::FEulerTransform_Float>().ToTransform();
						FVector Vector = TLLRN_ControlRig->GetControlSpecifiedEulerAngle(ControlElement);
						EulerTransform.Rotation = FRotator(Vector.Y, Vector.Z, Vector.X);
						Token.TransformValues.Add(TNameAndValue<FEulerTransform>{ ControlElement->GetFName(), EulerTransform });
						break;
					}
				}
			}

			if (TLLRN_ControlRig->GetObjectBinding())
			{
				if (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent = Cast<UTLLRN_ControlRigComponent>(TLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
				{
					if (TLLRN_ControlRigComponent->GetTLLRN_ControlRig() != TLLRN_ControlRig)
					{
						TLLRN_ControlRigComponent->Initialize();
						/*
						Previously with Sequencer and CR Components we would assign the CR to a Component 
						that the sequencer was using, in any world. This looks like it was causing issues 
						with two worlds running with pre-forward solve events so now we only do that if 
						in non-game, and if in game (which includes PIE), we don't re-set the 
						CR Component's CR, but instead grab the CR from it and then use that for evaluation.
						*/
						if (TLLRN_ControlRigComponent->GetWorld())
						{
							if (TLLRN_ControlRigComponent->GetWorld()->IsGameWorld() == false)
							{
								TLLRN_ControlRigComponent->SetTLLRN_ControlRig(TLLRN_ControlRig);
							}
						}
					}
					else
					{
						TArray<FName> SelectedControls = TLLRN_ControlRig->CurrentControlSelection();
						TLLRN_ControlRig->Initialize();
						TArray<FName> NewSelectedControls = TLLRN_ControlRig->CurrentControlSelection();
						if (SelectedControls != NewSelectedControls)
						{
							SelectControls(TLLRN_ControlRig,SelectedControls);
						}
					}
				}
				else if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(TLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
				{
					Token.SetSkelMesh(SkeletalMeshComponent);
				}
			}
		}

		return MoveTemp(Token);
	}

	FMovieSceneSequenceID SequenceID;
	TArray< FControlSpaceAndValue> SpaceValues;
	TArray< TNameAndValue<bool> > BoolValues;
	TArray< TNameAndValue<int32> > IntegerValues;
	TArray< TNameAndValue<float> > ScalarValues;
	TArray< TNameAndValue<FVector2D> > Vector2DValues;
	TArray< TNameAndValue<FVector> > VectorValues;
	TArray< TNameAndValue<FTransform> > TransformValues;

};

static UTLLRN_ControlRig* GetTLLRN_ControlRig(const UMovieSceneTLLRN_ControlRigParameterSection* Section,UObject* BoundObject)
{
	UWorld* GameWorld = (BoundObject && BoundObject->GetWorld() && BoundObject->GetWorld()->IsGameWorld()) ? BoundObject->GetWorld() : nullptr;
	UTLLRN_ControlRig* TLLRN_ControlRig =  Section->GetTLLRN_ControlRig(GameWorld);
	if (TLLRN_ControlRig && TLLRN_ControlRig->GetObjectBinding())
	{
		if (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent = Cast<UTLLRN_ControlRigComponent>(TLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
		{
			if (AActor* Actor = Cast<AActor>(BoundObject))
			{
				if (UTLLRN_ControlRigComponent* NewTLLRN_ControlRigComponent = Actor->FindComponentByClass<UTLLRN_ControlRigComponent>())
				{
					if (NewTLLRN_ControlRigComponent->GetWorld())
					{
						if (NewTLLRN_ControlRigComponent->GetWorld()->IsGameWorld())
						{
							TLLRN_ControlRig = NewTLLRN_ControlRigComponent->GetTLLRN_ControlRig();
							if (!TLLRN_ControlRig)
							{
								NewTLLRN_ControlRigComponent->Initialize();
								TLLRN_ControlRig = NewTLLRN_ControlRigComponent->GetTLLRN_ControlRig();
							}
							if (TLLRN_ControlRig)
							{
								if (TLLRN_ControlRig->GetObjectBinding() == nullptr)
								{
									TLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
								}
								if (TLLRN_ControlRig->GetObjectBinding()->GetBoundObject() != NewTLLRN_ControlRigComponent)
								{
									TLLRN_ControlRig->GetObjectBinding()->BindToObject(BoundObject);
								}
							}
						}
						else if (NewTLLRN_ControlRigComponent != TLLRN_ControlRigComponent)
						{
							NewTLLRN_ControlRigComponent->SetTLLRN_ControlRig(TLLRN_ControlRig);
						}
					}
				}
			}
		}
		else if (UTLLRN_ControlRigComponent* NewTLLRN_ControlRigComponent = Cast<UTLLRN_ControlRigComponent>(BoundObject))
		{
			if (NewTLLRN_ControlRigComponent->GetWorld())
			{
				if (NewTLLRN_ControlRigComponent->GetWorld()->IsGameWorld())
				{
					TLLRN_ControlRig = NewTLLRN_ControlRigComponent->GetTLLRN_ControlRig();
					if (!TLLRN_ControlRig)
					{
						NewTLLRN_ControlRigComponent->Initialize();
						TLLRN_ControlRig = NewTLLRN_ControlRigComponent->GetTLLRN_ControlRig();
					}
					if (TLLRN_ControlRig)
					{
						if (TLLRN_ControlRig->GetObjectBinding() == nullptr)
						{
							TLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
						}
						if (TLLRN_ControlRig->GetObjectBinding()->GetBoundObject() != NewTLLRN_ControlRigComponent)
						{
							TLLRN_ControlRig->GetObjectBinding()->BindToObject(BoundObject);
						}
					}
				}
				else if (NewTLLRN_ControlRigComponent != TLLRN_ControlRigComponent)
				{
					NewTLLRN_ControlRigComponent->SetTLLRN_ControlRig(TLLRN_ControlRig);
				}
			}
		}
	}
	else
	{
		return nullptr;
	}
	return TLLRN_ControlRig;
}

static UTickableConstraint* CreateConstraintIfNeeded(UWorld* InWorld, const FConstraintAndActiveValue& InConstraintValue, UMovieSceneTLLRN_ControlRigParameterSection* InSection)
{
	UTickableConstraint* Constraint = InConstraintValue.Constraint.Get();
	if (!Constraint) 
	{
		return nullptr;
	}
	
	// it's possible that we have it but it's not in the manager, due to manager not being saved with it (due to spawning or undo/redo).
	if (InWorld)
	{
		const FConstraintsManagerController& Controller = FConstraintsManagerController::Get(InWorld);
		if (Controller.GetConstraint(Constraint->ConstraintID) == nullptr)
		{
			Controller.AddConstraint(InConstraintValue.Constraint.Get());
			//need to reconstuct channels here.. note this is now lazy and so will recreate it next time view requests it
			//but only do it if the control rig has a valid world it may not for example in PIE
			if (InSection->GetTLLRN_ControlRig() && InSection->GetTLLRN_ControlRig()->GetWorld())
			{
				InSection->ReconstructChannelProxy();
				InSection->MarkAsChanged();
			}
		}
	}

	return Constraint;
}

/* Simple token used for non-blendables*/
struct FTLLRN_ControlRigParameterExecutionToken : IMovieSceneExecutionToken
{
	FTLLRN_ControlRigParameterExecutionToken(const UMovieSceneTLLRN_ControlRigParameterSection* InSection,
		const FEvaluatedTLLRN_ControlRigParameterSectionValues& Values)
	:	Section(InSection)
	{
		BoolValues = Values.BoolValues;
		IntegerValues = Values.IntegerValues;
		SpaceValues = Values.SpaceValues;
		ConstraintsValues = Values.ConstraintsValues;
	}
	FTLLRN_ControlRigParameterExecutionToken(FTLLRN_ControlRigParameterExecutionToken&&) = default;
	FTLLRN_ControlRigParameterExecutionToken& operator=(FTLLRN_ControlRigParameterExecutionToken&&) = default;

	// Non-copyable
	FTLLRN_ControlRigParameterExecutionToken(const FTLLRN_ControlRigParameterExecutionToken&) = delete;
	FTLLRN_ControlRigParameterExecutionToken& operator=(const FTLLRN_ControlRigParameterExecutionToken&) = delete;

	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player)
	{
		MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_TLLRN_ControlRigParameterTrack_TokenExecute)
		
		FMovieSceneSequenceID SequenceID = Operand.SequenceID;
		TArrayView<TWeakObjectPtr<>> BoundObjects = Player.FindBoundObjects(Operand);
		const UMovieSceneSequence* Sequence = Player.State.FindSequence(Operand.SequenceID);
		UTLLRN_ControlRig* TLLRN_ControlRig = nullptr;
		UObject* BoundObject = BoundObjects.Num() > 0 ? BoundObjects[0].Get() : nullptr;
		if (Sequence && BoundObject)
		{
			UWorld* GameWorld = (BoundObject->GetWorld() && BoundObject->GetWorld()->IsGameWorld()) ? BoundObject->GetWorld() : nullptr;
			TLLRN_ControlRig = Section->GetTLLRN_ControlRig(GameWorld);
			if (!TLLRN_ControlRig)
			{
				return;
			}
			if (!TLLRN_ControlRig->GetObjectBinding())
			{
				TLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
			}

			if (TLLRN_ControlRig->GetObjectBinding()->GetBoundObject() != FTLLRN_ControlRigObjectBinding::GetBindableObject(BoundObject))
			{
				TLLRN_ControlRig->GetObjectBinding()->BindToObject(BoundObject);
				TArray<FName> SelectedControls = TLLRN_ControlRig->CurrentControlSelection();
				TLLRN_ControlRig->Initialize();
				if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(FTLLRN_ControlRigObjectBinding::GetBindableObject(BoundObject)))
				{
					TLLRN_ControlRig->RequestInit();
					TLLRN_ControlRig->SetBoneInitialTransformsFromSkeletalMeshComponent(SkeletalMeshComponent, true);
					TLLRN_ControlRig->Evaluate_AnyThread();
				};
				if (GameWorld == nullptr && TLLRN_ControlRig->IsA<UFKTLLRN_ControlRig>())// mz only in editor replace Fk Control rig, will look post 29.20 to see if this really needed but want to unblock folks
				{
					UMovieSceneTLLRN_ControlRigParameterTrack* Track = Section->GetTypedOuter<UMovieSceneTLLRN_ControlRigParameterTrack>();
					if (Track)
					{
						Track->ReplaceTLLRN_ControlRig(TLLRN_ControlRig, true);
					}
				}
				TArray<FName> NewSelectedControls = TLLRN_ControlRig->CurrentControlSelection();
				if (SelectedControls != NewSelectedControls)
				{
					SelectControls(TLLRN_ControlRig, SelectedControls);
				}
			}

			// make sure to pick the correct CR instance for the  Components to bind.
			// In case of PIE + Spawnable Actor + CR component, sequencer should grab
			// CR component's CR instance for evaluation, see comment in BindToSequencerInstance
			// i.e. CR component should bind to the instance that it owns itself.
			TLLRN_ControlRig = GetTLLRN_ControlRig(Section, BoundObjects[0].Get());
			if (!TLLRN_ControlRig)
			{
				return;
			}
			// ensure that pre animated state is saved, must be done before bind
			Player.SavePreAnimatedState(*TLLRN_ControlRig, FMovieSceneTLLRN_ControlRigParameterTemplate::GetAnimTypeID(), FTLLRN_ControlRigParameterPreAnimatedTokenProducer(Operand.SequenceID));
			if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(FTLLRN_ControlRigObjectBinding::GetBindableObject(BoundObject)))
			{
				Player.SavePreAnimatedState(*SkeletalMeshComponent, FTLLRN_ControlRigSkeletalMeshComponentBindingTokenProducer::GetAnimTypeID(), FTLLRN_ControlRigSkeletalMeshComponentBindingTokenProducer(Operand.SequenceID, TLLRN_ControlRig));
			}

			FTLLRN_ControlRigBindingHelper::BindToSequencerInstance(TLLRN_ControlRig);

			if (TLLRN_ControlRig->GetObjectBinding())
			{
				if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(TLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
				{
					if (UTLLRN_ControlRigLayerInstance* AnimInstance = Cast<UTLLRN_ControlRigLayerInstance>(SkeletalMeshComponent->GetAnimInstance()))
					{
						float Weight = Section->EvaluateEasing(Context.GetTime());
						if (EnumHasAllFlags(Section->TransformMask.GetChannels(), EMovieSceneTransformChannel::Weight))
						{
							float ManualWeight = 1.f;
							Section->Weight.Evaluate(Context.GetTime(), ManualWeight);
							Weight *= ManualWeight;
						}
						FTLLRN_ControlRigIOSettings InputSettings;
						InputSettings.bUpdateCurves = true;
						InputSettings.bUpdatePose = true;
						//this is not great but assumes we have 1 absolute track that will be used for weighting
						if (Section->GetBlendType() == EMovieSceneBlendType::Absolute)
						{
							AnimInstance->UpdateTLLRN_ControlRigTrack(TLLRN_ControlRig->GetUniqueID(), Weight, InputSettings, true);
						}
					}
				}
				else
				{
					TLLRN_ControlRig = GetTLLRN_ControlRig(Section, BoundObject);
				}
			}
		}		

		//Do Bool straight up no blending
		if (Section->GetBlendType().IsValid() == false ||  Section->GetBlendType().Get() != EMovieSceneBlendType::Additive)
		{
			bool bWasDoNotKey = false;

			bWasDoNotKey = Section->GetDoNotKey();
			Section->SetDoNotKey(true);

			if (TLLRN_ControlRig)
			{
				const bool bSetupUndo = false;
				TLLRN_ControlRig->SetAbsoluteTime((float)Context.GetFrameRate().AsSeconds(Context.GetTime()));
				for (const FControlSpaceAndValue& SpaceNameAndValue : SpaceValues)
				{
					if (Section->ControlsToSet.Num() == 0 || Section->ControlsToSet.Contains(SpaceNameAndValue.ControlName))
					{
						if (UTLLRN_RigHierarchy* TLLRN_RigHierarchy = TLLRN_ControlRig->GetHierarchy())
						{
							if (FTLLRN_RigControlElement* TLLRN_RigControl = TLLRN_ControlRig->FindControl(SpaceNameAndValue.ControlName))
							{
								const FTLLRN_RigElementKey ControlKey = TLLRN_RigControl->GetKey();

								switch (SpaceNameAndValue.Value.SpaceType)
								{
									case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::Parent:
										TLLRN_ControlRig->SwitchToParent(ControlKey, TLLRN_RigHierarchy->GetDefaultParent(ControlKey), false, true);
										break;
									case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::World:
										TLLRN_ControlRig->SwitchToParent(ControlKey, TLLRN_RigHierarchy->GetWorldSpaceReferenceKey(), false, true);
										break;
									case EMovieSceneTLLRN_ControlTLLRN_RigSpaceType::TLLRN_ControlRig:
										{
#if WITH_EDITOR
											TLLRN_ControlRig->SwitchToParent(ControlKey, SpaceNameAndValue.Value.TLLRN_ControlTLLRN_RigElement, false, true);
#else
											TLLRN_ControlRig->SwitchToParent(ControlKey, SpaceNameAndValue.Value.TLLRN_ControlTLLRN_RigElement, false, true);
#endif	
										}
										break;
								}
							}
						}
						/*
						FTLLRN_RigControlElement* TLLRN_RigControl = TLLRN_ControlRig->FindControl(SpaceNameAndValue.ParameterName);
						if (TLLRN_RigControl && TLLRN_RigControl->Settings.ControlType == ETLLRN_RigControlType::Bool)
						{
							TLLRN_ControlRig->SetControlValue<bool>(SpaceNameAndValue.SpaceNameAndValue, SpaceNameAndValue.Value, true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);
						}
						*/
					}
				}


				for (const FBoolParameterStringAndValue& BoolNameAndValue : BoolValues)
				{
					if (Section->ControlsToSet.Num() == 0 || Section->ControlsToSet.Contains(BoolNameAndValue.ParameterName))
					{
						FTLLRN_RigControlElement* TLLRN_RigControl = TLLRN_ControlRig->FindControl(BoolNameAndValue.ParameterName);
						if (TLLRN_RigControl && TLLRN_RigControl->Settings.AnimationType != ETLLRN_RigControlAnimationType::ProxyControl &&
							TLLRN_RigControl->Settings.AnimationType != ETLLRN_RigControlAnimationType::VisualCue
							&& TLLRN_RigControl->Settings.ControlType == ETLLRN_RigControlType::Bool)
						{
							TLLRN_ControlRig->SetControlValue<bool>(BoolNameAndValue.ParameterName, BoolNameAndValue.Value, true, ETLLRN_ControlRigSetKey::Never,bSetupUndo);
						}
					}
				}

				for (const FIntegerParameterStringAndValue& IntegerNameAndValue : IntegerValues)
				{
					if (Section->ControlsToSet.Num() == 0 || Section->ControlsToSet.Contains(IntegerNameAndValue.ParameterName))
					{
						FTLLRN_RigControlElement* TLLRN_RigControl = TLLRN_ControlRig->FindControl(IntegerNameAndValue.ParameterName);
						if (TLLRN_RigControl && TLLRN_RigControl->Settings.AnimationType != ETLLRN_RigControlAnimationType::ProxyControl &&
							TLLRN_RigControl->Settings.AnimationType != ETLLRN_RigControlAnimationType::VisualCue
							&& TLLRN_RigControl->Settings.ControlType == ETLLRN_RigControlType::Integer)
						{
							TLLRN_ControlRig->SetControlValue<int32>(IntegerNameAndValue.ParameterName, IntegerNameAndValue.Value, true, ETLLRN_ControlRigSetKey::Never,bSetupUndo);
						}
					}
				}
				if (BoundObject)
				{
					UWorld* BoundObjectWorld = BoundObject->GetWorld();
					
					TSharedRef<UE::MovieScene::FSharedPlaybackState> SharedPlaybackState = Player.GetSharedPlaybackState();
					for (FConstraintAndActiveValue& ConstraintValue : ConstraintsValues)
					{
						UMovieSceneTLLRN_ControlRigParameterSection* NonConstSection = const_cast<UMovieSceneTLLRN_ControlRigParameterSection*>(Section);
						CreateConstraintIfNeeded(BoundObjectWorld, ConstraintValue, NonConstSection);

						if (ConstraintValue.Constraint.IsValid())
						{
							if (UTickableTransformConstraint* TransformConstraint = Cast<UTickableTransformConstraint>(ConstraintValue.Constraint))
							{
								TransformConstraint->InitConstraint(BoundObjectWorld);
							}
							ConstraintValue.Constraint->ResolveBoundObjects(Operand.SequenceID, SharedPlaybackState, TLLRN_ControlRig);
							ConstraintValue.Constraint->SetActive(ConstraintValue.Value);
						}
					}

					// for Constraints with TLLRN_ControlRig we need to resolve all Parents also
					// Don't need to do children since they wil be handled by the channel resolve above
					ResolveParentHandles(BoundObject, TLLRN_ControlRig, Operand, SharedPlaybackState);
				}
				else  //no bound object so turn off constraint
				{
					for (FConstraintAndActiveValue& ConstraintValue : ConstraintsValues)
					{
						if (ConstraintValue.Constraint.IsValid())
						{
							ConstraintValue.Constraint->SetActive(ConstraintValue.Value);
						}
					}
				}
			}
			Section->SetDoNotKey(bWasDoNotKey);
		}

	}

	void ResolveParentHandles(
		const UObject* InBoundObject, UTLLRN_ControlRig* InTLLRN_ControlRigInstance,
		const FMovieSceneEvaluationOperand& InOperand,
		const TSharedRef<UE::MovieScene::FSharedPlaybackState>& InSharedPlaybackState) const
	{
		if (!InBoundObject)
		{
			return;
		}

		UWorld* BoundObjectWorld = InBoundObject->GetWorld();
		const bool bIsGameWorld = InBoundObject->GetWorld() ? InBoundObject->GetWorld()->IsGameWorld() : false;
		
		UMovieSceneTLLRN_ControlRigParameterTrack* TLLRN_ControlRigTrack = Section->GetTypedOuter<UMovieSceneTLLRN_ControlRigParameterTrack>();

		// is this control rig a game world instance of this section's rig?
		auto WasAGameInstance = [TLLRN_ControlRigTrack](const UTLLRN_ControlRig* InRigToTest)
		{
			return TLLRN_ControlRigTrack ? TLLRN_ControlRigTrack->IsAGameInstance(InRigToTest) : false;
		};

		// is the parent handle of this constraint related to this section?
		// this return true if the handle's control rig has been spawned by the TLLRN_ControlRigTrack (whether in Editor or Game)
		// if false, it means that the handle represents another control on another control rig so we don't need to resolve it here
		// note that it returns true if TLLRN_ControlRigTrack is null (is this possible?!) or if the TLLRN_ControlRig is null (we can't infer anything from this)
		auto ShouldResolveParent = [TLLRN_ControlRigTrack](const UTLLRN_TransformableControlHandle* ParentControlHandle)
		{
			if (!ParentControlHandle)
			{
				return false;
			}

			if (!TLLRN_ControlRigTrack)
			{
				// cf. UObjectBaseUtility::IsInOuter
				return true;	
			}
			
			return ParentControlHandle->TLLRN_ControlRig ? ParentControlHandle->TLLRN_ControlRig->IsInOuter(TLLRN_ControlRigTrack) : true;
		};

		// this is the default's section rig. when bIsGameWorld is false, InTLLRN_ControlRigInstance should be equal to SectionRig
		const UTLLRN_ControlRig* SectionRig = Section->GetTLLRN_ControlRig();

		const FConstraintsManagerController& Controller = FConstraintsManagerController::Get(BoundObjectWorld);
		const TArray< TWeakObjectPtr<UTickableConstraint>> Constraints = Controller.GetAllConstraints();

		for (const TWeakObjectPtr<UTickableConstraint>& TickConstraint : Constraints)
		{
			UTickableTransformConstraint* TransformConstraint = Cast< UTickableTransformConstraint>(TickConstraint.Get());
			UTLLRN_TransformableControlHandle* ParentControlHandle = TransformConstraint ? Cast<UTLLRN_TransformableControlHandle>(TransformConstraint->ParentTRSHandle) : nullptr;
			if (ParentControlHandle && ShouldResolveParent(ParentControlHandle))
			{
				if (bIsGameWorld)
				{
					// switch from section's rig to the game instance
					if (ParentControlHandle->TLLRN_ControlRig == SectionRig)
					{
						ParentControlHandle->ResolveBoundObjects(InOperand.SequenceID, InSharedPlaybackState, InTLLRN_ControlRigInstance);
						TransformConstraint->EnsurePrimaryDependency(BoundObjectWorld);
					}
				}
				else
				{
					// switch from the game instance to the section's rig
					if (WasAGameInstance(ParentControlHandle->TLLRN_ControlRig.Get()))
					{
						ParentControlHandle->ResolveBoundObjects(InOperand.SequenceID, InSharedPlaybackState, InTLLRN_ControlRigInstance);
						TransformConstraint->EnsurePrimaryDependency(BoundObjectWorld);
					}
				}
			}
		}
	}
	
	const UMovieSceneTLLRN_ControlRigParameterSection* Section;
	/** Array of evaluated bool values */
	TArray<FBoolParameterStringAndValue, TInlineAllocator<2>> BoolValues;
	/** Array of evaluated integer values */
	TArray<FIntegerParameterStringAndValue, TInlineAllocator<2>> IntegerValues;
	/** Array of Space Values*/
	TArray<FControlSpaceAndValue, TInlineAllocator<2>> SpaceValues;
	/** Array of evaluated bool values */
	TArray<FConstraintAndActiveValue, TInlineAllocator<2>> ConstraintsValues;
};


FMovieSceneTLLRN_ControlRigParameterTemplate::FMovieSceneTLLRN_ControlRigParameterTemplate(
	const UMovieSceneTLLRN_ControlRigParameterSection& Section,
	const UMovieSceneTLLRN_ControlRigParameterTrack& Track)
	: FMovieSceneParameterSectionTemplate(Section)
	, Enums(Section.GetEnumParameterNamesAndCurves())
	, Integers(Section.GetIntegerParameterNamesAndCurves())
	, Spaces(Section.GetSpaceChannels())
	, Constraints(Section.GetConstraintsChannels())
{}



struct TTLLRN_ControlRigParameterActuatorFloat : TMovieSceneBlendingActuator<FTLLRN_ControlRigTrackTokenFloat>
{
	TTLLRN_ControlRigParameterActuatorFloat(FMovieSceneAnimTypeID& InAnimID, const FName& InParameterName, const UMovieSceneTLLRN_ControlRigParameterSection* InSection)
		: TMovieSceneBlendingActuator<FTLLRN_ControlRigTrackTokenFloat>(FMovieSceneBlendingActuatorID(InAnimID))
		, ParameterName(InParameterName)
		, SectionData(InSection)
	{}


	FTLLRN_ControlRigTrackTokenFloat RetrieveCurrentValue(UObject* InObject, IMovieScenePlayer* Player) const
	{
		const UMovieSceneTLLRN_ControlRigParameterSection* Section = SectionData.Get();

		UTLLRN_ControlRig* TLLRN_ControlRig = Section ? GetTLLRN_ControlRig(Section, InObject) : nullptr;

		if (TLLRN_ControlRig)
		{
			FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ParameterName);
			if (ControlElement && ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::ProxyControl &&
				ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::VisualCue
				&& (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Float || ControlElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat))
			{
				const float Val = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<float>();
				return FTLLRN_ControlRigTrackTokenFloat(Val);
			}

		}
		return FTLLRN_ControlRigTrackTokenFloat();
	}

	void Actuate(UObject* InObject, const FTLLRN_ControlRigTrackTokenFloat& InFinalValue, const TBlendableTokenStack<FTLLRN_ControlRigTrackTokenFloat>& OriginalStack, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player)
	{
		const UMovieSceneTLLRN_ControlRigParameterSection* Section = SectionData.Get();
		
		bool bWasDoNotKey = false;
		if (Section)
		{
			const bool bSetupUndo = false;
			bWasDoNotKey = Section->GetDoNotKey();
			Section->SetDoNotKey(true);

			UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig(Section, InObject);

			if (TLLRN_ControlRig && (Section->ControlsToSet.Num() == 0 || Section->ControlsToSet.Contains(ParameterName)))
			{
				FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ParameterName);
				if (ControlElement && ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::ProxyControl &&
					ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::VisualCue &&
					(ControlElement->Settings.ControlType == ETLLRN_RigControlType::Float || ControlElement->Settings.ControlType == ETLLRN_RigControlType::ScaleFloat))
				{
					TLLRN_ControlRig->SetControlValue<float>(ParameterName, InFinalValue.Value, true, ETLLRN_ControlRigSetKey::Never,bSetupUndo);
				}
			}

			Section->SetDoNotKey(bWasDoNotKey);
		}
	}


	virtual void Actuate(FMovieSceneInterrogationData& InterrogationData, const FTLLRN_ControlRigTrackTokenFloat& InValue, const TBlendableTokenStack<FTLLRN_ControlRigTrackTokenFloat>& OriginalStack, const FMovieSceneContext& Context) const override
	{
		FFloatInterrogationData Data;
		Data.Val = InValue.Value;
		Data.ParameterName = ParameterName;
		InterrogationData.Add(FFloatInterrogationData(Data), UMovieSceneTLLRN_ControlRigParameterSection::GetFloatInterrogationKey());
	}

	FName ParameterName;
	TWeakObjectPtr<const UMovieSceneTLLRN_ControlRigParameterSection> SectionData;

};


struct TTLLRN_ControlRigParameterActuatorVector2D : TMovieSceneBlendingActuator<FTLLRN_ControlRigTrackTokenVector2D>
{
	TTLLRN_ControlRigParameterActuatorVector2D(FMovieSceneAnimTypeID& InAnimID,  const FName& InParameterName, const UMovieSceneTLLRN_ControlRigParameterSection* InSection)
		: TMovieSceneBlendingActuator<FTLLRN_ControlRigTrackTokenVector2D>(FMovieSceneBlendingActuatorID(InAnimID))
		, ParameterName(InParameterName)
		, SectionData(InSection)
	{}



	FTLLRN_ControlRigTrackTokenVector2D RetrieveCurrentValue(UObject* InObject, IMovieScenePlayer* Player) const
	{
		const UMovieSceneTLLRN_ControlRigParameterSection* Section = SectionData.Get();

		UTLLRN_ControlRig* TLLRN_ControlRig = Section ? GetTLLRN_ControlRig(Section, InObject) : nullptr;

		if (TLLRN_ControlRig)
		{
			FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ParameterName);
			if (ControlElement && ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::ProxyControl &&
				ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::VisualCue
				&& (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Vector2D))
			{
				FVector3f Val = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FVector3f>();
				return FTLLRN_ControlRigTrackTokenVector2D(FVector2D(Val.X, Val.Y));
			}
		}
		return FTLLRN_ControlRigTrackTokenVector2D();
	}

	void Actuate(UObject* InObject, const FTLLRN_ControlRigTrackTokenVector2D& InFinalValue, const TBlendableTokenStack<FTLLRN_ControlRigTrackTokenVector2D>& OriginalStack, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player)
	{
		const UMovieSceneTLLRN_ControlRigParameterSection* Section = SectionData.Get();

		bool bWasDoNotKey = false;
		if (Section)
		{
			const bool bSetupUndo = false;
			bWasDoNotKey = Section->GetDoNotKey();
			Section->SetDoNotKey(true);

			UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig(Section, InObject);


			if (TLLRN_ControlRig && (Section->ControlsToSet.Num() == 0 || Section->ControlsToSet.Contains(ParameterName)))
			{
				FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ParameterName);
				if (ControlElement && ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::ProxyControl &&
					ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::VisualCue
					&& (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Vector2D))
				{
					const FVector3f Value(InFinalValue.Value.X, InFinalValue.Value.Y, 0.f);
					TLLRN_ControlRig->SetControlValue<FVector3f>(ParameterName, Value, true, ETLLRN_ControlRigSetKey::Never,bSetupUndo);
				}
			}

			Section->SetDoNotKey(bWasDoNotKey);
		}
	}
	virtual void Actuate(FMovieSceneInterrogationData& InterrogationData, const FTLLRN_ControlRigTrackTokenVector2D& InValue, const TBlendableTokenStack<FTLLRN_ControlRigTrackTokenVector2D>& OriginalStack, const FMovieSceneContext& Context) const override
	{
		FVector2DInterrogationData Data;
		Data.Val = InValue.Value;
		Data.ParameterName = ParameterName;
		InterrogationData.Add(FVector2DInterrogationData(Data), UMovieSceneTLLRN_ControlRigParameterSection::GetVector2DInterrogationKey());
	}

	FName ParameterName;
	TWeakObjectPtr<const UMovieSceneTLLRN_ControlRigParameterSection> SectionData;

};


struct TTLLRN_ControlRigParameterActuatorVector : TMovieSceneBlendingActuator<FTLLRN_ControlRigTrackTokenVector>
{
	TTLLRN_ControlRigParameterActuatorVector(FMovieSceneAnimTypeID& InAnimID, const FName& InParameterName, const UMovieSceneTLLRN_ControlRigParameterSection* InSection)
		: TMovieSceneBlendingActuator<FTLLRN_ControlRigTrackTokenVector>(FMovieSceneBlendingActuatorID(InAnimID))
		, ParameterName(InParameterName)
		, SectionData(InSection)
	{}



	FTLLRN_ControlRigTrackTokenVector RetrieveCurrentValue(UObject* InObject, IMovieScenePlayer* Player) const
	{
		const UMovieSceneTLLRN_ControlRigParameterSection* Section = SectionData.Get();

		UTLLRN_ControlRig* TLLRN_ControlRig = Section ? GetTLLRN_ControlRig(Section, InObject) : nullptr;

		if (TLLRN_ControlRig)
		{
			FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ParameterName);
			
			if (ControlElement && ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::ProxyControl &&
				ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::VisualCue
				&& (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Position || ControlElement->Settings.ControlType == ETLLRN_RigControlType::Scale || ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator))
			{
				FVector3f Val = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FVector3f>();
				if (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
				{
					FVector Vector = TLLRN_ControlRig->GetControlSpecifiedEulerAngle(ControlElement);
					Val = FVector3f(Vector.X, Vector.Y, Vector.Z);
				}
				return FTLLRN_ControlRigTrackTokenVector((FVector)Val);
			}
		}
		return FTLLRN_ControlRigTrackTokenVector();
	}

	void Actuate(UObject* InObject, const FTLLRN_ControlRigTrackTokenVector& InFinalValue, const TBlendableTokenStack<FTLLRN_ControlRigTrackTokenVector>& OriginalStack, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player)
	{
		const UMovieSceneTLLRN_ControlRigParameterSection* Section = SectionData.Get();
		
		bool bWasDoNotKey = false;
		if (Section)
		{
			const bool bSetupUndo = false;
			bWasDoNotKey = Section->GetDoNotKey();
			Section->SetDoNotKey(true);
			UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig(Section, InObject);

			if (TLLRN_ControlRig && (Section->ControlsToSet.Num() == 0 || Section->ControlsToSet.Contains(ParameterName)))
			{
				FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ParameterName);
				if (ControlElement && ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::ProxyControl &&
					ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::VisualCue
					&& (ControlElement->Settings.ControlType == ETLLRN_RigControlType::Position || ControlElement->Settings.ControlType == ETLLRN_RigControlType::Scale || ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator))
				{
					if (ControlElement && ControlElement->Settings.ControlType == ETLLRN_RigControlType::Rotator)
					{
						TLLRN_ControlRig->GetHierarchy()->SetControlSpecifiedEulerAngle(ControlElement, InFinalValue.Value);
					}
					TLLRN_ControlRig->SetControlValue<FVector3f>(ParameterName, (FVector3f)InFinalValue.Value, true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);
				}
			}

			Section->SetDoNotKey(bWasDoNotKey);
		}
	}
	virtual void Actuate(FMovieSceneInterrogationData& InterrogationData, const FTLLRN_ControlRigTrackTokenVector& InValue, const TBlendableTokenStack<FTLLRN_ControlRigTrackTokenVector>& OriginalStack, const FMovieSceneContext& Context) const override
	{
		FVectorInterrogationData Data;
		Data.Val = InValue.Value;
		Data.ParameterName = ParameterName;		
		InterrogationData.Add(FVectorInterrogationData(Data), UMovieSceneTLLRN_ControlRigParameterSection::GetVectorInterrogationKey());
	}


	FName ParameterName;
	TWeakObjectPtr<const UMovieSceneTLLRN_ControlRigParameterSection> SectionData;

};



struct TTLLRN_ControlRigParameterActuatorTransform : TMovieSceneBlendingActuator<FTLLRN_ControlRigTrackTokenTransform>
{
	TTLLRN_ControlRigParameterActuatorTransform(FMovieSceneAnimTypeID& InAnimID, const FName& InParameterName, const UMovieSceneTLLRN_ControlRigParameterSection* InSection)
		: TMovieSceneBlendingActuator<FTLLRN_ControlRigTrackTokenTransform>(FMovieSceneBlendingActuatorID(InAnimID))
		, ParameterName(InParameterName)
		, SectionData(InSection)
	{}

	FTLLRN_ControlRigTrackTokenTransform RetrieveCurrentValue(UObject* InObject, IMovieScenePlayer* Player) const
	{
		const UMovieSceneTLLRN_ControlRigParameterSection* Section = SectionData.Get();

		UTLLRN_ControlRig* TLLRN_ControlRig = Section ? GetTLLRN_ControlRig(Section, InObject) : nullptr;

		if (TLLRN_ControlRig && Section && (Section->ControlsToSet.Num() == 0 || Section->ControlsToSet.Contains(ParameterName)))
		{
			UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
			FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ParameterName);
			if (ControlElement && ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::ProxyControl &&
				ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::VisualCue)
			{
				if (ControlElement && ControlElement->Settings.ControlType == ETLLRN_RigControlType::Transform)
				{
					const FTransform Val = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FTLLRN_RigControlValue::FTransform_Float>().ToTransform();
					FEulerTransform EulerTransform(Val);
					FVector Vector = TLLRN_ControlRig->GetControlSpecifiedEulerAngle(ControlElement);
					EulerTransform.Rotation = FRotator(Vector.Y, Vector.Z, Vector.X);
					return FTLLRN_ControlRigTrackTokenTransform(EulerTransform);
				}
				else if (ControlElement && ControlElement->Settings.ControlType == ETLLRN_RigControlType::TransformNoScale)
				{
					FTransformNoScale ValNoScale = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FTLLRN_RigControlValue::FTransformNoScale_Float>().ToTransform();
					FTransform Val = ValNoScale;
					FEulerTransform EulerTransform(Val);
					FVector Vector = TLLRN_ControlRig->GetControlSpecifiedEulerAngle(ControlElement);
					EulerTransform.Rotation = FRotator(Vector.Y, Vector.Z, Vector.X);
					return FTLLRN_ControlRigTrackTokenTransform(EulerTransform);
				}
				else if (ControlElement && ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
				{
					FEulerTransform EulerTransform = TLLRN_ControlRig->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current).Get<FTLLRN_RigControlValue::FEulerTransform_Float>().ToTransform();
					FVector Vector = TLLRN_ControlRig->GetControlSpecifiedEulerAngle(ControlElement);
					EulerTransform.Rotation = FRotator(Vector.Y, Vector.Z, Vector.X);
					return FTLLRN_ControlRigTrackTokenTransform(EulerTransform);
				}
			}
		}
		return FTLLRN_ControlRigTrackTokenTransform();
	}

	void Actuate(UObject* InObject, const FTLLRN_ControlRigTrackTokenTransform& InFinalValue, const TBlendableTokenStack<FTLLRN_ControlRigTrackTokenTransform>& OriginalStack, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player)
	{
		const UMovieSceneTLLRN_ControlRigParameterSection* Section = SectionData.Get();
		if (Section)
		{
			UMovieSceneTrack * Track = Cast<UMovieSceneTrack>(Section->GetOuter());
			if (Track && Track->GetSectionToKey())
			{
				Section = Cast<const UMovieSceneTLLRN_ControlRigParameterSection>(Track->GetSectionToKey());
			}
		}

		bool bWasDoNotKey = false;
		if (Section)
		{
			bWasDoNotKey = Section->GetDoNotKey();
			Section->SetDoNotKey(true);
			const bool bSetupUndo = false;
			UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig(Section, InObject);

			if (TLLRN_ControlRig && (Section->ControlsToSet.Num() == 0 || Section->ControlsToSet.Contains(ParameterName)))
			{
				UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
				FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ParameterName);
				if (ControlElement && ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::ProxyControl &&
					ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::VisualCue)
				{
					if (ControlElement && ControlElement->Settings.ControlType == ETLLRN_RigControlType::Transform)
					{
						FVector EulerAngle(InFinalValue.Value.Rotation.Roll, InFinalValue.Value.Rotation.Pitch, InFinalValue.Value.Rotation.Yaw);
						Hierarchy->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
						TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FTransform_Float>(ParameterName, InFinalValue.Value.ToFTransform(), true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);
					}
					else if (ControlElement && ControlElement->Settings.ControlType == ETLLRN_RigControlType::TransformNoScale)
					{
						const FTransformNoScale NoScale = InFinalValue.Value.ToFTransform();
						FVector EulerAngle(InFinalValue.Value.Rotation.Roll, InFinalValue.Value.Rotation.Pitch, InFinalValue.Value.Rotation.Yaw);
						Hierarchy->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
						TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FTransformNoScale_Float>(ParameterName, NoScale, true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);
					}
					else if (ControlElement && ControlElement->Settings.ControlType == ETLLRN_RigControlType::EulerTransform)
					{
						FVector EulerAngle(InFinalValue.Value.Rotation.Roll, InFinalValue.Value.Rotation.Pitch, InFinalValue.Value.Rotation.Yaw);
						FQuat Quat = Hierarchy->GetControlQuaternion(ControlElement, EulerAngle);
						Hierarchy->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
						FRotator UERotator(Quat);
						FEulerTransform Transform = InFinalValue.Value;
						Transform.Rotation = UERotator;
						TLLRN_ControlRig->SetControlValue<FTLLRN_RigControlValue::FEulerTransform_Float>(ParameterName, Transform, true, ETLLRN_ControlRigSetKey::Never, bSetupUndo);	
						Hierarchy->SetControlSpecifiedEulerAngle(ControlElement, EulerAngle);
					}
				}
			}

			Section->SetDoNotKey(bWasDoNotKey);
		}
	}
	virtual void Actuate(FMovieSceneInterrogationData& InterrogationData, const FTLLRN_ControlRigTrackTokenTransform& InValue, const TBlendableTokenStack<FTLLRN_ControlRigTrackTokenTransform>& OriginalStack, const FMovieSceneContext& Context) const override
	{
		FEulerTransformInterrogationData Data;
		Data.Val = InValue.Value;
		Data.ParameterName = ParameterName;
		InterrogationData.Add(FEulerTransformInterrogationData(Data), UMovieSceneTLLRN_ControlRigParameterSection::GetTransformInterrogationKey());
	}
	FName ParameterName;
	TWeakObjectPtr<const UMovieSceneTLLRN_ControlRigParameterSection> SectionData;

};

void FMovieSceneTLLRN_ControlRigParameterTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	const FFrameTime Time = Context.GetTime();

	const UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(GetSourceSection());
	if (Section && Section->GetTLLRN_ControlRig())
	{
		UTLLRN_ControlRig* TLLRN_ControlRig = Section->GetTLLRN_ControlRig(); //this will be default(editor) control rig
		if (Operand.ObjectBindingID.IsValid())
		{
			TArrayView<TWeakObjectPtr<>> BoundObjects = PersistentData.GetMovieScenePlayer().FindBoundObjects(Operand);
			if(BoundObjects.Num() > 0 && BoundObjects[0].IsValid() && BoundObjects[0].Get()->GetWorld() && BoundObjects[0].Get()->GetWorld()->IsGameWorld()) //just support one bound object per control rig
			{
				UWorld* GameWorld =  BoundObjects[0].Get()->GetWorld();
				TLLRN_ControlRig = Section->GetTLLRN_ControlRig(GameWorld);
			}
		}

		FEvaluatedTLLRN_ControlRigParameterSectionChannelMasks* ChannelMasks = PersistentData.FindSectionData<FEvaluatedTLLRN_ControlRigParameterSectionChannelMasks>();
		if (!ChannelMasks)
		{
			// Naughty const_cast here, but we can't create this inside Initialize because of hotfix restrictions
			// The cast is ok because we actually do not have any threading involved
			ChannelMasks = &const_cast<FPersistentEvaluationData&>(PersistentData).GetOrAddSectionData<FEvaluatedTLLRN_ControlRigParameterSectionChannelMasks>();
			UMovieSceneTLLRN_ControlRigParameterSection* NonConstSection = const_cast<UMovieSceneTLLRN_ControlRigParameterSection*>(Section);
			ChannelMasks->Initialize(NonConstSection, Scalars, Bools, Integers, Enums, Vector2Ds, Vectors, Colors, Transforms);
		}

		UMovieSceneTrack* Track = Cast<UMovieSceneTrack>(Section->GetOuter());
		if (!Track)
		{
			return;
		}

		int32 BlendingOrder = Section->GetBlendingOrder();

		//Do blended tokens
		FEvaluatedTLLRN_ControlRigParameterSectionValues Values;

		EvaluateCurvesWithMasks(Context, *ChannelMasks, Values);

		float Weight = EvaluateEasing(Context.GetTime());
		if (EnumHasAllFlags(Section->TransformMask.GetChannels(), EMovieSceneTransformChannel::Weight))
		{
			float ManualWeight = 1.f;
			Section->Weight.Evaluate(Context.GetTime(), ManualWeight);
			Weight *= ManualWeight;
		}

		//Do basic token
		FTLLRN_ControlRigParameterExecutionToken ExecutionToken(Section,Values);
		ExecutionTokens.Add(MoveTemp(ExecutionToken));

		EMovieSceneBlendType BlendType = Section->GetBlendType().IsValid() ? Section->GetBlendType().Get() : EMovieSceneBlendType::Absolute;

		FTLLRN_ControlRigAnimTypeIDsPtr TypeIDs = FTLLRN_ControlRigAnimTypeIDs::Get(TLLRN_ControlRig);

		for (const FScalarParameterStringAndValue& ScalarNameAndValue : Values.ScalarValues)
		{
			if (Section->GetControlNameMask(ScalarNameAndValue.ParameterName) == false)
			{
				continue;
			}
			FMovieSceneAnimTypeID AnimTypeID = TypeIDs->FindScalar(ScalarNameAndValue.ParameterName);
			FMovieSceneBlendingActuatorID ActuatorTypeID(AnimTypeID);

			if (!ExecutionTokens.GetBlendingAccumulator().FindActuator< FTLLRN_ControlRigTrackTokenFloat>(ActuatorTypeID))
			{
				ExecutionTokens.GetBlendingAccumulator().DefineActuator(ActuatorTypeID, MakeShared <TTLLRN_ControlRigParameterActuatorFloat>(AnimTypeID, ScalarNameAndValue.ParameterName, Section));
			}
			ExecutionTokens.BlendToken(ActuatorTypeID, TBlendableToken<FTLLRN_ControlRigTrackTokenFloat>(ScalarNameAndValue.Value, BlendType, Weight, BlendingOrder));
		}

		UE::MovieScene::TMultiChannelValue<float, 3> VectorData;
		for (const FVectorParameterStringAndValue& VectorNameAndValue : Values.VectorValues)
		{
			if (Section->GetControlNameMask(VectorNameAndValue.ParameterName) == false)
			{
				continue;
			}
			FMovieSceneAnimTypeID AnimTypeID = TypeIDs->FindVector(VectorNameAndValue.ParameterName);
			FMovieSceneBlendingActuatorID ActuatorTypeID(AnimTypeID);

			if (!ExecutionTokens.GetBlendingAccumulator().FindActuator< FTLLRN_ControlRigTrackTokenVector>(ActuatorTypeID))
			{
				ExecutionTokens.GetBlendingAccumulator().DefineActuator(ActuatorTypeID, MakeShared <TTLLRN_ControlRigParameterActuatorVector>(AnimTypeID,  VectorNameAndValue.ParameterName,Section));
			}
			VectorData.Set(0, VectorNameAndValue.Value.X);
			VectorData.Set(1, VectorNameAndValue.Value.Y);
			VectorData.Set(2, VectorNameAndValue.Value.Z);

			ExecutionTokens.BlendToken(ActuatorTypeID, TBlendableToken<FTLLRN_ControlRigTrackTokenVector>(VectorData, BlendType, Weight, BlendingOrder));
		}

		UE::MovieScene::TMultiChannelValue<float, 2> Vector2DData;
		for (const FVector2DParameterStringAndValue& Vector2DNameAndValue : Values.Vector2DValues)
		{
			if (Section->GetControlNameMask(Vector2DNameAndValue.ParameterName) == false)
			{
				continue;
			}
			FMovieSceneAnimTypeID AnimTypeID = TypeIDs->FindVector2D(Vector2DNameAndValue.ParameterName);
			FMovieSceneBlendingActuatorID ActuatorTypeID(AnimTypeID);

			if (!ExecutionTokens.GetBlendingAccumulator().FindActuator< FTLLRN_ControlRigTrackTokenVector2D>(ActuatorTypeID))
			{
				ExecutionTokens.GetBlendingAccumulator().DefineActuator(ActuatorTypeID, MakeShared <TTLLRN_ControlRigParameterActuatorVector2D>(AnimTypeID,  Vector2DNameAndValue.ParameterName, Section));
			}
			Vector2DData.Set(0, Vector2DNameAndValue.Value.X);
			Vector2DData.Set(1, Vector2DNameAndValue.Value.Y);

			ExecutionTokens.BlendToken(ActuatorTypeID, TBlendableToken<FTLLRN_ControlRigTrackTokenVector2D>(Vector2DData, BlendType, Weight, BlendingOrder));
		}

		UE::MovieScene::TMultiChannelValue<float, 9> TransformData;
		for (const FEulerTransformParameterStringAndValue& TransformNameAndValue : Values.TransformValues)
		{
			if (Section->GetControlNameMask(TransformNameAndValue.ParameterName) == false)
			{
				continue;
			}
			FMovieSceneAnimTypeID AnimTypeID = TypeIDs->FindTransform(TransformNameAndValue.ParameterName);
			FMovieSceneBlendingActuatorID ActuatorTypeID(AnimTypeID);

			if (!ExecutionTokens.GetBlendingAccumulator().FindActuator< FTLLRN_ControlRigTrackTokenTransform>(ActuatorTypeID))
			{
				ExecutionTokens.GetBlendingAccumulator().DefineActuator(ActuatorTypeID, MakeShared <TTLLRN_ControlRigParameterActuatorTransform>(AnimTypeID,  TransformNameAndValue.ParameterName, Section));
			}

			const FEulerTransform& Transform = TransformNameAndValue.Transform;

			TransformData.Set(0, Transform.Location.X);
			TransformData.Set(1, Transform.Location.Y);
			TransformData.Set(2, Transform.Location.Z);

			TransformData.Set(3, Transform.Rotation.Roll);
			TransformData.Set(4, Transform.Rotation.Pitch);
			TransformData.Set(5, Transform.Rotation.Yaw);

			TransformData.Set(6, Transform.Scale.X);
			TransformData.Set(7, Transform.Scale.Y);
			TransformData.Set(8, Transform.Scale.Z);
			ExecutionTokens.BlendToken(ActuatorTypeID, TBlendableToken<FTLLRN_ControlRigTrackTokenTransform>(TransformData, BlendType, Weight, BlendingOrder));
		}

	}
}


void FMovieSceneTLLRN_ControlRigParameterTemplate::EvaluateCurvesWithMasks(const FMovieSceneContext& Context, FEvaluatedTLLRN_ControlRigParameterSectionChannelMasks& InMasks, FEvaluatedTLLRN_ControlRigParameterSectionValues& Values) const
{
	const FFrameTime Time = Context.GetTime();

	const UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(GetSourceSection());
	if (Section)
	{
		// Reserve the value arrays to avoid re-allocation
		Values.ScalarValues.Reserve(Scalars.Num());
		Values.BoolValues.Reserve(Bools.Num());
		Values.SpaceValues.Reserve(Spaces.Num());
		Values.IntegerValues.Reserve(Integers.Num() + Enums.Num()); // Both enums and integers output to the integer value array
		Values.Vector2DValues.Reserve(Vector2Ds.Num());
		Values.VectorValues.Reserve(Vectors.Num());
		Values.ColorValues.Reserve(Colors.Num());
		Values.TransformValues.Reserve(Transforms.Num());
		Values.ConstraintsValues.Reserve(Constraints.Num());

		const bool bIsAdditive = (Section->GetBlendType().IsValid() && Section->GetBlendType().Get() == EMovieSceneBlendType::Additive);
		const bool bIsAbsolute = (Section->GetBlendType().IsValid() && Section->GetBlendType().Get() == EMovieSceneBlendType::Absolute);

		// Populate each of the output arrays in turn
		for (int32 Index = 0; Index < Scalars.Num(); ++Index)
		{
			const FScalarParameterNameAndCurve& Scalar = this->Scalars[Index];
			float Value = 0;

			if (InMasks.ScalarCurveMask[Index])
			{
				Scalar.ParameterCurve.Evaluate(Time, Value);
			}
			else
			{
				Value = (!bIsAbsolute
					|| Scalar.ParameterCurve.GetDefault().IsSet() == false) ? 0.0f :
					Scalar.ParameterCurve.GetDefault().GetValue();
			}

			Values.ScalarValues.Emplace(Scalar.ParameterName, Value);
		}

		// when playing animation, instead of scrubbing/stepping thru frames, InTime might have a subframe of 0.999928
		// leading to a decimal value of 24399.999928 (for example). This results in evaluating one frame less than expected
		// (24399 instead of 24400) and leads to spaces and constraints switching parents/state after the control changes
		// its transform. (float/double channels will interpolate to a value pretty close to the one at 24400 as its based
		// on that 0.999928 subframe value.
		const FFrameTime RoundTime = Time.RoundToFrame();
		for (int32 Index = 0; Index < Spaces.Num(); ++Index)
		{
			FMovieSceneTLLRN_ControlTLLRN_RigSpaceBaseKey Value;
			const FTLLRN_SpaceControlNameAndChannel& Space = Spaces[Index];
			Space.SpaceCurve.Evaluate(RoundTime, Value);
			
			Values.SpaceValues.Emplace(Space.ControlName, Value);
		}

		for (int32 Index = 0; Index < Constraints.Num(); ++Index)
		{
			bool Value = false;
			const FConstraintAndActiveChannel& ConstraintAndActiveChannel = Constraints[Index];
			ConstraintAndActiveChannel.ActiveChannel.Evaluate(RoundTime, Value);
			Values.ConstraintsValues.Emplace(ConstraintAndActiveChannel.GetConstraint().Get(), Value);
		}
		
		for (int32 Index = 0; Index < Bools.Num(); ++Index)
		{
			bool Value = false;
			const FBoolParameterNameAndCurve& Bool = Bools[Index];
			if (InMasks.BoolCurveMask[Index])
			{
				Bool.ParameterCurve.Evaluate(Time, Value);
			}
			else
			{
				Value = (!bIsAbsolute
					|| Bool.ParameterCurve.GetDefault().IsSet() == false) ? false :
					Bool.ParameterCurve.GetDefault().GetValue();
			}

			Values.BoolValues.Emplace(Bool.ParameterName, Value);
		}
		for (int32 Index = 0; Index < Integers.Num(); ++Index)
		{
			int32  Value = 0;
			const FTLLRN_IntegerParameterNameAndCurve& Integer = Integers[Index];
			if (InMasks.IntegerCurveMask[Index])
			{
				Integer.ParameterCurve.Evaluate(Time, Value);
			}
			else
			{
				Value = (!bIsAbsolute
					|| Integer.ParameterCurve.GetDefault().IsSet() == false) ? 0 :
					Integer.ParameterCurve.GetDefault().GetValue();
			}

			Values.IntegerValues.Emplace(Integer.ParameterName, Value);
		}
		for (int32 Index = 0; Index < Enums.Num(); ++Index)
		{
			uint8  Value = 0;
			const FTLLRN_EnumParameterNameAndCurve& Enum = Enums[Index];
			if (InMasks.EnumCurveMask[Index])
			{
				Enum.ParameterCurve.Evaluate(Time, Value);
			}
			else
			{
				Value = (!bIsAbsolute
					|| Enum.ParameterCurve.GetDefault().IsSet() == false) ? 0 :
					Enum.ParameterCurve.GetDefault().GetValue();

			}
			Values.IntegerValues.Emplace(Enum.ParameterName, (int32)Value);

		}
		for (int32 Index = 0; Index < Vector2Ds.Num(); ++Index)
		{
			FVector2f Value(ForceInitToZero);
			const FVector2DParameterNameAndCurves& Vector2D = Vector2Ds[Index];

			if (InMasks.Vector2DCurveMask[Index])
			{
				Vector2D.XCurve.Evaluate(Time, Value.X);
				Vector2D.YCurve.Evaluate(Time, Value.Y);
			}
			else
			{
				if (bIsAbsolute)
				{
					if (Vector2D.XCurve.GetDefault().IsSet())
					{
						Value.X = Vector2D.XCurve.GetDefault().GetValue();
					}
					if (Vector2D.YCurve.GetDefault().IsSet())
					{
						Value.Y = Vector2D.YCurve.GetDefault().GetValue();
					}
				}
			}

			Values.Vector2DValues.Emplace(Vector2D.ParameterName, FVector2D(Value));
		}

		for (int32 Index = 0; Index < Vectors.Num(); ++Index)
		{
			FVector3f Value(ForceInitToZero);
			const FVectorParameterNameAndCurves& Vector = Vectors[Index];

			if (InMasks.VectorCurveMask[Index])
			{
				Vector.XCurve.Evaluate(Time, Value.X);
				Vector.YCurve.Evaluate(Time, Value.Y);
				Vector.ZCurve.Evaluate(Time, Value.Z);
			}
			else
			{
				if (bIsAbsolute)
				{
					if (Vector.XCurve.GetDefault().IsSet())
					{
						Value.X = Vector.XCurve.GetDefault().GetValue();
					}
					if (Vector.YCurve.GetDefault().IsSet())
					{
						Value.Y = Vector.YCurve.GetDefault().GetValue();
					}
					if (Vector.ZCurve.GetDefault().IsSet())
					{
						Value.Z = Vector.ZCurve.GetDefault().GetValue();
					}
				}
			}

			Values.VectorValues.Emplace(Vector.ParameterName, (FVector)Value);
		}
		for (int32 Index = 0; Index < Colors.Num(); ++Index)
		{
			FLinearColor ColorValue = FLinearColor::White;
			const FColorParameterNameAndCurves& Color = Colors[Index];
			if (InMasks.ColorCurveMask[Index])
			{
				Color.RedCurve.Evaluate(Time, ColorValue.R);
				Color.GreenCurve.Evaluate(Time, ColorValue.G);
				Color.BlueCurve.Evaluate(Time, ColorValue.B);
				Color.AlphaCurve.Evaluate(Time, ColorValue.A);
			}
			else
			{
				if (bIsAbsolute)
				{
					if (Color.RedCurve.GetDefault().IsSet())
					{
						ColorValue.R = Color.RedCurve.GetDefault().GetValue();
					}
					if (Color.GreenCurve.GetDefault().IsSet())
					{
						ColorValue.G = Color.GreenCurve.GetDefault().GetValue();
					}
					if (Color.BlueCurve.GetDefault().IsSet())
					{
						ColorValue.B = Color.BlueCurve.GetDefault().GetValue();
					}
					if (Color.AlphaCurve.GetDefault().IsSet())
					{
						ColorValue.A = Color.AlphaCurve.GetDefault().GetValue();
					}
				}
			}

			Values.ColorValues.Emplace(Color.ParameterName, ColorValue);
		}
		EMovieSceneTransformChannel ChannelMask = Section->GetTransformMask().GetChannels();
		for (int32 Index = 0; Index < Transforms.Num(); ++Index)
		{
			FVector3f Translation(ForceInitToZero), Scale(FVector3f::OneVector);
			if (bIsAdditive)
			{
				Scale = FVector3f(0.0f, 0.0f, 0.0f);
			}

			FRotator3f Rotator(0.0f, 0.0f, 0.0f);

			const FTransformParameterNameAndCurves& Transform = Transforms[Index];
			if (InMasks.TransformCurveMask[Index])
			{
				if (EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::TranslationX))
				{
					Transform.Translation[0].Evaluate(Time, Translation[0]);
				}
				else
				{
					if (bIsAbsolute && Transform.Translation[0].GetDefault().IsSet())
					{
						Translation[0] = Transform.Translation[0].GetDefault().GetValue();
					}
				}
				if (EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::TranslationY))
				{
					Transform.Translation[1].Evaluate(Time, Translation[1]);
				}
				else
				{
					if (bIsAbsolute && Transform.Translation[1].GetDefault().IsSet())
					{
						Translation[1] = Transform.Translation[1].GetDefault().GetValue();
					}
				}
				if (EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::TranslationZ))
				{
					Transform.Translation[2].Evaluate(Time, Translation[2]);
				}
				else
				{
					if (bIsAbsolute && Transform.Translation[2].GetDefault().IsSet())
					{
						Translation[2] = Transform.Translation[2].GetDefault().GetValue();
					}
				}
				if (EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::RotationX))
				{
					Transform.Rotation[0].Evaluate(Time, Rotator.Roll);
				}
				else
				{
					if (bIsAbsolute && Transform.Rotation[0].GetDefault().IsSet())
					{
						Rotator.Roll = Transform.Rotation[0].GetDefault().GetValue();
					}
				}
				if (EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::RotationY))
				{
					Transform.Rotation[1].Evaluate(Time, Rotator.Pitch);
				}
				else
				{
					if (bIsAbsolute && Transform.Rotation[1].GetDefault().IsSet())
					{
						Rotator.Pitch = Transform.Rotation[1].GetDefault().GetValue();
					}
				}
				if (EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::RotationZ))
				{
					Transform.Rotation[2].Evaluate(Time, Rotator.Yaw);
				}
				else
				{
					if (bIsAbsolute && Transform.Rotation[2].GetDefault().IsSet())
					{
						Rotator.Yaw = Transform.Rotation[2].GetDefault().GetValue();
					}
				}
				//mz todo quat interp...
				if (EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::ScaleX))
				{
					Transform.Scale[0].Evaluate(Time, Scale[0]);
				}
				else
				{
					if (bIsAbsolute && Transform.Scale[0].GetDefault().IsSet())
					{
						Scale[0] = Transform.Scale[0].GetDefault().GetValue();
					}
				}
				if (EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::ScaleY))
				{
					Transform.Scale[1].Evaluate(Time, Scale[1]);
				}
				else
				{
					if (bIsAbsolute && Transform.Scale[1].GetDefault().IsSet())
					{
						Scale[1] = Transform.Scale[1].GetDefault().GetValue();
					}
				}
				if (EnumHasAllFlags(ChannelMask, EMovieSceneTransformChannel::ScaleZ))
				{
					Transform.Scale[2].Evaluate(Time, Scale[2]);
				}
				else
				{
					if (bIsAbsolute && Transform.Scale[2].GetDefault().IsSet())
					{
						Scale[2] = Transform.Scale[2].GetDefault().GetValue();
					}
				}
			}
			else //completely masked use default or zeroed, which is already set if additive
			{
				if (bIsAbsolute)
				{

					if (Transform.Translation[0].GetDefault().IsSet())
					{
						Translation[0] = Transform.Translation[0].GetDefault().GetValue();
					}
					if (Transform.Translation[1].GetDefault().IsSet())
					{
						Translation[1] = Transform.Translation[1].GetDefault().GetValue();
					}
					if (Transform.Translation[2].GetDefault().IsSet())
					{
						Translation[2] = Transform.Translation[2].GetDefault().GetValue();
					}

					if (Transform.Rotation[0].GetDefault().IsSet())
					{
						Rotator.Roll = Transform.Rotation[0].GetDefault().GetValue();
					}

					if (Transform.Rotation[1].GetDefault().IsSet())
					{
						Rotator.Pitch = Transform.Rotation[1].GetDefault().GetValue();
					}

					if (Transform.Rotation[2].GetDefault().IsSet())
					{
						Rotator.Yaw = Transform.Rotation[2].GetDefault().GetValue();
					}

					if (Transform.Scale[0].GetDefault().IsSet())
					{
						Scale[0] = Transform.Scale[0].GetDefault().GetValue();
					}

					if (Transform.Scale[1].GetDefault().IsSet())
					{
						Scale[1] = Transform.Scale[1].GetDefault().GetValue();
					}

					if (Transform.Scale[2].GetDefault().IsSet())
					{
						Scale[2] = Transform.Scale[2].GetDefault().GetValue();
					}
				}
			}
			FEulerTransformParameterStringAndValue NameAndValue(Transform.ParameterName, FEulerTransform(FRotator(Rotator), (FVector)Translation, (FVector)Scale));
			Values.TransformValues.Emplace(NameAndValue);
		}
	}
}


FMovieSceneAnimTypeID FMovieSceneTLLRN_ControlRigParameterTemplate::GetAnimTypeID()
{
	return TMovieSceneAnimTypeID<FMovieSceneTLLRN_ControlRigParameterTemplate>();
}


void FMovieSceneTLLRN_ControlRigParameterTemplate::Interrogate(const FMovieSceneContext& Context, FMovieSceneInterrogationData& Container, UObject* BindingOverride) const
{
	MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_TLLRN_ControlRigTemplateParameter_Evaluate)

	const FFrameTime Time = Context.GetTime();

	const UMovieSceneTLLRN_ControlRigParameterSection* Section = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(GetSourceSection());
	if (Section && Section->GetTLLRN_ControlRig() && MovieSceneHelpers::IsSectionKeyable(Section))
	{
		FEvaluatedTLLRN_ControlRigParameterSectionChannelMasks ChannelMasks;
		UMovieSceneTLLRN_ControlRigParameterSection* NonConstSection = const_cast<UMovieSceneTLLRN_ControlRigParameterSection*>(Section);
		ChannelMasks.Initialize(NonConstSection, Scalars, Bools, Integers, Enums, Vector2Ds, Vectors, Colors, Transforms);

		//Do blended tokens
		FEvaluatedTLLRN_ControlRigParameterSectionValues Values;
		EvaluateCurvesWithMasks(Context, ChannelMasks, Values);

		FTLLRN_ControlRigAnimTypeIDsPtr TypeIDs = FTLLRN_ControlRigAnimTypeIDs::Get(Section->GetTLLRN_ControlRig());
		EMovieSceneBlendType BlendType = Section->GetBlendType().IsValid() ? Section->GetBlendType().Get() : EMovieSceneBlendType::Absolute;
		int32 BlendingOrder = Section->GetBlendingOrder();

		float Weight = EvaluateEasing(Context.GetTime());
		if (EnumHasAllFlags(Section->TransformMask.GetChannels(), EMovieSceneTransformChannel::Weight))
		{
			float ManualWeight = 1.f;
			Section->Weight.Evaluate(Context.GetTime(), ManualWeight);
			Weight *= ManualWeight;
		}

		for (const FScalarParameterStringAndValue& ScalarNameAndValue : Values.ScalarValues)
		{
			if (Section->GetControlNameMask(ScalarNameAndValue.ParameterName) == false)
			{
				continue;
			}
			FMovieSceneAnimTypeID AnimTypeID = TypeIDs->FindScalar(ScalarNameAndValue.ParameterName);
			FMovieSceneBlendingActuatorID ActuatorTypeID(AnimTypeID);

			if (!Container.GetAccumulator().FindActuator< FTLLRN_ControlRigTrackTokenFloat>(ActuatorTypeID))
			{
				Container.GetAccumulator().DefineActuator(ActuatorTypeID, MakeShared <TTLLRN_ControlRigParameterActuatorFloat>(AnimTypeID, ScalarNameAndValue.ParameterName, Section));
			}
			Container.GetAccumulator().BlendToken(FMovieSceneEvaluationOperand(), ActuatorTypeID, FMovieSceneEvaluationScope(), Context,TBlendableToken<FTLLRN_ControlRigTrackTokenFloat>(ScalarNameAndValue.Value, BlendType, Weight, BlendingOrder));
		}

		UE::MovieScene::TMultiChannelValue<float, 2> Vector2DData;
		for (const FVector2DParameterStringAndValue& Vector2DNameAndValue : Values.Vector2DValues)
		{
			if (Section->GetControlNameMask(Vector2DNameAndValue.ParameterName) == false)
			{
				continue;
			}
			FMovieSceneAnimTypeID AnimTypeID = TypeIDs->FindVector2D(Vector2DNameAndValue.ParameterName);
			FMovieSceneBlendingActuatorID ActuatorTypeID(AnimTypeID);

			if (!Container.GetAccumulator().FindActuator< FTLLRN_ControlRigTrackTokenVector>(ActuatorTypeID))
			{
				Container.GetAccumulator().DefineActuator(ActuatorTypeID, MakeShared <TTLLRN_ControlRigParameterActuatorVector2D>(AnimTypeID, Vector2DNameAndValue.ParameterName, Section));
			}
			Vector2DData.Set(0, Vector2DNameAndValue.Value.X);
			Vector2DData.Set(1, Vector2DNameAndValue.Value.Y);

			Container.GetAccumulator().BlendToken(FMovieSceneEvaluationOperand(), ActuatorTypeID, FMovieSceneEvaluationScope(), Context, TBlendableToken<FTLLRN_ControlRigTrackTokenVector2D>(Vector2DData, BlendType, Weight, BlendingOrder));
		}

		UE::MovieScene::TMultiChannelValue<float, 3> VectorData;
		for (const FVectorParameterStringAndValue& VectorNameAndValue : Values.VectorValues)
		{
			if (Section->GetControlNameMask(VectorNameAndValue.ParameterName) == false)
			{
				continue;
			}
			FMovieSceneAnimTypeID AnimTypeID = TypeIDs->FindVector(VectorNameAndValue.ParameterName);
			FMovieSceneBlendingActuatorID ActuatorTypeID(AnimTypeID);

			if (!Container.GetAccumulator().FindActuator< FTLLRN_ControlRigTrackTokenVector>(ActuatorTypeID))
			{
				Container.GetAccumulator().DefineActuator(ActuatorTypeID, MakeShared <TTLLRN_ControlRigParameterActuatorVector>(AnimTypeID, VectorNameAndValue.ParameterName, Section));
			}
			VectorData.Set(0, VectorNameAndValue.Value.X);
			VectorData.Set(1, VectorNameAndValue.Value.Y);
			VectorData.Set(2, VectorNameAndValue.Value.Z);

			Container.GetAccumulator().BlendToken(FMovieSceneEvaluationOperand(), ActuatorTypeID, FMovieSceneEvaluationScope(),Context,TBlendableToken<FTLLRN_ControlRigTrackTokenVector>(VectorData, BlendType, Weight, BlendingOrder));
		}

		UE::MovieScene::TMultiChannelValue<float, 9> TransformData;
		for (const FEulerTransformParameterStringAndValue& TransformNameAndValue : Values.TransformValues)
		{
			if (Section->GetControlNameMask(TransformNameAndValue.ParameterName) == false)
			{
				continue;
			}
			FMovieSceneAnimTypeID AnimTypeID = TypeIDs->FindTransform(TransformNameAndValue.ParameterName);
			FMovieSceneBlendingActuatorID ActuatorTypeID(AnimTypeID);

			if (!Container.GetAccumulator().FindActuator< FTLLRN_ControlRigTrackTokenTransform>(ActuatorTypeID))
			{
				Container.GetAccumulator().DefineActuator(ActuatorTypeID, MakeShared <TTLLRN_ControlRigParameterActuatorTransform>(AnimTypeID, TransformNameAndValue.ParameterName, Section));
			}

			const FEulerTransform& Transform = TransformNameAndValue.Transform;

			TransformData.Set(0, Transform.Location.X);
			TransformData.Set(1, Transform.Location.Y);
			TransformData.Set(2, Transform.Location.Z);

			TransformData.Set(3, Transform.Rotation.Roll);
			TransformData.Set(4, Transform.Rotation.Pitch);
			TransformData.Set(5, Transform.Rotation.Yaw);

			TransformData.Set(6, Transform.Scale.X);
			TransformData.Set(7, Transform.Scale.Y);
			TransformData.Set(8, Transform.Scale.Z);
			Container.GetAccumulator().BlendToken(FMovieSceneEvaluationOperand(),ActuatorTypeID, FMovieSceneEvaluationScope(), Context, TBlendableToken<FTLLRN_ControlRigTrackTokenTransform>(TransformData, BlendType, Weight, BlendingOrder));
		}

	}
}

