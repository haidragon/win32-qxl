/*
   Copyright (C) 2009 Red Hat, Inc.

   This software is licensed under the GNU General Public License,
   version 2 (GPLv2) (see COPYING for details), subject to the
   following clarification.

   With respect to binaries built using the Microsoft(R) Windows
   Driver Kit (WDK), GPLv2 does not extend to any code contained in or
   derived from the WDK ("WDK Code").  As to WDK Code, by using or
   distributing such binaries you agree to be bound by the Microsoft
   Software License Terms for the WDK.  All WDK Code is considered by
   the GPLv2 licensors to qualify for the special exception stated in
   section 3 of GPLv2 (commonly known as the system library
   exception).

   There is NO WARRANTY for this software, express or implied,
   including the implied warranties of NON-INFRINGEMENT, TITLE,
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "stddef.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "os_dep.h"

#include "winerror.h"
#include "windef.h"
#include "wingdi.h"
#include "winddi.h"
#include "devioctl.h"
#include "ntddvdeo.h"
#include "ioaccess.h"

#include "qxldd.h"
#include "utils.h"
#include "mspace.h"
#include "res.h"
#include "surface.h"

static BOOL CreateDrawArea(PDev *pdev, UINT8 *base_mem, ULONG format, UINT32 cx, UINT32 cy,
                           UINT32 stride, UINT32 surface_id)
{
    SIZEL  size;
    DrawArea *drawarea;

    size.cx = cx;
    size.cy = cy;

    drawarea = &GetSurfaceInfo(pdev, surface_id)->draw_area;

    if (!(drawarea->bitmap = (HSURF)EngCreateBitmap(size, stride, format, 0, base_mem))) {
        DEBUG_PRINT((pdev, 0, "%s: EngCreateBitmap failed\n", __FUNCTION__));
        return FALSE;
    }

    if (!EngAssociateSurface(drawarea->bitmap, pdev->eng, 0)) {
        DEBUG_PRINT((pdev, 0, "%s: EngAssociateSurface failed\n", __FUNCTION__));
        goto error;
    }

    if (!(drawarea->surf_obj = EngLockSurface(drawarea->bitmap))) {
        DEBUG_PRINT((pdev, 0, "%s: EngLockSurface failed\n", __FUNCTION__));
        goto error;
    }

    drawarea->base_mem = base_mem;

    return TRUE;
error:
    EngDeleteSurface(drawarea->bitmap);
    return FALSE;
}

static VOID FreeDrawArea(DrawArea *drawarea)
{
    if (drawarea->surf_obj) {
        EngUnlockSurface(drawarea->surf_obj);
        EngDeleteSurface(drawarea->bitmap);
        drawarea->surf_obj = NULL;
    }
}

HBITMAP CreateDeviceBitmap(PDev *pdev, SIZEL size, ULONG format, QXLPHYSICAL *phys_mem,
                           UINT8 **base_mem, UINT32 surface_id, UINT8 allocation_type)
{
    UINT32 surface_format, depth;
    HBITMAP surf;
    UINT32 stride;

    switch (format) {
        case BMF_16BPP:
            surface_format = SPICE_SURFACE_FMT_16_555;
            depth = 16;
            break;
        case BMF_24BPP:
        case BMF_32BPP:
            surface_format = SPICE_SURFACE_FMT_32_xRGB;
            depth = 32;
            break;
        case BMF_8BPP:
        default:
            return 0;
    };

    if (!(surf = EngCreateDeviceBitmap((DHSURF)GetSurfaceInfo(pdev, surface_id), size, format))) {
        DEBUG_PRINT((NULL, 0, "%s: create device surface failed, 0x%lx\n",
                     __FUNCTION__, pdev));
        goto out_error1;
    }

    if (!EngAssociateSurface((HSURF)surf, pdev->eng,  HOOK_SYNCHRONIZE | HOOK_COPYBITS |
                             HOOK_BITBLT | HOOK_TEXTOUT | HOOK_STROKEPATH | HOOK_STRETCHBLT |
                             HOOK_STRETCHBLTROP | HOOK_TRANSPARENTBLT | HOOK_ALPHABLEND
#ifdef CALL_TEST
                             | HOOK_PLGBLT | HOOK_FILLPATH | HOOK_STROKEANDFILLPATH | HOOK_LINETO |
                             HOOK_GRADIENTFILL 
#endif
                             )) {
        DEBUG_PRINT((pdev, 0, "%s: EngAssociateSurface failed\n", __FUNCTION__));
        goto out_error2;
    }

    GetSurfaceInfo(pdev, surface_id)->u.pdev = pdev;

    QXLGetSurface(pdev, phys_mem, size.cx, size.cy, depth,
                  &stride, base_mem, allocation_type);
    if (!*base_mem) {
        goto out_error2;
    }

    if (!CreateDrawArea(pdev, *base_mem, format, size.cx, size.cy, stride, surface_id)) {
        goto out_error3;
    }

    if (allocation_type != DEVICE_BITMAP_ALLOCATION_TYPE_SURF0) {
        QXLSurfaceCmd *surface;

        surface = SurfaceCmd(pdev, QXL_SURFACE_CMD_CREATE, surface_id);
        surface->u.surface_create.format = surface_format;
        surface->u.surface_create.width = size.cx;
        surface->u.surface_create.height = size.cy;
        surface->u.surface_create.stride = -(INT32)stride;
        surface->u.surface_create.data = *phys_mem;
        PushSurfaceCmd(pdev, surface);
    }

    return surf;

out_error3:
    QXLDelSurface(pdev, *base_mem, allocation_type);
out_error2:
    EngDeleteSurface((HSURF)surf);
out_error1:
    return 0;
}

VOID DeleteDeviceBitmap(PDev *pdev, UINT32 surface_id, UINT8 allocation_type)
{
    DrawArea *drawarea;

    drawarea = &GetSurfaceInfo(pdev,surface_id)->draw_area;

    FreeDrawArea(drawarea);

    if (allocation_type != DEVICE_BITMAP_ALLOCATION_TYPE_SURF0 &&
        pdev->Res->surfaces_info[surface_id].draw_area.base_mem != NULL) {
        QXLSurfaceCmd *surface;

        surface = SurfaceCmd(pdev, QXL_SURFACE_CMD_DESTROY, surface_id);
        QXLGetDelSurface(pdev, surface, surface_id, allocation_type);
        PushSurfaceCmd(pdev, surface);
    }
}
