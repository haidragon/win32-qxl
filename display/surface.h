#ifndef SURFACE_H
#define SURFACE_H

enum {
    DEVICE_BITMAP_ALLOCATION_TYPE_SURF0,
};

typedef struct DrawArea {
   HSURF bitmap;
   SURFOBJ* surf_obj;
} DrawArea;

BOOL CreateDrawArea(PDev *pdev, DrawArea *drawarea, UINT8 *base_mem, UINT32 cx, UINT32 cy);
VOID FreeDrawArea(DrawArea *drawarea);

HBITMAP CreateDeviceBitmap(PDev *pdev, SIZEL size, ULONG format, PHYSICAL *phys_mem,
                           UINT8 **base_mem, UINT8 allocation_type);
VOID DeleteDeviceBitmap(HSURF surf);

#endif
