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

#ifndef _H_QXL_DRIVER
#define _H_QXL_DRIVER

#include "qxl_dev.h"

#if (WINVER < 0x0501)
#include "wdmhelper.h"
#endif

enum {
    FIRST_AVIL_IOCTL_FUNC = 0x800,
    QXL_GET_INFO_FUNC = FIRST_AVIL_IOCTL_FUNC
};

#define IOCTL_QXL_GET_INFO \
    CTL_CODE(FILE_DEVICE_VIDEO, QXL_GET_INFO_FUNC, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define QXL_DRIVER_INFO_VERSION 2

typedef struct QXLDriverInfo {
    UINT32 version;
    QXLCommandRing *cmd_ring;
    QXLCursorRing *cursor_ring;
    QXLReleaseRing *release_ring;
    UINT32 notify_cmd_port;
    UINT32 notify_cursor_port;
    UINT32 notify_oom_port;
    PEVENT display_event;
    PEVENT cursor_event;
    PEVENT sleep_event;

    UINT32 num_io_pages;
    void *io_pages_virt;
    UINT64 io_pages_phys;

    UINT8 *draw_area;
    UINT32 draw_area_size;

    UINT32 *update_id;
    UINT32 *compression_level;

    UINT32 update_area_port;
    Rect *update_area;

    UINT32 *mm_clock;

    UINT32 log_port;
    UINT8 *log_buf;
    UINT32 *log_level;
#if (WINVER < 0x0501)
    PQXLWaitForEvent WaitForEvent;
#endif
} QXLDriverInfo;


#endif

