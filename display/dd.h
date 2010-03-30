#ifndef DD_H
#define DD_H

BOOL DrvGetDirectDrawInfo(DHPDEV dhpdev, DD_HALINFO *pHallInfo,
                          DWORD *pdvNumHeaps, VIDEOMEMORY *pvmList,
                          DWORD *pdvNumFourCCCodes,
                          DWORD *pdvFourCC);

BOOL DrvEnableDirectDraw(DHPDEV dhpdev, DD_CALLBACKS *pCallBacks,
                         DD_SURFACECALLBACKS *pSurfaceCallBacks,
                         DD_PALETTECALLBACKS *pPaletteCallBacks);

void DrvDisableDirectDraw(DHPDEV dhpdev);

#endif
