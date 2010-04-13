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

#ifndef _H_ROP
#define _H_ROP

#define ROP3_DEST (1 << 0)
#define ROP3_SRC (1 << 1)
#define ROP3_BRUSH (1 << 2)
#define ROP3_ALL (ROP3_DEST | ROP3_SRC | ROP3_BRUSH)

typedef struct ROP3Info {
    UINT8 effect;
    UINT8 flags;
    UINT32 method_type;
    UINT16 method_data;
} ROP3Info;

extern ROP3Info rops2[];

BOOL BitBltFromDev(PDev *pdev, SURFOBJ *src, SURFOBJ *dest, SURFOBJ *mask, CLIPOBJ *clip,
                   XLATEOBJ *color_trans, RECTL *dest_rect, POINTL src_pos,
                   POINTL *mask_pos, BRUSHOBJ *brush, POINTL *brush_pos, ROP4 rop4);
#endif
