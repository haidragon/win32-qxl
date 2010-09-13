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
  return info - pdev->Res->surfaces_info;
}

static _inline SurfaceInfo *GetSurfaceInfo(PDev *pdev, UINT32 id)
{
  if (id == 0) {
    return &pdev->surface0_info;
  }
  return &pdev->Res->surfaces_info[id];
}

static _inline UINT32 GetSurfaceId(SURFOBJ *surf)
{
    SurfaceInfo *surface;

    surface = (SurfaceInfo *)surf->dhsurf;
    return GetSurfaceIdFromInfo(surface);
}

static _inline void FreeSurface(PDev *pdev, UINT32 surface_id)
{
    SurfaceInfo *surface;

    if (surface_id == 0) {
        return;
    }
    surface = &pdev->Res->surfaces_info[surface_id];
    surface->draw_area.base_mem = NULL; /* Mark as not used */
    surface->u.next_free = pdev->Res->free_surfaces;
    pdev->Res->free_surfaces = surface;
}


static UINT32 GetFreeSurface(PDev *pdev)
{
    UINT32 x;
    SurfaceInfo *surface;

    surface = pdev->Res->free_surfaces;
    if (surface == NULL) {
        return 0;
    }

    pdev->Res->free_surfaces = surface->u.next_free;

    return surface - pdev->Res->surfaces_info;
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
