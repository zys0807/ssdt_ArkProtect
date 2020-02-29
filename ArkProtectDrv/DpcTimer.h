#ifndef CXX_DpcTimer_H
#define CXX_DpcTimer_H

#include <ntifs.h>
#include "Private.h"
#include "NtStructs.h"

typedef struct _DPC_TIMER_ENTRY_INFORMATION
{
	UINT_PTR TimerObject;
	UINT_PTR RealDpc;
	UINT_PTR Cycle;       // ����
	UINT_PTR TimeDispatch;
} DPC_TIMER_ENTRY_INFORMATION, *PDPC_TIMER_ENTRY_INFORMATION;

typedef struct _DPC_TIMER_INFORMATION
{
	UINT32                      NumberOfDpcTimers;
	DPC_TIMER_ENTRY_INFORMATION DpcTimerEntry[1];
} DPC_TIMER_INFORMATION, *PDPC_TIMER_INFORMATION;


#ifdef _WIN64

BOOLEAN
FindKiWaitVariableAddress(OUT PUINT_PTR* KiWaitNeverAddress, OUT PUINT_PTR* KiWaitAlwaysAddress);

PKDPC
APTransTimerDPCEx(IN PKTIMER Timer, IN UINT64 KiWaitNeverAddress, IN UINT64 KiWaitAlwaysAddress);

NTSTATUS
APEnumDpcTimerByIterateKTimerTableEntry(OUT PDPC_TIMER_INFORMATION dti, IN UINT32 DpcTimerCount);

#else

UINT_PTR
APGetKiTimerTableListHead();

NTSTATUS 
APEnumDpcTimerByIterateKiTimerTableListHead(OUT PDPC_TIMER_INFORMATION dti, IN UINT32 DpcTimerCount);

#endif // _WIN64


NTSTATUS
APEnumDpcTimer(OUT PVOID OutputBuffer, IN UINT32 OutputLength);

#endif // !CXX_DpcTimer_H


