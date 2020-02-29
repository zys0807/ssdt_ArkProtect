/***************************************************************************************
* AUTHOR : MZ
* DATE   : 2016-3-18
* MODULE : ZwQueryVirtualMemory.C
* 
* Command: 
*    Source of IOCTRL Sample Driver
*
* Description:
*        Demonstrates communications between USER and KERNEL.
*
****************************************************************************************
* Copyright (C) 2010 MZ.
****************************************************************************************/

//#######################################################################################
//# I N C L U D E S
//#######################################################################################

#ifndef CXX_ZWQUERYVIRTUALMEMORY_H
#    include "ZwQueryVirtualMemory.h"
#include "Common.h"
#include "GetSSDTFuncAddress.h"
#endif
typedef struct _MEMORY_SECTION_NAME  {
	UNICODE_STRING Name;
	WCHAR     Buffer[260];
}MEMORY_SECTION_NAME, *PMEMORY_SECTION_NAME;
/*
typedef struct _MEMORY_BASIC_INFORMATION {
	PVOID       BaseAddress;           //��ѯ�ڴ����ռ�ĵ�һ��ҳ�����ַ
	PVOID       AllocationBase;        //�ڴ����ռ�ĵ�һ���������ַ��С�ڵ���BaseAddress��
	DWORD       AllocationProtect;     //���򱻳��α���ʱ����ı�������
	SIZE_T      RegionSize;            //��BaseAddress��ʼ��������ͬ���Ե�ҳ��Ĵ�С��
	DWORD       State;                 //ҳ���״̬�������ֿ���ֵMEM_COMMIT��MEM_FREE��MEM_RESERVE
	DWORD       Protect;               //ҳ������ԣ�����ܵ�ȡֵ��AllocationProtect��ͬ
	DWORD       Type;                  //���ڴ������ͣ������ֿ���ֵ��MEM_IMAGE��MEM_MAPPED��MEM_PRIVATE
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;*/
typedef NTSTATUS
(*pfnNtQueryVirtualMemory)(HANDLE ProcessHandle, PVOID BaseAddress,
MEMORY_INFORMATION_CLASS MemoryInformationClass,
PVOID MemoryInformation,
SIZE_T MemoryInformationLength,
PSIZE_T ReturnLength);

extern ULONG_PTR  ObjectTableOffsetOf_EPROCESS;
extern ULONG_PTR  PreviousModeOffsetOf_KTHREAD;
extern ULONG_PTR  IndexOffsetOfFunction;

extern ULONG_PTR  ObjectHeaderSize;
extern ULONG_PTR  ObjectTypeOffsetOf_OBJECT_HEADER;
extern ULONG_PTR  HighUserAddress;

extern WIN_VERSION WinVersion;
extern ULONG_PTR LdrInPebOffset;
extern ULONG_PTR ModListInLdrOffset;

extern ULONG_PTR  HighUserAddress;

extern pfnNtQueryVirtualMemory   my_NtQueryVirtualMemoryAddress;

//NTSTATUS
//DriverEntry2(IN PDRIVER_OBJECT pDriverObj, IN PUNICODE_STRING pRegistryString)
//{
//    NTSTATUS        status = STATUS_SUCCESS;
//    UNICODE_STRING  ustrLinkName;
//    UNICODE_STRING  ustrDevName;  
//    PDEVICE_OBJECT  pDevObj;
//    PEPROCESS Process = NULL;
//    HANDLE Id = NULL;
//
//    WinVersion = GetWindowsVersion();
//    InitGlobalVariable();
//    NtQueryVirtualMemoryAddress = (pfnNtQueryVirtualMemory)GetFuncAddress("NtQueryVirtualMemory");
//
//    Process = PsGetCurrentProcess();
//    Id = PsGetProcessId(Process);
//    EnumMoudleByNtQueryVirtualMemory((ULONG)Id);
//    pDriverObj->DriverUnload = DriverUnload;
//
//    return STATUS_SUCCESS;
//}
//
//VOID
//DriverUnload2(IN PDRIVER_OBJECT pDriverObj)
//{    
//    return;
//}
//

//NTSTATUS EnumMoudleByNtQueryVirtualMemory(ULONG ProcessId)
//{
//    NTSTATUS Status;
//    PEPROCESS  Process = NULL;
//    HANDLE    hProcess = NULL;
//    SIZE_T ulRet = 0;
//    WCHAR DosPath[260] = {0};
//
//    if (ProcessId)
//    {
//        Status = PsLookupProcessByProcessId((HANDLE)ProcessId, &Process);
//        if (!NT_SUCCESS(Status))
//        {
//            return Status;
//        }    
//    }
//    if (IsRealProcess(Process))   //�ж��Ƿ�Ϊ��ʬ���̣���ֻ���ж��˶������ͺ;�����Ƿ����
//    {
//        ObfDereferenceObject(Process);
//        Status = ObOpenObjectByPointer(Process, 
//            OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, 
//            NULL, 
//            GENERIC_ALL, 
//            *PsProcessType, 
//            KernelMode, 
//            &hProcess
//            );
//        if (NT_SUCCESS(Status))
//        {
//            ULONG_PTR ulBase = 0;
//            //�ı�PreviousMode
//            PETHREAD EThread = PsGetCurrentThread();
//            CHAR PreMode     = ChangePreMode(EThread);   //KernelMode
//            do 
//            {
//                MEMORY_BASIC_INFORMATION mbi = {0};
//                Status = NtQueryVirtualMemoryAddress(hProcess, 
//                    (PVOID)ulBase, 
//                    MemoryBasicInformation, 
//                    &mbi, 
//                    sizeof(MEMORY_BASIC_INFORMATION), 
//                    &ulRet);
//                if (NT_SUCCESS(Status))
//                {    
//                    //�����Image �ٲ�ѯSectionName,��FileObject Name
//                    if (mbi.Type==MEM_IMAGE)
//                    {
//                        MEMORY_SECTION_NAME msn = {0};
//                        Status = NtQueryVirtualMemoryAddress(hProcess,
//                            (PVOID)ulBase,
//                            MemorySectionName,
//                            &msn,
//                            sizeof(MEMORY_SECTION_NAME),
//                            &ulRet);
//                        if (NT_SUCCESS(Status)) 
//                        {
//                            DbgPrint("SectionName:%wZ\r\n",&(msn.Name));  
//                            NtPathToDosPathW(msn.Name.Buffer,DosPath);
//                            DbgPrint("DosName:%S\r\n",DosPath);
//                        }
//                    }
//                    ulBase += mbi.RegionSize;
//                }
//                else ulBase += PAGE_SIZE;    
//            } while (ulBase < (ULONG_PTR)HighUserAddress);
//            NtClose(hProcess);
//            RecoverPreMode(EThread,PreMode);
//        }
//    }
//    return Status;
//}
//

