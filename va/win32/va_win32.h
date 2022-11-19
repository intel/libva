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

#ifndef _VA_WIN32_H_
#define _VA_WIN32_H_

#include <va/va.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \brief HANDLE memory type
 *
 * The HANDLE type can be used with D3D12 sharing resource functions such as:
 * https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-opensharedhandle
 * https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createsharedhandle
 *
 * When used with VASurfaceAttribExternalBufferDescriptor for vaCreateSurfaces(...,num_surfaces,...),
 * the associated VAGenericValueTypePointer is of type HANDLE[num_surfaces]
 * to create each of num_surfaces from the corresponding HANDLE element
 *
 * When used for vaAcquire/ReleaseBufferHandle VABufferInfo.handle type is HANDLE
 * When used for vlVaExportSurfaceHandle, descriptor parameter type is HANDLE*
 */
#define VA_SURFACE_ATTRIB_MEM_TYPE_NTHANDLE       0x00010000
/** \brief ID3D12Resource memory type
 * When used with VASurfaceAttribExternalBufferDescriptor for vaCreateSurfaces(...,num_surfaces,...),
 * the associated VAGenericValueTypePointer is of type ID3D12Resource*[num_surfaces]
 * to create each of num_surfaces from the corresponding ID3D12Resource* element
 *
 * When used for vaAcquire/ReleaseBufferHandle VABufferInfo.handle type is ID3D12Resource*
 * When used for vlVaExportSurfaceHandle, descriptor parameter type is ID3D12Resource**
 */
#define VA_SURFACE_ATTRIB_MEM_TYPE_D3D12_RESOURCE 0x00020000

/*
 * Returns a suitable VADisplay for VA API
 */
VADisplay vaGetDisplayWin32(
    /* Can be null for adapter autoselection in the VA driver */
    const LUID* adapter_luid
);

#ifdef __cplusplus
}
#endif

#endif /* _VA_WIN32_H_ */
