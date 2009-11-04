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

#ifndef _H_RES
#define _H_RES

#include "qxldd.h"

UINT64 ReleaseOutput(PDev *pdev, UINT64 output_id);

QXLDrawable *Drawable(PDev *pdev, UINT8 type, RECTL *area, CLIPOBJ *clip);
void PushDrawable(PDev *pdev, QXLDrawable *drawable);

BOOL QXLGetPath(PDev *pdev, QXLDrawable *drawable, PHYSICAL *path_phys, PATHOBJ *path);
BOOL QXLGetMask(PDev *pdev, QXLDrawable *drawable, QMask *qxl_mask, SURFOBJ *mask, POINTL *pos,
                BOOL invers, LONG width, LONG height);
BOOL QXLGetBrush(PDev *pdev, QXLDrawable *drawable, Brush *qxl_brush,
                            BRUSHOBJ *brush, POINTL *brush_pos);
BOOL QXLGetBitmap(PDev *pdev, QXLDrawable *drawable, PHYSICAL *image_phys, SURFOBJ *surf,
                  Rect *area, XLATEOBJ *color_trans, UINT32 *hash_key, BOOL use_cache);
BOOL QXLGetBitsFromCache(PDev *pdev, QXLDrawable *drawable, UINT32 hash_key, PHYSICAL *image_phys);
BOOL QXLGetAlphaBitmap(PDev *pdev, QXLDrawable *drawable, PHYSICAL *image_phys, SURFOBJ *surf,
                       Rect *area);
UINT8 *QXLGetBuf(PDev *pdev, QXLDrawable *drawable, PHYSICAL *buf_phys, UINT32 size);
BOOL QXLGetStr(PDev *pdev, QXLDrawable *drawable, PHYSICAL *str_phys, FONTOBJ *font, STROBJ *str);

void UpdateArea(PDev *pdev, RECTL *area);

QXLCursorCmd *CursorCmd(PDev *pdev);
void PushCursorCmd(PDev *pdev, QXLCursorCmd *cursor_cmd);

BOOL GetAlphaCursor(PDev *pdev, QXLCursorCmd *cmd, LONG hot_x, LONG hot_y, SURFOBJ *surf);
BOOL GetColorCursor(PDev *pdev, QXLCursorCmd *cmd, LONG hot_x, LONG hot_y, SURFOBJ *surf,
                    SURFOBJ *mask, XLATEOBJ *color_trans);
BOOL GetMonoCursor(PDev *pdev, QXLCursorCmd *cmd, LONG hot_x, LONG hot_y, SURFOBJ *surf);
BOOL GetTransparentCursor(PDev *pdev, QXLCursorCmd *cmd);

BOOL ResInit(PDev *pdev);
void ResDestroy(PDev *pdev);
void ResInitGlobals();
void ResDestroyGlobals();
#endif
