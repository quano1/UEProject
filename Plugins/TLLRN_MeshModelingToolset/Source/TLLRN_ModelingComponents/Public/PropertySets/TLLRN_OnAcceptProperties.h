// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveToolBuilder.h"

#include "TLLRN_OnAcceptProperties.generated.h"


class UInteractiveToolManager;
class AActor;


/** Options to handle source meshes */
UENUM()
enum class ETLLRN_HandleSourcesMethod : uint8
{
	/** Delete all input objects */
	DeleteSources UMETA(DisplayName = "Delete Inputs"),

	/** Hide all input objects */
	HideSources UMETA(DisplayName = "Hide Inputs"),

	/** Keep all input objects */
	KeepSources UMETA(DisplayName = "Keep Inputs"),

	/** Keep only the first input object and delete all other input objects */
	KeepFirstSource UMETA(DisplayName = "Keep First Input"),

	/** Keep only the last input object and delete all other input objects */
	KeepLastSource UMETA(DisplayName = "Keep Last Input")
};

// Base class for property settings for tools that create a new actor and need to decide what to do with the input objects.
UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_OnAcceptHandleSourcesPropertiesBase : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:

	void ApplyMethod(const TArray<AActor*>& Actors, UInteractiveToolManager* ToolManager, const AActor* MustKeepActor = nullptr);

protected:
	virtual ETLLRN_HandleSourcesMethod GetHandleInputs() const
	{
		return ETLLRN_HandleSourcesMethod::KeepSources;
	}
};

// Specialization for property settings for tools that create a new actor and need to decide what to do with multiple input objects.
UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_OnAcceptHandleSourcesProperties : public UTLLRN_OnAcceptHandleSourcesPropertiesBase
{
	GENERATED_BODY()
public:

	/** Defines what to do with the input objects when accepting the tool results. */
	UPROPERTY(EditAnywhere, Category = OnToolAccept)
	ETLLRN_HandleSourcesMethod HandleInputs;

protected:
	virtual ETLLRN_HandleSourcesMethod GetHandleInputs() const override
	{
		return HandleInputs;
	}
};

// Specialization for property settings for tools that create a new actor and need to decide what to do with a single input object.
UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_OnAcceptHandleSourcesPropertiesSingle : public UTLLRN_OnAcceptHandleSourcesPropertiesBase
{
	GENERATED_BODY()
public:

	/** Defines what to do with the input object when accepting the tool results. */
	UPROPERTY(EditAnywhere, Category = OnToolAccept, meta = (ValidEnumValues = "DeleteSources, HideSources, KeepSources"))
	ETLLRN_HandleSourcesMethod HandleInputs;

protected:
	virtual ETLLRN_HandleSourcesMethod GetHandleInputs() const override
	{
		return HandleInputs;
	}
};
