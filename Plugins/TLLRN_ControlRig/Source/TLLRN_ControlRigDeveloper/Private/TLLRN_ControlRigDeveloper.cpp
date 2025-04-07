// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigDeveloper.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "TLLRN_ControlRigBlueprintCompiler.h"
#include "Modules/ModuleManager.h"
#include "MessageLogModule.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigDeveloperModule"

DEFINE_LOG_CATEGORY(LogTLLRN_ControlRigDeveloper);

class FTLLRN_ControlRigDeveloperModule : public ITLLRN_ControlRigDeveloperModule
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Compiler customization for animation controllers */
	FTLLRN_ControlRigBlueprintCompiler TLLRN_ControlRigBlueprintCompiler;

	static TSharedPtr<FKismetCompilerContext> GetTLLRN_ControlRigCompiler(UBlueprint* BP, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompileOptions);

	virtual void RegisterPinTypeColor(UStruct* Struct, const FLinearColor Color) override;

	virtual void UnRegisterPinTypeColor(UStruct* Struct) override;
	
	virtual const FLinearColor* FindPinTypeColor(UStruct* Struct) const override;

private:

	TMap<UStruct*, FLinearColor> PinTypeColorMap;
};

void FTLLRN_ControlRigDeveloperModule::StartupModule()
{
	// Register blueprint compiler
	FKismetCompilerContext::RegisterCompilerForBP(UTLLRN_ControlRigBlueprint::StaticClass(), &FTLLRN_ControlRigDeveloperModule::GetTLLRN_ControlRigCompiler);
	IKismetCompilerInterface& KismetCompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>("KismetCompiler");
	KismetCompilerModule.GetCompilers().Add(&TLLRN_ControlRigBlueprintCompiler);

	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	FMessageLogInitializationOptions InitOptions;
	InitOptions.bShowFilters = true;
	InitOptions.bShowPages = false;
	InitOptions.bAllowClear = true;
	MessageLogModule.RegisterLogListing("TLLRN_ControlRigLog", LOCTEXT("TLLRN_ControlRigLog", "TLLRN Control Rig Log"), InitOptions);
}

void FTLLRN_ControlRigDeveloperModule::ShutdownModule()
{
	IKismetCompilerInterface* KismetCompilerModule = FModuleManager::GetModulePtr<IKismetCompilerInterface>("KismetCompiler");
	if (KismetCompilerModule)
	{
		KismetCompilerModule->GetCompilers().Remove(&TLLRN_ControlRigBlueprintCompiler);
	}

	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessageLogModule.UnregisterLogListing("TLLRN_ControlRigLog");
}

TSharedPtr<FKismetCompilerContext> FTLLRN_ControlRigDeveloperModule::GetTLLRN_ControlRigCompiler(UBlueprint* BP, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompileOptions)
{
	return TSharedPtr<FKismetCompilerContext>(new FTLLRN_ControlRigBlueprintCompilerContext(BP, InMessageLog, InCompileOptions));
}

void FTLLRN_ControlRigDeveloperModule::RegisterPinTypeColor(UStruct* Struct, const FLinearColor Color)
{
	if (Struct)
	{ 
		PinTypeColorMap.FindOrAdd(Struct) = Color;
	}
}

void FTLLRN_ControlRigDeveloperModule::UnRegisterPinTypeColor(UStruct* Struct)
{
	PinTypeColorMap.Remove(Struct);
}

const FLinearColor* FTLLRN_ControlRigDeveloperModule::FindPinTypeColor(UStruct* Struct) const
{
	return PinTypeColorMap.Find(Struct);
}

IMPLEMENT_MODULE(FTLLRN_ControlRigDeveloperModule, TLLRN_ControlRigDeveloper)

#undef LOCTEXT_NAMESPACE
