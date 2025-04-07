// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_RigHierarchyElements.h"
#include "TLLRN_TLLRN_RigModuleDefines.generated.h"

USTRUCT(BlueprintType)
struct FTLLRN_ModularRigSettings
{
	GENERATED_BODY()

	// Whether or not to autoresolve secondary connectors once the primary connector is resolved
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = TLLRN_ModularRig)
	bool bAutoResolve = true;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigModuleIdentifier
{
	GENERATED_BODY()
	
	FTLLRN_RigModuleIdentifier()
		: Name()
		, Type(TEXT("Module"))
	{}

	// The name of the module used to find it in the module library
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FString Name;

	// The kind of module this is (for example "Arm")
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FString Type;

	bool IsValid() const { return !Name.IsEmpty(); }
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigModuleConnector
{
	GENERATED_BODY()
	
	FTLLRN_RigModuleConnector()
	{}

	bool operator==(const FTLLRN_RigModuleConnector& Other) const;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FString Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FTLLRN_RigConnectorSettings Settings;
	
	bool IsPrimary() const { return Settings.Type == ETLLRN_ConnectorType::Primary; }
	bool IsSecondary() const { return Settings.Type == ETLLRN_ConnectorType::Secondary; }
	bool IsOptional() const { return IsSecondary() && Settings.bOptional; }
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigModuleSettings
{
	GENERATED_BODY()
	
	FTLLRN_RigModuleSettings()
	{}

	bool IsValidModule(bool bRequireExposedConnectors = true) const
	{
		return
			Identifier.IsValid() &&
			(!bRequireExposedConnectors || !ExposedConnectors.IsEmpty());
	}

	const FTLLRN_RigModuleConnector* FindPrimaryConnector() const
	{
		return ExposedConnectors.FindByPredicate([](const FTLLRN_RigModuleConnector& Connector)
		{
			return Connector.IsPrimary();
		});
	}

	// The identifier used to retrieve the module in the module library
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Module)
	FTLLRN_RigModuleIdentifier Identifier;

	// The icon used for the module in the module library
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Module,  meta = (AllowedClasses = "/Script/Engine.Texture2D"))
	FSoftObjectPath Icon;

	// The category of the module
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Module)
	FString Category;

	// The keywords of the module
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Module)
	FString Keywords;

	// The description of the module
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Module, meta = (MultiLine = true))
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = Module)
	TArray<FTLLRN_RigModuleConnector> ExposedConnectors;
};


USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigModuleDescription
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Module)
	FSoftObjectPath Path;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Module)
	FTLLRN_RigModuleSettings Settings;
};