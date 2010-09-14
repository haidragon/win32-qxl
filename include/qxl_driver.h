/*
   Copyright (C) 2009 Red Hat, Inc.

   This software is licensed under the GNU General Public License,
   version 2 (GPLv2) (see COPYING for details), subject to the
   following clarification.

   With respect to binaries built using the Microsoft(R) Windows
   Driver Kit (WDK), GPLv2 does not extend to any code contained in or
   derived from the WDK ("WDK Code").  As to WDK Code, by using or
   distributing such binaries you agree to be bound by the Microsoft
   Software License Terms for the WDK.  All WDK Code is considered by
   the GPLv2 licensors to qualify for the special exception stated in
   section 3 of GPLv2 (commonly known as the system library
   exception).

   There is NO WARRANTY for this software, express or implied,
   including the implied warranties of NON-INFRINGEMENT, TITLE,
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef _H_QXL_DRIVER
#define _H_QXL_DRIVER

#include <spice\qxl_dev.h>

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

typedef struct MemSlot {
    UINT8 generation;
    UINT64 start_phys_addr;
    UINT64 end_phys_addr;
    UINT64 start_virt_addr;
    UINT64 end_virt_addr;
} MemSlot;

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

    UINT32 num_pages;
    void *io_pages_virt;
    UINT64 io_pages_phys;

    UINT8 *surface0_area;
    UINT32 surface0_area_size;

    UINT32 *update_id;
    UINT32 *compression_level;

    UINT32 update_area_port;
    QXLRect *update_area;
    UINT32 *update_surface;

    UINT32 *mm_clock;

    UINT32 log_port;
    UINT8 *log_buf;
    UINT32 *log_level;
#if (WINVER < 0x0501)
    PQXLWaitForEvent WaitForEvent;
#endif
    UINT8 num_mem_slot;
    UINT8 main_mem_slot_id;
    UINT8 slot_id_bits;
    UINT8 slot_gen_bits;
    UINT8 *slots_generation;
    UINT64 *ram_slot_start;
    UINT64 *ram_slot_end;
    MemSlot main_mem_slot;

    UINT32 destroy_surface_wait_port;
    UINT32 create_primary_port;
    UINT32 destroy_primary_port;
    UINT32 memslot_add_port;
    UINT32 memslot_del_port;
    UINT32 destroy_all_surfaces_port;

    UINT32 dev_id;

    QXLSurfaceCreate *primary_surface_create;

    UINT32 n_surfaces;

    UINT64 fb_phys;
} QXLDriverInfo;


#endif

