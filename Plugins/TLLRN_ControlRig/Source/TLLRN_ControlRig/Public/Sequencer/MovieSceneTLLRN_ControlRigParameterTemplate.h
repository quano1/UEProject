// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Evaluation/MovieSceneParameterTemplate.h"
#include "MovieSceneTLLRN_ControlRigParameterSection.h"

#include "MovieSceneTLLRN_ControlRigParameterTemplate.generated.h"

class UMovieSceneTLLRN_ControlRigParameterTrack;
struct FEvaluatedTLLRN_ControlRigParameterSectionValues;
USTRUCT()
struct FMovieSceneTLLRN_ControlRigParameterTemplate : public FMovieSceneParameterSectionTemplate
{
	GENERATED_BODY()

	FMovieSceneTLLRN_ControlRigParameterTemplate() {}
	FMovieSceneTLLRN_ControlRigParameterTemplate(const UMovieSceneTLLRN_ControlRigParameterSection& Section, const UMovieSceneTLLRN_ControlRigParameterTrack& Track);
	static FMovieSceneAnimTypeID GetAnimTypeID();

private:
	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
	virtual void Interrogate(const FMovieSceneContext& Context, FMovieSceneInterrogationData& Container, UObject* BindingOverride) const override;

private:
	void EvaluateCurvesWithMasks(const FMovieSceneContext& Context, struct FEvaluatedTLLRN_ControlRigParameterSectionChannelMasks& InMasks, FEvaluatedTLLRN_ControlRigParameterSectionValues& Values) const;

protected:
	/** The bool parameter names and their associated curves. */
	UPROPERTY()
	TArray<FTLLRN_EnumParameterNameAndCurve> Enums;
	/** The enum parameter names and their associated curves. */
	UPROPERTY()
	TArray<FTLLRN_IntegerParameterNameAndCurve> Integers;
	/** Controls and their space keys*/
	UPROPERTY()
	TArray<FTLLRN_SpaceControlNameAndChannel> Spaces;
	
	UPROPERTY()
	TArray<FConstraintAndActiveChannel> Constraints;
};

