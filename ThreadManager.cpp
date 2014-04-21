/*-----------------------------------------------------------------------------
 * ThreadManager.cpp
 *-----------------------------------------------------------------------------
 * 
 *-----------------------------------------------------------------------------
 * All rights reserved by somma (fixbrain@gmail.com, unsorted@msn.com)
 *-----------------------------------------------------------------------------
 * - 02.09.2010 created
**---------------------------------------------------------------------------*/
#include "stdafx.h"
#include "ThreadManager.h"
#include "Win32Utils.h"
#include "slogger.h"

/**----------------------------------------------------------------------------
    \brief  

    \param  
    \return         
    \code
    \endcode        
-----------------------------------------------------------------------------*/
PDTTHREAD_CONTEXT 
CreateThreadContext(
    IN LPTHREAD_START_ROUTINE ThreadProcedure,
    IN LPVOID Threadparam
    )
{
    _ASSERTE(NULL != ThreadProcedure);
    if (NULL == ThreadProcedure) return NULL;

    PDTTHREAD_CONTEXT ctx = (PDTTHREAD_CONTEXT) malloc(sizeof(DTTHREAD_CONTEXT));
    if (NULL == ctx) return NULL;
    RtlZeroMemory(ctx, sizeof(DTTHREAD_CONTEXT));

    ctx->KillEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == ctx->KillEvent) {free(ctx); return NULL;}
    ctx->ThreadParam = Threadparam;
    ctx->ThreadProcedure = ThreadProcedure;

    ctx->ThreadHandle = CreateThread(
                            NULL, 
                            0, 
                            ThreadProcedure, 
                            Threadparam, 
                            0, 
                            &ctx->ThreadId
                            );
    _ASSERTE(NULL != ctx->ThreadHandle);
    if (NULL == ctx->ThreadHandle)
    {
        DBG_ERR
            "can not create ReconnectDtServerThread, gle=0x%08x", 
            GetLastError()
        DBG_END

        free(ctx);
        return NULL;
    }

    return ctx;
}

/**----------------------------------------------------------------------------
    \brief  

    \param  
    \return         
    \code
    \endcode        
-----------------------------------------------------------------------------*/
void DestroyThreadContext(IN PDTTHREAD_CONTEXT& ctx)
{
    if(NULL == ctx) return;

    _ASSERTE(NULL != ctx->KillEvent);
    _ASSERTE(NULL != ctx->ThreadHandle);
    if (NULL == ctx || NULL == ctx->KillEvent || NULL == ctx->ThreadHandle) return;


    // DAED-LOCK üũ
    //  - thread procedure ������ DestroyThreadContext() �� ȣ���� ��� 
    // 
    if (GetCurrentThreadId() == ctx->ThreadId)
    {
        DBG_ERR
            "!!CRITICAL!! DEAD LOCK DETECTED, invalid function call, caller tid=0x%08x, ctx tid=0x%08x", 
            GetCurrentThreadId(), ctx->ThreadId
        DBG_END

        return;
    }

    // thread ���� �̺�Ʈ �ñ׳�
    // 
    SetEvent(ctx->KillEvent);
    
    // thread �� ����� ��� ctx ���ҽ��� ����
    // 
    DWORD ExitCode=STILL_ACTIVE;
    if (TRUE != GetExitCodeThread(ctx->ThreadHandle, &ExitCode))
    {
        DBG_ERR
            "GetExitCodeThread(tid=0x%08x, handle=0x%08x) failed, gle=0x%08x",
            ctx->ThreadId, ctx->ThreadHandle, GetLastError()
        DBG_END

        // progream �� ũ���� �ǵ� �� �� ����. 
        // 
        CloseHandle(ctx->KillEvent);
        free(ctx);
        return;
    }

    if (STILL_ACTIVE != ExitCode)
    {
        // �̹� �����尡 ����� ����� �׳� �ڵ鸸 �ݾƹ����� �ȴ�.
        // 
        CloseHandle(ctx->KillEvent);
        free(ctx);
        return;
    }
    else
    {
        if (WAIT_OBJECT_0 != WaitForSingleObject(
                                ctx->ThreadHandle, 
                                CONTEXT_TERMINATED_EVENT_TIMEOUT
                                ))
        {
            DBG_ERR
                "!!CRITICAL!! thread is not responding, tid=0x%08x, handle=0x%08x", 
                ctx->ThreadId, ctx->ThreadHandle
            DBG_END
        }

        CloseHandle(ctx->KillEvent);
        free(ctx);
    }

    return;
}