/*
   Copyright (C) 2009 Red Hat, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

BOOL CreateDrawArea(PDev *pdev, UINT8 *base_mem, UINT32 cx, UINT32 cy, UINT32 stride,
                    UINT32 surface_id)
{
    SIZEL  size;
    DrawArea *drawarea;

    size.cx = cx;
    size.cy = cy;

    drawarea = &pdev->surfaces_info[surface_id].draw_area;

    if (!(drawarea->bitmap = (HSURF)EngCreateBitmap(size, stride, BMF_32BPP, 0, base_mem))) {
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

VOID FreeDrawArea(DrawArea *drawarea)
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
    UINT8 depth;
    HBITMAP surf;
    UINT32 stride;

    switch (format) {
        case BMF_8BPP:
            return 0;
            break;
        case BMF_16BPP:
            depth = 16;
            break;
        case BMF_24BPP:
            depth = 32;
            break;
        case BMF_32BPP:
            depth = 32;
            break;
        default:
            return 0;
    };

    if (!(surf = EngCreateDeviceBitmap((DHSURF)&pdev->surfaces_info[surface_id], size, format))) {
        DEBUG_PRINT((NULL, 0, "%s: create device surface failed, 0x%lx\n",
                     __FUNCTION__, pdev));
        goto out_error1;
    }

    if (allocation_type == DEVICE_BITMAP_ALLOCATION_TYPE_SURF0) {
        if (!EngAssociateSurface((HSURF)surf, pdev->eng,  HOOK_SYNCHRONIZE | HOOK_COPYBITS |
                                 HOOK_BITBLT | HOOK_TEXTOUT | HOOK_STROKEPATH | HOOK_STRETCHBLT |
                                 HOOK_STRETCHBLTROP | HOOK_TRANSPARENTBLT
#ifdef CALL_TEST
                                 | HOOK_PLGBLT | HOOK_FILLPATH | HOOK_STROKEANDFILLPATH | HOOK_LINETO |
                                 HOOK_GRADIENTFILL 
#endif
                                 )) {
            DEBUG_PRINT((pdev, 0, "%s: EngAssociateSurface failed\n", __FUNCTION__));
            goto out_error2;
        }
    } else {
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
    }

    pdev->surfaces_info[surface_id].pdev = pdev;

    QXLGetSurface(pdev, phys_mem, size.cx, size.cy, depth, &stride, base_mem, allocation_type);
    if (!*base_mem) {
        goto out_error2;
    }

    if (!CreateDrawArea(pdev, *base_mem, size.cx, size.cy, stride, surface_id)) {
        goto out_error3;
    }

    if (allocation_type != DEVICE_BITMAP_ALLOCATION_TYPE_SURF0) {
        QXLSurfaceCmd *surface;

        surface = SurfaceCmd(pdev, QXL_SURFACE_CMD_CREATE, surface_id);
        surface->u.surface_create.depth = depth;
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
    FreeSurface(pdev, surface_id);
    EngDeleteSurface((HSURF)surf);
out_error1:
    return 0;
}

VOID DeleteDeviceBitmap(PDev *pdev, UINT32 surface_id, UINT8 allocation_type)
{
    DrawArea *drawarea;

    drawarea = &pdev->surfaces_info[surface_id].draw_area;

    FreeDrawArea(drawarea);

    if (allocation_type != DEVICE_BITMAP_ALLOCATION_TYPE_SURF0 &&
        pdev->Res.surfaces_used[surface_id]) {
        QXLSurfaceCmd *surface;

        surface = SurfaceCmd(pdev, QXL_SURFACE_CMD_DESTROY, surface_id);
        QXLGetDelSurface(pdev, surface, surface_id, allocation_type);
        PushSurfaceCmd(pdev, surface);
    }
}
