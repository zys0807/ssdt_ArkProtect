#ifndef CXX_ProcessModule_H
#define CXX_ProcessModule_H 
#include <ntifs.h>
#include <ntstrsafe.h>
#include "Private.h"
#include "ProcessCore.h"

//typedef struct _MEMORY_BASIC_INFORMATION {
//	PVOID BaseAddress;
//	PVOID AllocationBase;
//	ULONG AllocationProtect;
//	SIZE_T RegionSize;
//	ULONG State;
//	ULONG Protect;
//	ULONG Type;
//} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;
typedef enum _eLdrType
{
	lt_InLoadOrderModuleList = 0,
	lt_InMemoryOrderModuleList,
	lt_InInitializationOrderModuleList
} eLdrType;

typedef struct _PROCESS_MODULE_ENTRY_INFORMATION
{
	UINT_PTR	BaseAddress;
	UINT_PTR	SizeOfImage;
	WCHAR	    wzFilePath[MAX_PATH];
	WCHAR       wzCompanyName[MAX_PATH];
} PROCESS_MODULE_ENTRY_INFORMATION, *PPROCESS_MODULE_ENTRY_INFORMATION;

typedef struct _PROCESS_MODULE_INFORMATION
{
	UINT32                           NumberOfModules;
	PROCESS_MODULE_ENTRY_INFORMATION ModuleEntry[1];
} PROCESS_MODULE_INFORMATION, *PPROCESS_MODULE_INFORMATION;



BOOLEAN
APIsProcessModuleInList(IN UINT_PTR BaseAddress, IN UINT32 ModuleSize, IN PPROCESS_MODULE_INFORMATION pmi, IN UINT32 ModuleCount);

NTSTATUS 
APEnumProcessModuleByZwQueryVirtualMemory(IN PEPROCESS EProcess, OUT PPROCESS_MODULE_INFORMATION pmi, IN UINT32 ModuleCount);

NTSTATUS
APEnumProcessModuleByPeb(IN PEPROCESS EProcess, OUT PPROCESS_MODULE_INFORMATION pmi, IN UINT32 ModuleCount);

NTSTATUS
APEnumProcessModule(IN UINT32 ProcessId, OUT PVOID OutputBuffer, IN UINT32 OutputLength);

#endif // !CXX_ProcessModule_H
