// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Rigs/TLLRN_RigHierarchyContainer.h"
#include "Rigs/TLLRN_TLLRN_RigCurveContainer.h"
#include "AnimationDataSource.h"
#include "Animation/AttributesRuntime.h"
#include "TLLRN_ControlRigAssetUserData.h"
#include "RigVMCore/RigVMExecuteContext.h"
#include "TLLRN_RigUnitContext.generated.h"

class UTLLRN_ControlRig;
class UTLLRN_ControlRigShapeLibrary;
struct FTLLRN_RigModuleInstance;

/**
 * The type of interaction happening on a rig
 */
UENUM()
enum class ETLLRN_ControlRigInteractionType : uint8
{
	None = 0,
	Translate = (1 << 0),
	Rotate = (1 << 1),
	Scale = (1 << 2),
	All = Translate | Rotate | Scale
};

UENUM(BlueprintType, meta = (RigVMTypeAllowed))
enum class ETLLRN_RigMetaDataNameSpace : uint8
{
	// Use no namespace - store the metadata directly on the item
	None,
	// Store the metadata for item relative to its module
	Self,
	// Store the metadata relative to its parent model
	Parent,
	// Store the metadata under the root module
	Root,
	Last UMETA(Hidden)
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigHierarchySettings
{
	GENERATED_BODY();

	FTLLRN_RigHierarchySettings()
		: ProceduralElementLimit(2000)
	{
	}

	// Sets the limit for the number of elements to create procedurally
	UPROPERTY(EditAnywhere, Category = "Hierarchy Settings")
	int32 ProceduralElementLimit;
};

/** Execution context that rig units use */
struct FTLLRN_RigUnitContext
{
	/** default constructor */
	FTLLRN_RigUnitContext()
		: AnimAttributeContainer(nullptr)
		, DataSourceRegistry(nullptr)
		, InteractionType((uint8)ETLLRN_ControlRigInteractionType::None)
		, ElementsBeingInteracted()
	{
	}

	/** An external anim attribute container */
	UE::Anim::FStackAttributeContainer* AnimAttributeContainer;
	
	/** The registry to access data source */
	const UAnimationDataSourceRegistry* DataSourceRegistry;

	/** The current hierarchy settings */
	FTLLRN_RigHierarchySettings HierarchySettings;

	/** The type of interaction currently happening on the rig (0 == None) */
	uint8 InteractionType;

	/** The elements being interacted with. */
	TArray<FTLLRN_RigElementKey> ElementsBeingInteracted;

	/** Acceptable subset of connection matches */
	FTLLRN_ModularRigResolveResult ConnectionResolve;

	/**
	 * Returns a given data source and cast it to the expected class.
	 *
	 * @param InName The name of the data source to look up.
	 * @return The requested data source
	 */
	template<class T>
	T* RequestDataSource(const FName& InName) const
	{
		if (DataSourceRegistry == nullptr)
		{
			return nullptr;
		}
		return DataSourceRegistry->RequestSource<T>(InName);
	}

	/**
	 * Returns true if this context is currently being interacted on
	 */
	bool IsInteracting() const
	{
		return InteractionType != (uint8)ETLLRN_ControlRigInteractionType::None;
	}
};

USTRUCT(BlueprintType)
struct FTLLRN_ControlRigExecuteContext : public FRigVMExecuteContext
{
	GENERATED_BODY()

public:
	
	FTLLRN_ControlRigExecuteContext()
		: FRigVMExecuteContext()
		, Hierarchy(nullptr)
		, TLLRN_ControlRig(nullptr)
		, TLLRN_RigModuleNameSpace()
		, TLLRN_RigModuleNameSpaceHash(0)
	{
	}

	virtual void Copy(const FRigVMExecuteContext* InOtherContext) override
	{
		Super::Copy(InOtherContext);

		const FTLLRN_ControlRigExecuteContext* OtherContext = (const FTLLRN_ControlRigExecuteContext*)InOtherContext; 
		Hierarchy = OtherContext->Hierarchy;
		TLLRN_ControlRig = OtherContext->TLLRN_ControlRig;
	}

	/**
     * Finds a name spaced user data object
     */
	const UNameSpacedUserData* FindUserData(const FString& InNameSpace) const
	{
		// walk in reverse since asset user data at the end with the same
		// namespace overrides previously listed user data
		for(int32 Index = AssetUserData.Num() - 1; AssetUserData.IsValidIndex(Index); Index--)
		{
			if(!IsValid(AssetUserData[Index]))
			{
				continue;
			}
			if(const UNameSpacedUserData* NameSpacedUserData = Cast<UNameSpacedUserData>(AssetUserData[Index]))
			{
				if(NameSpacedUserData->NameSpace.Equals(InNameSpace, ESearchCase::CaseSensitive))
				{
					return NameSpacedUserData;
				}
			}
		}
		return nullptr;
	}

	/**
	 * Returns true if the event currently running is considered a construction event
	 */
	bool IsRunningConstructionEvent() const;

	/**
	 * Add the namespace from a given name
	 */
	FName AddTLLRN_RigModuleNameSpace(const FName& InName) const;
	FString AddTLLRN_RigModuleNameSpace(const FString& InName) const;

	/**
	 * Remove the namespace from a given name
	 */
	FName RemoveTLLRN_RigModuleNameSpace(const FName& InName) const;
	FString RemoveTLLRN_RigModuleNameSpace(const FString& InName) const;

	/**
	 * Returns true if this context is used on a module currently
	 */
	bool IsTLLRN_RigModule() const
	{
		return !GetTLLRN_RigModuleNameSpace().IsEmpty();
	}

	/**
	 * Returns the namespace of the currently running rig module
	 */
	FString GetTLLRN_RigModuleNameSpace() const
	{
		return TLLRN_RigModuleNameSpace;
	}

	/**
	 * Returns the namespace given a namespace type
	 */
	FString GetElementNameSpace(ETLLRN_RigMetaDataNameSpace InNameSpaceType) const;
	
	/**
	 * Returns the module this unit is running inside of (or nullptr)
	 */
	const FTLLRN_RigModuleInstance* GetTLLRN_RigModuleInstance() const
	{
		return TLLRN_RigModuleInstance;
	}
	
	/**
	 * Returns the module this unit is running inside of (or nullptr)
	 */
	const FTLLRN_RigModuleInstance* GetTLLRN_RigModuleInstance(ETLLRN_RigMetaDataNameSpace InNameSpaceType) const;

	/**
	 * Adapts a metadata name according to rig module namespace.
	 */
	FName AdaptMetadataName(ETLLRN_RigMetaDataNameSpace InNameSpaceType, const FName& InMetadataName) const;

	/** The list of available asset user data object */
	TArray<const UAssetUserData*> AssetUserData;

	DECLARE_DELEGATE_FourParams(FOnAddShapeLibrary, const FTLLRN_ControlRigExecuteContext* InContext, const FString&, UTLLRN_ControlRigShapeLibrary*, bool /* log results */);
	FOnAddShapeLibrary OnAddShapeLibraryDelegate;

	DECLARE_DELEGATE_RetVal_OneParam(bool, FOnShapeExists, const FName&);
	FOnShapeExists OnShapeExistsDelegate;
	
	FTLLRN_RigUnitContext UnitContext;
	UTLLRN_RigHierarchy* Hierarchy;
	UTLLRN_ControlRig* TLLRN_ControlRig;

#if WITH_EDITOR
	virtual void Report(const FRigVMLogSettings& InLogSettings, const FName& InFunctionName, int32 InInstructionIndex, const FString& InMessage) const override
	{
		FString Prefix = GetTLLRN_RigModuleNameSpace();
		if (!Prefix.IsEmpty())
		{
			const FString Name = FString::Printf(TEXT("%s %s"), *Prefix, *InFunctionName.ToString());
			FRigVMExecuteContext::Report(InLogSettings, *Name, InInstructionIndex, InMessage);
		}
		else
		{
			FRigVMExecuteContext::Report(InLogSettings, InFunctionName, InInstructionIndex, InMessage);
		}
	}
#endif

private:
	FString TLLRN_RigModuleNameSpace;
	uint32 TLLRN_RigModuleNameSpaceHash;
	const FTLLRN_RigModuleInstance* TLLRN_RigModuleInstance;

	friend class FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard;
	friend class UTLLRN_ModularRig;
};

class TLLRN_CONTROLRIG_API FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard
{
public:
	FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard(FTLLRN_ControlRigExecuteContext& InContext, const UTLLRN_ControlRig* InTLLRN_ControlRig);
	FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard(FTLLRN_ControlRigExecuteContext& InContext, const FString& InNewModuleNameSpace);
	~FTLLRN_ControlRigExecuteContextTLLRN_RigModuleGuard();

private:

	FTLLRN_ControlRigExecuteContext& Context;
	FString PreviousTLLRN_RigModuleNameSpace;
	uint32 PreviousTLLRN_RigModuleNameSpaceHash;
};

#if WITH_EDITOR
#define UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT(Severity, Format, ...) \
ExecuteContext.Report(EMessageSeverity::Severity, ExecuteContext.GetFunctionName(), ExecuteContext.GetInstructionIndex(), FString::Printf((Format), ##__VA_ARGS__)); 

#define UE_TLLRN_CONTROLRIG_RIGUNIT_LOG_MESSAGE(Format, ...) UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT(Info, (Format), ##__VA_ARGS__)
#define UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(Format, ...) UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT(Warning, (Format), ##__VA_ARGS__)
#define UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(Format, ...) UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT(Error, (Format), ##__VA_ARGS__)
#else
#define UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT(Severity, Format, ...)
#define UE_TLLRN_CONTROLRIG_RIGUNIT_LOG_MESSAGE(Format, ...)
#define UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(Format, ...)
#define UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_ERROR(Format, ...)
#endif
