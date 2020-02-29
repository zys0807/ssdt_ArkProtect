
#ifndef CXX_ZWQUERYVIRTUALMEMORY_H
#define CXX_ZWQUERYVIRTUALMEMORY_H


#include <ntifs.h>
#include <devioctl.h>
typedef unsigned long DWORD;

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObj, IN PUNICODE_STRING pRegistryString);

VOID DriverUnload(IN PDRIVER_OBJECT pDriverObj);


/*
typedef enum _MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation,  //�ڴ������Ϣ
    MemoryWorkingSetList
}MEMORY_INFORMATION_CLASS;*/


typedef NTSTATUS
    (*pfnNtQueryVirtualMemory)(HANDLE ProcessHandle,PVOID BaseAddress,
    MEMORY_INFORMATION_CLASS MemoryInformationClass,
    PVOID MemoryInformation,
    SIZE_T MemoryInformationLength,
    PSIZE_T ReturnLength);

//MemoryBasicInformation 
//typedef struct _MEMORY_BASIC_INFORMATION {  
//    PVOID       BaseAddress;           //��ѯ�ڴ����ռ�ĵ�һ��ҳ�����ַ
//    PVOID       AllocationBase;        //�ڴ����ռ�ĵ�һ���������ַ��С�ڵ���BaseAddress��
//    DWORD       AllocationProtect;     //���򱻳��α���ʱ����ı�������
//    SIZE_T      RegionSize;            //��BaseAddress��ʼ��������ͬ���Ե�ҳ��Ĵ�С��
//    DWORD       State;                 //ҳ���״̬�������ֿ���ֵMEM_COMMIT��MEM_FREE��MEM_RESERVE
//    DWORD       Protect;               //ҳ������ԣ�����ܵ�ȡֵ��AllocationProtect��ͬ
//    DWORD       Type;                  //���ڴ������ͣ������ֿ���ֵ��MEM_IMAGE��MEM_MAPPED��MEM_PRIVATE
//} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;
NTSTATUS EnumMoudleByNtQueryVirtualMemory(ULONG ProcessId);
//MemorySectionName 
//typedef struct _MEMORY_SECTION_NAME  {  
//    UNICODE_STRING Name;  
//    WCHAR     Buffer[260];  
//}MEMORY_SECTION_NAME,*PMEMORY_SECTION_NAME;



#endif