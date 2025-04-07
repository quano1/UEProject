// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rig/Solvers/TLLRN_IKRigSolver.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRigSolver)

#if WITH_EDITORONLY_DATA

void UTLLRN_IKRigSolver::PostLoad()
{
	Super::PostLoad();
	SetFlags(RF_Transactional); // patch old solvers to enable undo/redo
}

void UTLLRN_IKRigSolver::PostTransacted(const FTransactionObjectEvent& TransactionEvent)
{
	Super::PostTransacted(TransactionEvent);
	TLLRN_IKRigSolverModified.Broadcast(this);
}

#endif


