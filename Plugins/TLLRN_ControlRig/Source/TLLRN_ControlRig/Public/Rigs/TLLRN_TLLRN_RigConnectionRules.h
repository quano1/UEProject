// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_RigHierarchyDefines.h"
#include "UObject/StructOnScope.h"
#include "TLLRN_TLLRN_RigConnectionRules.generated.h"

struct FTLLRN_RigBaseElement;
struct FTLLRN_RigConnectionRule;
class FTLLRN_RigElementKeyRedirector;
class UTLLRN_RigHierarchy;
struct FTLLRN_RigModuleInstance;
struct FTLLRN_RigBaseElement;
struct FTLLRN_RigTransformElement;
struct FTLLRN_RigConnectorElement;

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigConnectionRuleStash
{
	GENERATED_BODY()

	FTLLRN_RigConnectionRuleStash();
	FTLLRN_RigConnectionRuleStash(const FTLLRN_RigConnectionRule* InRule);

	void Save(FArchive& Ar);
	void Load(FArchive& Ar);
	
	friend uint32 GetTypeHash(const FTLLRN_RigConnectionRuleStash& InRuleStash);

	bool IsValid() const;
	UScriptStruct* GetScriptStruct() const;
	TSharedPtr<FStructOnScope> Get() const;
	const FTLLRN_RigConnectionRule* Get(TSharedPtr<FStructOnScope>& InOutStorage) const;

	bool operator == (const FTLLRN_RigConnectionRuleStash& InOther) const;

	bool operator != (const FTLLRN_RigConnectionRuleStash& InOther) const
	{
		return !(*this == InOther);
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rule)
	FString ScriptStructPath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rule)
	FString ExportedText;
};

struct TLLRN_CONTROLRIG_API FTLLRN_RigConnectionRuleInput
{
public:
	
	FTLLRN_RigConnectionRuleInput()
	: Hierarchy(nullptr)
	, Module(nullptr)
	, Redirector(nullptr)
	{
	}

	const UTLLRN_RigHierarchy* GetHierarchy() const
	{
		return Hierarchy;
	}
	
	const FTLLRN_RigModuleInstance* GetModule() const
	{
		return Module;
	}
	
	const FTLLRN_RigElementKeyRedirector* GetRedirector() const
	{
		return Redirector;
	}

	const FTLLRN_RigConnectorElement* FindPrimaryConnector(FText* OutErrorMessage = nullptr) const;
   	TArray<const FTLLRN_RigConnectorElement*> FindSecondaryConnectors(bool bOptional, FText* OutErrorMessage = nullptr) const;

	const FTLLRN_RigTransformElement* ResolveConnector(const FTLLRN_RigConnectorElement* InConnector, FText* OutErrorMessage) const;
	const FTLLRN_RigTransformElement* ResolvePrimaryConnector(FText* OutErrorMessage = nullptr) const;

private:

	const UTLLRN_RigHierarchy* Hierarchy;
	const FTLLRN_RigModuleInstance* Module;
	const FTLLRN_RigElementKeyRedirector* Redirector;

	friend class UTLLRN_ModularRigRuleManager;
};

USTRUCT(meta=(Hidden))
struct TLLRN_CONTROLRIG_API FTLLRN_RigConnectionRule
{
	GENERATED_BODY()

public:

	FTLLRN_RigConnectionRule() {}
	virtual ~FTLLRN_RigConnectionRule() {}

	virtual UScriptStruct* GetScriptStruct() const { return FTLLRN_RigConnectionRule::StaticStruct(); }
	virtual FTLLRN_RigElementResolveResult Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const;
};

USTRUCT(BlueprintType, DisplayName="And Rule")
struct TLLRN_CONTROLRIG_API FTLLRN_RigAndConnectionRule : public FTLLRN_RigConnectionRule
{
	GENERATED_BODY()

public:

	FTLLRN_RigAndConnectionRule()
	{}

	template<typename TypeA, typename TypeB>
	FTLLRN_RigAndConnectionRule(const TypeA& InA, const TypeB& InB)
	{
		ChildRules.Emplace(&InA);
		ChildRules.Emplace(&InB);
	}

	virtual ~FTLLRN_RigAndConnectionRule() override {}

	virtual UScriptStruct* GetScriptStruct() const override { return FTLLRN_RigAndConnectionRule::StaticStruct(); }
	virtual FTLLRN_RigElementResolveResult Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Settings)
	TArray<FTLLRN_RigConnectionRuleStash> ChildRules;
};

USTRUCT(BlueprintType, DisplayName="Or Rule")
struct TLLRN_CONTROLRIG_API FTLLRN_RigOrConnectionRule : public FTLLRN_RigConnectionRule
{
	GENERATED_BODY()

public:

	FTLLRN_RigOrConnectionRule()
	{}

	template<typename TypeA, typename TypeB>
	FTLLRN_RigOrConnectionRule(const TypeA& InA, const TypeB& InB)
	{
		ChildRules.Emplace(&InA);
		ChildRules.Emplace(&InB);
	}

	virtual ~FTLLRN_RigOrConnectionRule() override {}

	virtual UScriptStruct* GetScriptStruct() const override { return FTLLRN_RigOrConnectionRule::StaticStruct(); }
	virtual FTLLRN_RigElementResolveResult Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Settings)
	TArray<FTLLRN_RigConnectionRuleStash> ChildRules;
};

USTRUCT(BlueprintType, DisplayName="Type Rule")
struct TLLRN_CONTROLRIG_API FTLLRN_RigTypeConnectionRule : public FTLLRN_RigConnectionRule
{
	GENERATED_BODY()

public:

	FTLLRN_RigTypeConnectionRule()
		: ElementType(ETLLRN_RigElementType::Socket)
	{}

	FTLLRN_RigTypeConnectionRule(ETLLRN_RigElementType InElementType)
	: ElementType(InElementType)
	{}

	virtual ~FTLLRN_RigTypeConnectionRule() override {}

	virtual UScriptStruct* GetScriptStruct() const override { return FTLLRN_RigTypeConnectionRule::StaticStruct(); }
	virtual FTLLRN_RigElementResolveResult Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Settings)
	ETLLRN_RigElementType ElementType;
};

USTRUCT(BlueprintType, DisplayName="Tag Rule")
struct TLLRN_CONTROLRIG_API FTLLRN_RigTagConnectionRule : public FTLLRN_RigConnectionRule
{
	GENERATED_BODY()

public:

	FTLLRN_RigTagConnectionRule()
		: Tag(NAME_None)
	{}

	FTLLRN_RigTagConnectionRule(const FName& InTag)
	: Tag(InTag)
	{}

	virtual ~FTLLRN_RigTagConnectionRule() override {}

	virtual UScriptStruct* GetScriptStruct() const override { return FTLLRN_RigTagConnectionRule::StaticStruct(); }
	virtual FTLLRN_RigElementResolveResult Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Settings)
	FName Tag;
};

USTRUCT(BlueprintType, DisplayName="Child of Primary")
struct TLLRN_CONTROLRIG_API FTLLRN_RigChildOfPrimaryConnectionRule : public FTLLRN_RigConnectionRule
{
	GENERATED_BODY()

public:

	FTLLRN_RigChildOfPrimaryConnectionRule()
	{}

	virtual ~FTLLRN_RigChildOfPrimaryConnectionRule() override {}

	virtual UScriptStruct* GetScriptStruct() const override { return FTLLRN_RigChildOfPrimaryConnectionRule::StaticStruct(); }
	virtual FTLLRN_RigElementResolveResult Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const override;
};
/*
USTRUCT(BlueprintType, DisplayName="On Chain Rule")
struct TLLRN_CONTROLRIG_API FRigOnChainRule : public FTLLRN_RigConnectionRule
{
	GENERATED_BODY()

public:

	FRigOnChainRule()
	: MinNumBones(2)
	, MaxNumBones(0)
	{}

	FRigOnChainRule(int32 InMinNumBones = 2, int32 InMaxNumBones = 0)
	: MinNumBones(InMinNumBones)
	, MaxNumBones(InMaxNumBones)
	{}

	virtual ~FRigOnChainRule() override {}

	virtual UScriptStruct* GetScriptStruct() const override { return FRigOnChainRule::StaticStruct(); }
	virtual FTLLRN_RigElementResolveResult Resolve(const FTLLRN_RigBaseElement* InTarget, const FTLLRN_RigConnectionRuleInput& InRuleInput) const override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Settings)
	int32 MinNumBones;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Settings)
	int32 MaxNumBones;
};
*/