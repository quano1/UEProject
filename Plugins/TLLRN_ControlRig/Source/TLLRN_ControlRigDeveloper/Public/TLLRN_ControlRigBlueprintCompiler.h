// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RigVMBlueprintCompiler.h"

class TLLRN_CONTROLRIGDEVELOPER_API FTLLRN_ControlRigBlueprintCompiler : public FRigVMBlueprintCompiler
{
public:
	/** IBlueprintCompiler interface */
	virtual bool CanCompile(const UBlueprint* Blueprint) override;
	virtual void Compile(UBlueprint* Blueprint, const FKismetCompilerOptions& CompileOptions, FCompilerResultsLog& Results) override;
};

class TLLRN_CONTROLRIGDEVELOPER_API FTLLRN_ControlRigBlueprintCompilerContext : public FRigVMBlueprintCompilerContext
{
public:
	FTLLRN_ControlRigBlueprintCompilerContext(UBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions)
		: FRigVMBlueprintCompilerContext(SourceSketch, InMessageLog, InCompilerOptions)
	{
	}

	// FKismetCompilerContext interface
	virtual void SpawnNewClass(const FString& NewClassName) override;
	virtual void CopyTermDefaultsToDefaultObject(UObject* DefaultObject) override;
};