/*
 * Copyright (C) 2010 Splitted-Desktop Systems. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <X11/Xlib.h>

#define ADL_OK 0
#define ADL_MAX_PATH 256

/*
 * Based on public AMD Display Library (ADL) SDK:
 * <http://developer.amd.com/gpu/adlsdk/Pages/default.aspx>
 */
typedef struct AdapterInfo {
    int iSize;
    int iAdapterIndex;
    char strUDID[ADL_MAX_PATH]; 
    int iBusNumber;
    int iDeviceNumber;
    int iFunctionNumber;
    int iVendorID;
    char strAdapterName[ADL_MAX_PATH];
    char strDisplayName[ADL_MAX_PATH];
    int iPresent;
    int iXScreenNum;
    int iDrvIndex;
    char strXScreenConfigName[ADL_MAX_PATH];
} AdapterInfo, *LPAdapterInfo;

typedef struct XScreenInfo {
    int iXScreenNum;
    char strXScreenConfigName[ADL_MAX_PATH];
} XScreenInfo, *LPXScreenInfo;

typedef void *(*ADL_MAIN_MALLOC_CALLBACK)(int);
typedef int (*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
typedef int (*ADL_MAIN_CONTROL_DESTROY)(void);
typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int *);
typedef int (*ADL_ADAPTER_ADAPTERINFO_GET)(LPAdapterInfo, int);
typedef int (*ADL_ADAPTER_XSCREENINFO_GET)(LPXScreenInfo, int);

static void *ADL_Main_Memory_Alloc(int iSize)
{
    return malloc(iSize);
}

static void ADL_Main_Memory_Free(void *arg)
{
    void ** const lpBuffer = arg;

    if (lpBuffer && *lpBuffer) {
        free(*lpBuffer);
        *lpBuffer = NULL;
    }
}

static int get_display_name_length(const char *name)
{
    const char *m;

    if (!name)
        return 0;

    /* Strip out screen number */
    m = strchr(name, ':');
    if (m) {
        m = strchr(m, '.');
        if (m)
            return m - name;
    }
    return strlen(name);
}

static int match_display_name(Display *x11_dpy, const char *display_name)
{
    Display *test_dpy;
    char *test_dpy_name, *x11_dpy_name;
    int test_dpy_namelen, x11_dpy_namelen;
    int m;

    test_dpy = XOpenDisplay(display_name);
    if (!test_dpy)
        return 0;

    test_dpy_name    = XDisplayString(test_dpy);
    test_dpy_namelen = get_display_name_length(test_dpy_name);
    x11_dpy_name     = XDisplayString(x11_dpy);
    x11_dpy_namelen  = get_display_name_length(x11_dpy_name);

    m = (test_dpy_namelen == x11_dpy_namelen &&
         (test_dpy_namelen == 0 ||
          (test_dpy_namelen > 0 &&
           strncmp(test_dpy_name, x11_dpy_name, test_dpy_namelen) == 0)));

    XCloseDisplay(test_dpy);
    return m;
}

Bool VA_FGLRXGetClientDriverName( Display *dpy, int screen,
    int *ddxDriverMajorVersion, int *ddxDriverMinorVersion,
    int *ddxDriverPatchVersion, char **clientDriverName )
{
    ADL_MAIN_CONTROL_CREATE          ADL_Main_Control_Create;
    ADL_MAIN_CONTROL_DESTROY         ADL_Main_Control_Destroy;
    ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get;
    ADL_ADAPTER_ADAPTERINFO_GET      ADL_Adapter_AdapterInfo_Get;
    ADL_ADAPTER_XSCREENINFO_GET      ADL_Adapter_XScreenInfo_Get;

    LPAdapterInfo lpAdapterInfo = NULL;
    LPXScreenInfo lpXScreenInfo = NULL;
    void *libadl_handle = NULL;
    Bool success = False;
    int is_adl_initialized = 0;
    int i, num_adapters, lpAdapterInfo_size, lpXScreenInfo_size;

    if (ddxDriverMajorVersion)
        *ddxDriverMajorVersion = 0;
    if (ddxDriverMinorVersion)
        *ddxDriverMinorVersion = 0;
    if (ddxDriverPatchVersion)
        *ddxDriverPatchVersion = 0;
    if (clientDriverName)
        *clientDriverName = NULL;

    libadl_handle = dlopen("libatiadlxx.so", RTLD_LAZY|RTLD_GLOBAL);
    if (!libadl_handle)
        goto end;

    dlerror();
    ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE)
        dlsym(libadl_handle,"ADL_Main_Control_Create");
    if (dlerror() || !ADL_Main_Control_Create)
        goto end;

    ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY)
        dlsym(libadl_handle,"ADL_Main_Control_Destroy");
    if (dlerror() || !ADL_Main_Control_Destroy)
        goto end;

    ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET)
        dlsym(libadl_handle,"ADL_Adapter_NumberOfAdapters_Get");
    if (dlerror() || !ADL_Adapter_NumberOfAdapters_Get)
        goto end;

    ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET)
        dlsym(libadl_handle,"ADL_Adapter_AdapterInfo_Get");
    if (dlerror() || !ADL_Adapter_AdapterInfo_Get)
        goto end;

    ADL_Adapter_XScreenInfo_Get = (ADL_ADAPTER_XSCREENINFO_GET)
        dlsym(libadl_handle,"ADL_Adapter_XScreenInfo_Get");
    if (dlerror() || !ADL_Adapter_XScreenInfo_Get)
        goto end;

    if (ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1) != ADL_OK)
        goto end;
    is_adl_initialized = 1;

    if (ADL_Adapter_NumberOfAdapters_Get(&num_adapters) != ADL_OK)
        goto end;
    if (num_adapters <= 0)
        goto end;

    lpAdapterInfo_size = num_adapters * sizeof(*lpAdapterInfo);
    lpAdapterInfo = ADL_Main_Memory_Alloc(lpAdapterInfo_size);
    if (!lpAdapterInfo)
        goto end;
    memset(lpAdapterInfo, 0, lpAdapterInfo_size);

    for (i = 0; i < num_adapters; i++)
        lpAdapterInfo[i].iSize = sizeof(lpAdapterInfo[i]);

    lpXScreenInfo_size = num_adapters * sizeof(*lpXScreenInfo);
    lpXScreenInfo = ADL_Main_Memory_Alloc(lpXScreenInfo_size);
    if (!lpXScreenInfo)
        goto end;
    memset(lpXScreenInfo, 0, lpXScreenInfo_size);

    if (ADL_Adapter_AdapterInfo_Get(lpAdapterInfo, lpAdapterInfo_size) != ADL_OK)
        goto end;

    if (ADL_Adapter_XScreenInfo_Get(lpXScreenInfo, lpXScreenInfo_size) != ADL_OK)
        goto end;

    for (i = 0; i < num_adapters; i++) {
        LPXScreenInfo const lpCurrXScreenInfo = &lpXScreenInfo[i];
        LPAdapterInfo const lpCurrAdapterInfo = &lpAdapterInfo[i];
        if (!lpCurrAdapterInfo->iPresent)
            continue;
#if 0
        printf("Adapter %d:\n", i);
        printf("  iAdapterIndex: %d\n",    lpCurrAdapterInfo->iAdapterIndex);
        printf("  strUDID: '%s'\n",        lpCurrAdapterInfo->strUDID);
        printf("  iBusNumber: %d\n",       lpCurrAdapterInfo->iBusNumber);
        printf("  iDeviceNumber: %d\n",    lpCurrAdapterInfo->iDeviceNumber);
        printf("  iFunctionNumber: %d\n",  lpCurrAdapterInfo->iFunctionNumber);
        printf("  iVendorID: 0x%04x\n",    lpCurrAdapterInfo->iVendorID);
        printf("  strAdapterName: '%s'\n", lpCurrAdapterInfo->strAdapterName);
        printf("  strDisplayName: '%s'\n", lpCurrAdapterInfo->strDisplayName);
        printf("  iPresent: %d\n",         lpCurrAdapterInfo->iPresent);
        printf("  iXScreenNum: %d\n",      lpCurrXScreenInfo->iXScreenNum);
#endif
        if (screen == lpCurrXScreenInfo->iXScreenNum &&
            match_display_name(dpy, lpCurrAdapterInfo->strDisplayName)) {
            if (clientDriverName)
                *clientDriverName = strdup("fglrx");
            success = True;
            break;
        }
    }

end:
    if (lpXScreenInfo)
        ADL_Main_Memory_Free(&lpXScreenInfo);
    if (lpAdapterInfo)
        ADL_Main_Memory_Free(&lpAdapterInfo);
    if (is_adl_initialized)
        ADL_Main_Control_Destroy();
    if (libadl_handle)
        dlclose(libadl_handle);
    return success;
}
