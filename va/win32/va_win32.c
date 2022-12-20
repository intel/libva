/*
 * Copyright © Microsoft Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "sysdeps.h"
#include "va.h"
#include "va_backend.h"
#include "va_internal.h"
#include "va_trace.h"
#include "va_win32.h"
#include "compat_win32.h"

/*
 * Initialize default driver to the VAOn12 driver implementation
 * which will be selected when provided with an adapter LUID which
 * does not have a registered VA driver
*/
const char VAAPI_DEFAULT_DRIVER_NAME[] = "vaon12";

typedef struct _VADisplayContextWin32 {
    LUID adapter_luid;
    char registry_driver_name[MAX_PATH];
    bool registry_driver_available_flag;
} VADisplayContextWin32;

static void LoadDriverNameFromRegistry(VADisplayContextWin32* pWin32Ctx)
{
    HMODULE hGdi32 = LoadLibraryA("gdi32.dll");
    if (!hGdi32)
        return;

    D3DKMT_OPENADAPTERFROMLUID OpenArgs = { .AdapterLuid = pWin32Ctx->adapter_luid };
    D3DDDI_QUERYREGISTRY_INFO RegistryInfo = {
        .QueryType = D3DDDI_QUERYREGISTRY_ADAPTERKEY,
        .QueryFlags.TranslatePath = true,
        .ValueName = L"VAAPIDriverName",
        .ValueType = REG_SZ,
    };
    D3DDDI_QUERYREGISTRY_INFO *pRegistryInfo = &RegistryInfo;
#ifndef _WIN64
    BOOL isWowProcess = false;
    if (IsWow64Process(GetCurrentProcess(), &isWowProcess) && isWowProcess)
        wcscpy(RegistryInfo.ValueName, "VAAPIDriverNameWow");
#endif
    D3DKMT_QUERYADAPTERINFO QAI = {
        .Type = KMTQAITYPE_QUERYREGISTRY,
        .pPrivateDriverData = &RegistryInfo,
        .PrivateDriverDataSize = sizeof(RegistryInfo),
    };

    PFND3DKMT_OPENADAPTERFROMLUID pfnOpenAdapterFromLuid = (PFND3DKMT_OPENADAPTERFROMLUID)GetProcAddress(hGdi32, "D3DKMTOpenAdapterFromLuid");
    PFND3DKMT_CLOSEADAPTER pfnCloseAdapter = (PFND3DKMT_CLOSEADAPTER)GetProcAddress(hGdi32, "D3DKMTCloseAdapter");
    PFND3DKMT_QUERYADAPTERINFO pfnQueryAdapterInfo = (PFND3DKMT_QUERYADAPTERINFO)GetProcAddress(hGdi32, "D3DKMTQueryAdapterInfo");
    if (!pfnOpenAdapterFromLuid || !pfnCloseAdapter || !pfnQueryAdapterInfo)
        goto cleanup;

    if (!NT_SUCCESS(pfnOpenAdapterFromLuid(&OpenArgs)))
        goto cleanup;

    QAI.hAdapter = OpenArgs.hAdapter;
    if (!NT_SUCCESS(pfnQueryAdapterInfo(&QAI)) ||
        pRegistryInfo->Status != D3DDDI_QUERYREGISTRY_STATUS_BUFFER_OVERFLOW)
        goto cleanup;

    size_t RegistryInfoSize = sizeof(RegistryInfo) + RegistryInfo.OutputValueSize;
    pRegistryInfo = malloc(RegistryInfoSize);
    if (!pRegistryInfo)
        goto cleanup;

    memcpy(pRegistryInfo, &RegistryInfo, sizeof(RegistryInfo));
    QAI.pPrivateDriverData = pRegistryInfo;
    QAI.PrivateDriverDataSize = RegistryInfoSize;
    if (!NT_SUCCESS(pfnQueryAdapterInfo(&QAI)) ||
        pRegistryInfo->Status != D3DDDI_QUERYREGISTRY_STATUS_SUCCESS)
        goto cleanup;

    if (!WideCharToMultiByte(CP_ACP, 0, pRegistryInfo->OutputString,
                             RegistryInfo.OutputValueSize / sizeof(wchar_t),
                             pWin32Ctx->registry_driver_name,
                             sizeof(pWin32Ctx->registry_driver_name),
                             NULL, NULL))
        goto cleanup;

    pWin32Ctx->registry_driver_available_flag = true;

cleanup:
    if (pRegistryInfo && pRegistryInfo != &RegistryInfo)
        free(pRegistryInfo);
    if (pfnCloseAdapter && OpenArgs.hAdapter) {
        D3DKMT_CLOSEADAPTER Close = { OpenArgs.hAdapter };
        pfnCloseAdapter(&Close);
    }
    FreeLibrary(hGdi32);
}

static int va_DisplayContextIsValid(
    VADisplayContextP pDisplayContext
)
{
    return (pDisplayContext != NULL &&
            pDisplayContext->pDriverContext != NULL);
}

static void va_DisplayContextDestroy(
    VADisplayContextP pDisplayContext
)
{
    if (pDisplayContext == NULL)
        return;

    if (pDisplayContext->pDriverContext
        && pDisplayContext->pDriverContext->native_dpy)
        free(pDisplayContext->pDriverContext->native_dpy);

    free(pDisplayContext->pDriverContext);
    free(pDisplayContext->opaque);
    free(pDisplayContext);
}

static VAStatus va_DisplayContextGetNumCandidates(
    VADisplayContextP pDisplayContext,
    int *num_candidates
)
{
    /*  Always report the default driver name
        If available, also add the adapter specific registered driver */
    LUID* adapter = pDisplayContext->pDriverContext->native_dpy;
    VADisplayContextWin32* pWin32Ctx = (VADisplayContextWin32*) pDisplayContext->opaque;
    if (adapter && pWin32Ctx->registry_driver_available_flag)
        *num_candidates = 2;
    else
        *num_candidates = 1;

    return VA_STATUS_SUCCESS;
}

static VAStatus va_DisplayContextGetDriverNameByIndex(
    VADisplayContextP pDisplayContext,
    char **driver_name,
    int candidate_index
)
{
    LUID* adapter = pDisplayContext->pDriverContext->native_dpy;
    VADisplayContextWin32* pWin32Ctx = (VADisplayContextWin32*) pDisplayContext->opaque;
    if (adapter && pWin32Ctx->registry_driver_available_flag) {
        /* Always prefer the adapter registered driver name as first option */
        if (candidate_index == 0) {
            *driver_name = calloc(sizeof(pWin32Ctx->registry_driver_name), sizeof(char));
            memcpy(*driver_name, pWin32Ctx->registry_driver_name, sizeof(pWin32Ctx->registry_driver_name));
        }
        /* Provide the default driver name as a fallback option */
        else if (candidate_index == 1) {
            *driver_name = calloc(sizeof(VAAPI_DEFAULT_DRIVER_NAME), sizeof(char));
            memcpy(*driver_name, VAAPI_DEFAULT_DRIVER_NAME, sizeof(VAAPI_DEFAULT_DRIVER_NAME));
        }
    } else {
        /* Provide the default driver name as a fallback option */
        *driver_name = calloc(sizeof(VAAPI_DEFAULT_DRIVER_NAME), sizeof(char));
        memcpy(*driver_name, VAAPI_DEFAULT_DRIVER_NAME, sizeof(VAAPI_DEFAULT_DRIVER_NAME));
    }

    return VA_STATUS_SUCCESS;
}

VADisplay vaGetDisplayWin32(
    /* Can be null for adapter autoselection in the VA driver */
    const LUID* adapter_luid
)
{
    VADisplayContextP pDisplayContext;
    VADriverContextP  pDriverContext;

    pDisplayContext = va_newDisplayContext();
    if (!pDisplayContext)
        return NULL;

    pDisplayContext->vaIsValid       = va_DisplayContextIsValid;
    pDisplayContext->vaDestroy       = va_DisplayContextDestroy;
    pDisplayContext->vaGetDriverNameByIndex = va_DisplayContextGetDriverNameByIndex;
    pDisplayContext->vaGetNumCandidates = va_DisplayContextGetNumCandidates;
    pDisplayContext->opaque = calloc(1, sizeof(VADisplayContextWin32));
    if (!pDisplayContext->opaque) {
        va_DisplayContextDestroy(pDisplayContext);
        return NULL;
    }

    VADisplayContextWin32* pWin32Ctx = (VADisplayContextWin32*) pDisplayContext->opaque;
    if (adapter_luid) {
        /* Copy LUID information to display context */
        memcpy(&pWin32Ctx->adapter_luid, adapter_luid, sizeof(pWin32Ctx->adapter_luid));

        /* Load the preferred driver name from the driver registry if available */
        LoadDriverNameFromRegistry(pWin32Ctx);
        if (pWin32Ctx->registry_driver_available_flag) {
            fprintf(stderr, "VA_Win32: Found driver %s in the registry for LUID %ld %ld \n", pWin32Ctx->registry_driver_name, pWin32Ctx->adapter_luid.LowPart, pWin32Ctx->adapter_luid.HighPart);
        } else {
            fprintf(stderr, "VA_Win32: Couldn't find a driver in the registry for LUID %ld %ld. Using default driver: %s \n", pWin32Ctx->adapter_luid.LowPart, pWin32Ctx->adapter_luid.HighPart, VAAPI_DEFAULT_DRIVER_NAME);
        }
    }

    pDriverContext = va_newDriverContext(pDisplayContext);
    if (!pDriverContext) {
        va_DisplayContextDestroy(pDisplayContext);
        return NULL;
    }

    pDriverContext->display_type = VA_DISPLAY_WIN32;

    if (adapter_luid) {
        /* Copy LUID information to driver context */
        pDriverContext->native_dpy   = calloc(1, sizeof(*adapter_luid));
        memcpy(pDriverContext->native_dpy, adapter_luid, sizeof(*adapter_luid));
    }

    return (VADisplay)pDisplayContext;
}
