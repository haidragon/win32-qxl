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

BOOL CreateDrawArea(PDev *pdev, DrawArea *drawarea, UINT8 *base_mem, UINT32 cx, UINT32 cy)
{
    SIZEL  size;

    size.cx = cx;
    size.cy = cy;

    if (!(drawarea->bitmap = (HSURF)EngCreateBitmap(size, size.cx << 2, BMF_32BPP, 0, base_mem))) {
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

    return TRUE;
error:
    EngDeleteSurface(drawarea->bitmap);
    return FALSE;
}

VOID FreeDrawArea(DrawArea *drawarea)
{
    EngUnlockSurface(drawarea->surf_obj);
    EngDeleteSurface(drawarea->bitmap);
}

HBITMAP CreateDeviceBitmap(PDev *pdev, SIZEL size, ULONG format, QXLPHYSICAL *phys_mem,
                           UINT8 **base_mem, UINT8 allocation_type)
{
    UINT8 depth;
    HBITMAP surf;

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

    if (!(surf = EngCreateDeviceBitmap((DHSURF)pdev, size, format))) {
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

    if (!QXLGetSurface(pdev, phys_mem, size.cx, size.cy, 32, base_mem, allocation_type)) {
        goto out_error2;
    }

    return surf;

out_error2:
    EngDeleteSurface((HSURF)surf);
out_error1:
    return 0;
}

VOID DeleteDeviceBitmap(HSURF surf)
{
    EngDeleteSurface(surf);
}
