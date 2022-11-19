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
const char VAAPI_REG_SUB_KEY[] = "VAAPIDriverName";
const char VAAPI_REG_SUB_KEY_WOW[] = "VAAPIDriverNameWow";

inline const char* GetVAAPIRegKeyName()
{
#ifdef _WIN64
    return VAAPI_REG_SUB_KEY;
#else
    BOOL is_wow64 = FALSE;
    if (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)
        return VAAPI_REG_SUB_KEY_WOW;
    return VAAPI_REG_SUB_KEY;
#endif
}

typedef struct _VADisplayContextWin32 {
    LUID adapter_luid;
    char registry_driver_name[MAX_PATH];
    uint8_t registry_driver_available_flag;
} VADisplayContextWin32;

static bool TryLoadDriverNameFromRegistry(const LUID* adapter_luid, VADisplayContextWin32* pWin32Ctx)
{
    if (!adapter_luid)
        return false;

    /* Get handle to GDI Runtime */
    HMODULE h = LoadLibrary("gdi32.dll");
    if (h == NULL)
        return false;

    bool ret = false;
    int result = 0;
    /* Check the OS version */
    if (GetProcAddress(h, "D3DKMTSubmitPresentBltToHwQueue")) {
        D3DKMT_ENUMADAPTERS2 EnumAdapters;
        NTSTATUS status = STATUS_SUCCESS;

        EnumAdapters.NumAdapters = 0;
        EnumAdapters.pAdapters = NULL;
        PFND3DKMT_ENUMADAPTERS2 pEnumAdapters2 = (PFND3DKMT_ENUMADAPTERS2)GetProcAddress(h, "D3DKMTEnumAdapters2");
        if (!pEnumAdapters2) {
            fprintf(stderr, "VA_Win32: GetProcAddress failed for D3DKMTEnumAdapters2\n");
            goto out;
        }
        PFND3DKMT_QUERYADAPTERINFO pQueryAdapterInfo = (PFND3DKMT_QUERYADAPTERINFO)GetProcAddress(h, "D3DKMTQueryAdapterInfo");
        if (!pQueryAdapterInfo) {
            fprintf(stderr, "VA_Win32: GetProcAddress failed for D3DKMTQueryAdapterInfo\n");
            goto out;
        }
        while (1) {
            EnumAdapters.NumAdapters = 0;
            EnumAdapters.pAdapters = NULL;
            status = pEnumAdapters2(&EnumAdapters);
            if (status == STATUS_BUFFER_TOO_SMALL) {
                /* Number of Adapters increased between calls, retry; */
                continue;
            } else if (!NT_SUCCESS(status)) {
                fprintf(stderr, "VA_Win32: D3DKMTEnumAdapters2 status != SUCCESS\n");
                goto out;
            }
            break;
        }
        EnumAdapters.pAdapters = malloc(sizeof(*EnumAdapters.pAdapters) * (EnumAdapters.NumAdapters));
        if (EnumAdapters.pAdapters == NULL) {
            fprintf(stderr, "VA_Win32: Allocation failure for adapters buffer\n");
            goto out;
        }
        status = pEnumAdapters2(&EnumAdapters);
        if (!NT_SUCCESS(status)) {
            fprintf(stderr, "VA_Win32: D3DKMTEnumAdapters2 status != SUCCESS\n");
            goto out;
        }
        const char* cszVAAPIRegKeyName = GetVAAPIRegKeyName();
        const int szVAAPIRegKeyName = (int)(strlen(cszVAAPIRegKeyName) + 1) * sizeof(cszVAAPIRegKeyName[0]);
        for (UINT AdapterIndex = 0; AdapterIndex < EnumAdapters.NumAdapters; AdapterIndex++) {
            D3DDDI_QUERYREGISTRY_INFO queryArgs = {0};
            D3DDDI_QUERYREGISTRY_INFO* pQueryArgs = &queryArgs;
            D3DDDI_QUERYREGISTRY_INFO* pQueryBuffer = NULL;
            queryArgs.QueryType = D3DDDI_QUERYREGISTRY_ADAPTERKEY;
            queryArgs.QueryFlags.TranslatePath = TRUE;
            queryArgs.ValueType = REG_SZ;
            result = MultiByteToWideChar(
                         CP_ACP,
                         0,
                         cszVAAPIRegKeyName,
                         szVAAPIRegKeyName,
                         queryArgs.ValueName,
                         ARRAYSIZE(queryArgs.ValueName));
            if (!result) {
                fprintf(stderr, "VA_Win32: MultiByteToWideChar status != SUCCESS\n");
                continue;
            }
            D3DKMT_QUERYADAPTERINFO queryAdapterInfo = {0};
            queryAdapterInfo.hAdapter = EnumAdapters.pAdapters[AdapterIndex].hAdapter;
            queryAdapterInfo.Type = KMTQAITYPE_QUERYREGISTRY;
            queryAdapterInfo.pPrivateDriverData = &queryArgs;
            queryAdapterInfo.PrivateDriverDataSize = sizeof(queryArgs);
            status = pQueryAdapterInfo(&queryAdapterInfo);
            if (!NT_SUCCESS(status)) {
                /* Try a different value type.  Some vendors write the key as a multi-string type. */
                queryArgs.ValueType = REG_MULTI_SZ;
                status = pQueryAdapterInfo(&queryAdapterInfo);
                if (NT_SUCCESS(status)) {
                    fprintf(stderr, "VA_Win32: Accepting multi-string registry key type\n");
                } else {
                    /* Continue trying to get as much info on each adapter as possible. */
                    /* It's too late to return FALSE and claim WDDM2_4 enumeration is not available here. */
                    continue;
                }
            }
            if (NT_SUCCESS(status) && pQueryArgs->Status == D3DDDI_QUERYREGISTRY_STATUS_BUFFER_OVERFLOW) {
                ULONG queryBufferSize = sizeof(D3DDDI_QUERYREGISTRY_INFO) + queryArgs.OutputValueSize;
                pQueryBuffer = malloc(queryBufferSize);
                if (pQueryBuffer == NULL)
                    continue;
                memcpy(pQueryBuffer, &queryArgs, sizeof(D3DDDI_QUERYREGISTRY_INFO));
                queryAdapterInfo.pPrivateDriverData = pQueryBuffer;
                queryAdapterInfo.PrivateDriverDataSize = queryBufferSize;
                status = pQueryAdapterInfo(&queryAdapterInfo);
                pQueryArgs = pQueryBuffer;
            }
            if (NT_SUCCESS(status) && pQueryArgs->Status == D3DDDI_QUERYREGISTRY_STATUS_SUCCESS) {
                wchar_t* pWchar = pQueryArgs->OutputString;
                memset(pWin32Ctx->registry_driver_name, 0, sizeof(pWin32Ctx->registry_driver_name));
                size_t len;
                wcstombs_s(&len, pWin32Ctx->registry_driver_name, sizeof(pWin32Ctx->registry_driver_name), pWchar, sizeof(pWin32Ctx->registry_driver_name));
                assert(len <= (sizeof(pWin32Ctx->registry_driver_name) - 1));
                if (0 == memcmp(adapter_luid, &EnumAdapters.pAdapters[AdapterIndex].AdapterLuid, sizeof(EnumAdapters.pAdapters[AdapterIndex].AdapterLuid))) {
                    ret = true;
                    /* Found the adapter we wanted, finish iteration. Associated driver string is loaded in pWin32Ctx->registry_driver_name */
                    /* VAAPI needs the stripped driver filename, without the absolute path or the extension. */

                    /* Zero-out the extension section with null terminators if present */
                    char* pExtPosition = strstr(pWin32Ctx->registry_driver_name, ".dll");
                    if (pExtPosition)
                        memset(pExtPosition, '\0', 4 /*size of the 4 chars of .dll*/);

                    free(pQueryBuffer);
                    goto out;
                }
            } else if (status == (NTSTATUS)STATUS_INVALID_PARAMETER) {
                free(pQueryBuffer);
                goto out;
            }
            free(pQueryBuffer);
        }
out:
        free(EnumAdapters.pAdapters);
    }

    FreeLibrary(h);

    return ret;
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

    VADisplayContextWin32* pWin32Ctx = (VADisplayContextWin32*) pDisplayContext->opaque;
    if (adapter_luid) {
        /* Copy LUID information to display context */
        memcpy(&pWin32Ctx->adapter_luid, adapter_luid, sizeof(pWin32Ctx->adapter_luid));

        /* Load the preferred driver name from the driver registry if available */
        pWin32Ctx->registry_driver_available_flag = TryLoadDriverNameFromRegistry(&pWin32Ctx->adapter_luid, pWin32Ctx) ? 1 : 0;
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
