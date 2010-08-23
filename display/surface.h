#ifndef SURFACE_H
#define SURFACE_H

#include "qxldd.h"

static _inline UINT32 GetSurfaceIdFromInfo(SurfaceInfo *info)
{
  PDev *pdev;

  pdev = info->u.pdev;
  if (info == &pdev->surface0_info) {
    return 0;
  }
  return info - pdev->Res.surfaces_info;
}

static _inline SurfaceInfo *GetSurfaceInfo(PDev *pdev, UINT32 id)
{
  if (id == 0) {
    return &pdev->surface0_info;
  }
  return &pdev->Res.surfaces_info[id];
}

static _inline UINT32 GetSurfaceId(SURFOBJ *surf)
{
    SurfaceInfo *surface;

    surface = (SurfaceInfo *)surf->dhsurf;
    return GetSurfaceIdFromInfo(surface);
}

static _inline void FreeSurface(PDev *pdev, UINT32 surface_id)
{
   pdev->Res.surfaces_used[surface_id] = 0;
}


static UINT32 GetFreeSurface(PDev *pdev)
{
    UINT32 x;

    //not effective, fix me
    for (x = 1; x < pdev->n_surfaces; ++x) {
        if (!pdev->Res.surfaces_used[x]) {
            pdev->Res.surfaces_used[x] = 1;
            return x;
        }
    }

    return 0;
}

enum {
    DEVICE_BITMAP_ALLOCATION_TYPE_SURF0,
    DEVICE_BITMAP_ALLOCATION_TYPE_DEVRAM,
    DEVICE_BITMAP_ALLOCATION_TYPE_VRAM,
};

BOOL CreateDrawArea(PDev *pdev, UINT8 *base_mem, ULONG format, UINT32 cx, UINT32 cy, UINT32 stride,
                    UINT32 surface_id);
VOID FreeDrawArea(DrawArea *drawarea);

HBITMAP CreateDeviceBitmap(PDev *pdev, SIZEL size, ULONG format, QXLPHYSICAL *phys_mem,
                           UINT8 **base_mem, UINT32 surface_id, UINT8 allocation_type);
VOID DeleteDeviceBitmap(PDev *pdev, UINT32 surface_id, UINT8 allocation_type);

#endif
