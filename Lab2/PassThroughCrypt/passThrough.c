#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "aes.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

#define LAB2_EXTENSION   L"lab2ext"

static const UCHAR gLab2Key[32] = {
    0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe, 0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
    0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7, 0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4
};

static const UCHAR gLab2Iv[16] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, 0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
};

static BOOLEAN
Lab2IsTargetFile (
    _Inout_ PFLT_CALLBACK_DATA Data
    )
{
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
    NTSTATUS status;
    BOOLEAN matched = FALSE;
    const UNICODE_STRING required = RTL_CONSTANT_STRING(LAB2_EXTENSION);

    status = FltGetFileNameInformation(
        Data,
        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
        &nameInfo );
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    status = FltParseFileNameInformation( nameInfo );
    if (NT_SUCCESS(status)) {
        matched = RtlEqualUnicodeString( &required, &nameInfo->Extension, FALSE );
    }

    FltReleaseFileNameInformation( nameInfo );
    return matched;
}

static VOID
Lab2CryptBuffer (
    _Inout_updates_bytes_(length) PUCHAR buffer,
    _In_ ULONG length,
    _In_ BOOLEAN encrypt
    )
{
    struct AES_ctx ctx;
    ULONG blocks = length / AES_BLOCKLEN;

    if (buffer == NULL || blocks == 0) {
        return;
    }

    AES_init_ctx_iv( &ctx, gLab2Key, gLab2Iv );

    if (encrypt) {
        AES_CBC_encrypt_buffer( &ctx, buffer, (size_t)blocks * AES_BLOCKLEN );
    } else {
        AES_CBC_decrypt_buffer( &ctx, buffer, (size_t)blocks * AES_BLOCKLEN );
    }
}

PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

ULONG gTraceFlags = 0;

#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
PtInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
PtInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
PtInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
PtUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
PtInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
PtPreOperationPassThrough (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

VOID
PtOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    );

FLT_POSTOP_CALLBACK_STATUS
PtPostOperationPassThrough (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
PtPreOperationNoPostOperationPassThrough (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

BOOLEAN
PtDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PtUnload)
#pragma alloc_text(PAGE, PtInstanceQueryTeardown)
#pragma alloc_text(PAGE, PtInstanceSetup)
#pragma alloc_text(PAGE, PtInstanceTeardownStart)
#pragma alloc_text(PAGE, PtInstanceTeardownComplete)
#endif

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_CLOSE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_READ,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_WRITE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_SET_INFORMATION,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_QUERY_EA,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_SET_EA,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_SHUTDOWN,
      0,
      PtPreOperationNoPostOperationPassThrough,
      NULL },

    { IRP_MJ_LOCK_CONTROL,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_CLEANUP,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_QUERY_SECURITY,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_SET_SECURITY,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_QUERY_QUOTA,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_SET_QUOTA,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_PNP,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_MDL_READ,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      PtPreOperationPassThrough,
      PtPostOperationPassThrough },

    { IRP_MJ_OPERATION_END }
};

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),
    FLT_REGISTRATION_VERSION,
    0,

    NULL,
    Callbacks,

    PtUnload,

    PtInstanceSetup,
    PtInstanceQueryTeardown,
    PtInstanceTeardownStart,
    PtInstanceTeardownComplete,

    NULL,
    NULL,
    NULL

};

NTSTATUS
PtInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )

{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!PtInstanceSetup: Entered\n") );

    return STATUS_SUCCESS;
}

NTSTATUS
PtInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )

{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!PtInstanceQueryTeardown: Entered\n") );

    return STATUS_SUCCESS;
}

VOID
PtInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )

{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!PtInstanceTeardownStart: Entered\n") );
}

VOID
PtInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )

{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!PtInstanceTeardownComplete: Entered\n") );
}

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )

{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!DriverEntry: Entered\n") );

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {

        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterHandle );
        }
    }

    return status;
}

NTSTATUS
PtUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )

{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!PtUnload: Entered\n") );

    FltUnregisterFilter( gFilterHandle );

    return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS
PtPreOperationPassThrough (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )

{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!PtPreOperationPassThrough: Entered\n") );

    if (PtDoRequestOperationStatus( Data )) {

        status = FltRequestOperationStatusCallback( Data,
                                                    PtOperationStatusCallback,
                                                    (PVOID)(++OperationStatusCtx) );
        if (!NT_SUCCESS(status)) {

            PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                          ("PassThrough!PtPreOperationPassThrough: FltRequestOperationStatusCallback Failed, status=%08x\n",
                           status) );
        }
    }

    if (Data->Iopb->MajorFunction == IRP_MJ_WRITE &&
        Lab2IsTargetFile( Data )) {

        PUCHAR writeBuffer = Data->Iopb->Parameters.Write.WriteBuffer;
        ULONG  writeLength = Data->Iopb->Parameters.Write.Length;

        DbgPrint( "*** Lab2: IRP_MJ_WRITE matched, encrypting %lu bytes\n", writeLength );
        if (writeBuffer != NULL && writeLength >= AES_BLOCKLEN) {
            Lab2CryptBuffer( writeBuffer, writeLength, TRUE  );
        }
    }

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

VOID
PtOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    )

{
    UNREFERENCED_PARAMETER( FltObjects );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!PtOperationStatusCallback: Entered\n") );

    PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                  ("PassThrough!PtOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
                   OperationStatus,
                   RequesterContext,
                   ParameterSnapshot->MajorFunction,
                   ParameterSnapshot->MinorFunction,
                   FltGetIrpName(ParameterSnapshot->MajorFunction)) );
}

FLT_POSTOP_CALLBACK_STATUS
PtPostOperationPassThrough (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )

{

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!PtPostOperationPassThrough: Entered\n") );

    if (NT_SUCCESS( Data->IoStatus.Status ) &&
        Data->Iopb->MajorFunction == IRP_MJ_READ &&
        Lab2IsTargetFile( Data )) {

        PUCHAR readBuffer = Data->Iopb->Parameters.Read.ReadBuffer;
        ULONG  bytesRead  = (ULONG)Data->IoStatus.Information;

        DbgPrint( "*** Lab2: IRP_MJ_READ matched, decrypting %lu bytes\n", bytesRead );
        if (readBuffer != NULL && bytesRead >= AES_BLOCKLEN) {
            Lab2CryptBuffer( readBuffer, bytesRead, FALSE  );
        }
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
PtPreOperationNoPostOperationPassThrough (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )

{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("PassThrough!PtPreOperationNoPostOperationPassThrough: Entered\n") );

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

BOOLEAN
PtDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    )

{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    return (BOOLEAN)

             (((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
               ((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK)  ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK)   ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

              ||

              ((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
               (iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
             );
}
