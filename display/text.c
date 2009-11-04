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

#include "os_dep.h"
#include "qxldd.h"
#include "utils.h"
#include "res.h"
#include "rop.h"

BOOL APIENTRY DrvTextOut(SURFOBJ *surf, STROBJ *str, FONTOBJ *font, CLIPOBJ *clip,
                         RECTL *ignored, RECTL *opaque_rect,
                         BRUSHOBJ *fore_brush, BRUSHOBJ *back_brash,
                         POINTL *brushs_origin, MIX mix)
{
    QXLDrawable *drawable;
    ROP3Info *fore_rop;
    ROP3Info *back_rop;
    PDev* pdev;
    RECTL area;

    if (!(pdev = (PDev *)surf->dhpdev)) {
        DEBUG_PRINT((NULL, 0, "%s: err no pdev\n", __FUNCTION__));
        return FALSE;
    }

    CountCall(pdev, CALL_COUNTER_TEXT_OUT);

    DEBUG_PRINT((pdev, 3, "%s\n", __FUNCTION__));
    ASSERT(pdev, opaque_rect == NULL ||
           (opaque_rect->left < opaque_rect->right && opaque_rect->top < opaque_rect->bottom));
    ASSERT(pdev, surf && str && font && clip);

    if (opaque_rect) {
        CopyRect(&area, opaque_rect);
    } else {
        CopyRect(&area, &str->rclBkGround);
    }

    if (clip) {
        if (clip->iDComplexity == DC_TRIVIAL) {
            clip = NULL;
        } else {
            SectRect(&clip->rclBounds, &area, &area);
            if (IsEmptyRect(&area)) {
                DEBUG_PRINT((pdev, 1, "%s: empty rect after clip\n", __FUNCTION__));
                return TRUE;
            }
        }
    }

    if (!(drawable = Drawable(pdev, QXL_DRAW_TEXT, &area, clip))) {
        return FALSE;
    }

    if (opaque_rect) {
        ASSERT(pdev, back_brash && brushs_origin);
        if (!QXLGetBrush(pdev, drawable, &drawable->u.text.back_brush, back_brash, brushs_origin)) {
            goto error;
        }
        CopyRect(&drawable->u.text.back_area, &area);
        drawable->u.text.back_mode = ROPD_OP_PUT;
        drawable->effect = QXL_EFFECT_OPAQUE;
    } else {
        drawable->u.text.back_brush.type = BRUSH_TYPE_NONE;
        RtlZeroMemory(&drawable->u.text.back_area, sizeof(drawable->u.text.back_area));
        drawable->u.text.back_mode = 0;
        drawable->effect = QXL_EFFECT_BLEND;
    }

    fore_rop = &rops2[(mix - 1) & 0x0f];
    back_rop = &rops2[((mix >> 8) - 1) & 0x0f];

    if (!((fore_rop->flags | back_rop->flags) & ROP3_BRUSH)) {
        drawable->u.stroke.brush.type = BRUSH_TYPE_NONE;
    } else if (!QXLGetBrush(pdev, drawable, &drawable->u.text.fore_brush, fore_brush,
                            brushs_origin)) {
        DEBUG_PRINT((pdev, 0, "%s: get brush failed\n", __FUNCTION__));
        goto error;
    }

    if (fore_rop->method_data != back_rop->method_data && back_rop->method_data) {
        DEBUG_PRINT((pdev, 0, "%s: ignoring back rop, fore %u back %u\n",
                     __FUNCTION__,
                     (UINT32)fore_rop->method_data,
                     (UINT32)back_rop->method_data));
    }
    drawable->u.text.fore_mode = fore_rop->method_data;

    if (!QXLGetStr(pdev, drawable, &drawable->u.text.str, font, str)) {
        DEBUG_PRINT((pdev, 0, "%s: get str failed\n", __FUNCTION__));
        goto error;
    }

    PushDrawable(pdev, drawable);
    DEBUG_PRINT((pdev, 4, "%s: done\n", __FUNCTION__));
    return TRUE;

error:
    ReleaseOutput(pdev, drawable->release_info.id);
    DEBUG_PRINT((pdev, 4, "%s: error\n", __FUNCTION__));
    return FALSE;
}
