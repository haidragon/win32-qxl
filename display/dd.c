#include <ddrawi.h>
#include <ddraw.h>
#include <dxmini.h>
#include "os_dep.h"
#include "devioctl.h"
#include "ntddvdeo.h"
#include "ioaccess.h"
#include "qxldd.h"

static UINT8 get_depth(PDev *pdev)
{
    if (pdev->bitmap_format == BMF_32BPP) {
        return 32;
    } else {
        return 16;
    }
}

BOOL DrvGetDirectDrawInfo(DHPDEV dhpdev, DD_HALINFO *pHallInfo,
                          DWORD *pdvNumHeaps, VIDEOMEMORY *pvmList,
                          DWORD *pdvNumFourCCCodes,
                          DWORD *pdvFourCC)
{
    PDev *pdev;
    DWORD offset;

    pdev = (PDev *)dhpdev;

    *pdvNumHeaps = 1;
    *pdvNumFourCCCodes = 0;

    if (!pdev->dd_slot_initialized) {
        return FALSE;
    }

    offset = pdev->resolution.cy * pdev->stride;

    if (pvmList) {
        VIDEOMEMORY *pvmobj = pvmList;

        pvmobj->dwFlags = VIDMEM_ISLINEAR;

        pvmobj->fpStart = (FLATPTR)pdev->fb;
        pvmobj->fpEnd = pvmobj->fpStart + pdev->fb_size - 1;

        pvmobj->ddsCaps.dwCaps = 0;
        pvmobj->ddsCapsAlt.dwCaps = 0;

        pdev->pvmList = pvmList;
    }

    memset(pHallInfo, 0, sizeof(DD_HALINFO));

    pHallInfo->vmiData.pvPrimary =  pdev->fb;
    pHallInfo->vmiData.fpPrimary = 0;

    pHallInfo->dwSize = sizeof (DD_HALINFO);

    pHallInfo->vmiData.dwFlags = 0;
    pHallInfo->vmiData.dwDisplayWidth = pdev->resolution.cx;
    pHallInfo->vmiData.dwDisplayHeight = pdev->resolution.cy;
    pHallInfo->vmiData.lDisplayPitch = pdev->stride;
    pHallInfo->vmiData.ddpfDisplay.dwSize = sizeof(DDPIXELFORMAT);
    pHallInfo->vmiData.ddpfDisplay.dwFlags = DDPF_RGB;

    pHallInfo->vmiData.ddpfDisplay.dwRGBBitCount = get_depth(pdev);

    pHallInfo->vmiData.ddpfDisplay.dwRBitMask = pdev->red_mask;
    pHallInfo->vmiData.ddpfDisplay.dwGBitMask = pdev->green_mask;
    pHallInfo->vmiData.ddpfDisplay.dwBBitMask = pdev->blue_mask;

    pHallInfo->vmiData.dwOffscreenAlign = 4;
    pHallInfo->vmiData.dwOverlayAlign = 4;
    pHallInfo->vmiData.dwTextureAlign = 4;
    pHallInfo->vmiData.dwZBufferAlign = 4;
    pHallInfo->vmiData.dwAlphaAlign = 4;

    pHallInfo->ddCaps.dwSize = sizeof (DDCORECAPS);
    pHallInfo->ddCaps.dwVidMemTotal = pdev->fb_size;
    pHallInfo->ddCaps.dwVidMemFree = pdev->fb_size;

    pdev->dd_initialized = TRUE;

    return TRUE;
}

DWORD CALLBACK QxlCanCreateSurface(PDD_CANCREATESURFACEDATA data)
{
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD CALLBACK QxlFlip(PDD_FLIPDATA lpFlip)
{
    return DDHAL_DRIVER_NOTHANDLED;
}

BOOL DrvEnableDirectDraw(DHPDEV dhpdev, DD_CALLBACKS *pCallBacks,
                         DD_SURFACECALLBACKS *pSurfaceCallBacks,
                         DD_PALETTECALLBACKS *pPaletteCallBacks)
{
    memset(pCallBacks, 0, sizeof (DD_CALLBACKS));
    memset(pSurfaceCallBacks, 0, sizeof (DD_SURFACECALLBACKS));
    memset(pPaletteCallBacks, 0, sizeof (DD_PALETTECALLBACKS));

    pCallBacks->dwSize = sizeof (DD_CALLBACKS);
    pCallBacks->CanCreateSurface = QxlCanCreateSurface;

    pSurfaceCallBacks->dwSize = sizeof (DD_SURFACECALLBACKS);
    pSurfaceCallBacks->Flip = QxlFlip;

    pPaletteCallBacks->dwSize = sizeof (DD_PALETTECALLBACKS);

    return TRUE;
}

void DrvDisableDirectDraw(DHPDEV dhpdev)
{
    PDev *pdev;

    pdev = (PDev *)dhpdev;
    pdev->dd_initialized = FALSE;
}
