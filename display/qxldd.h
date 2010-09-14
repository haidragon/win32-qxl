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

#ifndef _H_QXLDD
#define _H_QXLDD


#include "stddef.h"

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "wingdi.h"
#include "winddi.h"
#include "qxl_driver.h"
#include "mspace.h"
#if (WINVER < 0x0501)
#include "wdmhelper.h"
#endif

#define ALLOC_TAG 'dlxq'

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define DEBUG_PRINT(arg) DebugPrint arg

#ifdef DBG
#define ASSERT(pdev, x) if (!(x)) { \
    DebugPrint(pdev, 0, "ASSERT(%s) failed @ %s\n", #x, __FUNCTION__); \
    EngDebugBreak();\
}
#define ONDBG(x) x
#else
#define ASSERT(pdev, x)
#define ONDBG(x)
#endif

#define PANIC(pdev, str) {                                      \
    DebugPrint(pdev, 0, "PANIC: %s @ %s\n", str, __FUNCTION__); \
    EngDebugBreak();                                            \
}

typedef enum {
    QXL_SUCCESS,
    QXL_FAILED,
    QXL_UNSUPPORTED,
} QXLRESULT;

typedef struct Ring RingItem;
typedef struct Ring {
    RingItem *prev;
    RingItem *next;
} Ring;

#define IMAGE_HASH_SHIFT 15
#define IMAGE_HASH_SIZE (1 << IMAGE_HASH_SHIFT)
#define IMAGE_HASH_MASK (IMAGE_HASH_SIZE - 1)

#define IMAGE_POOL_SIZE (1 << 15)

#define CURSOR_CACHE_SIZE (1 << 6)
#define CURSOR_HASH_SIZE (CURSOR_CACHE_SIZE << 1)
#define CURSOR_HASH_NASKE (CURSOR_HASH_SIZE - 1)

#define PALETTE_CACHE_SIZE (1 << 6)
#define PALETTE_HASH_SIZE (PALETTE_CACHE_SIZE << 1)
#define PALETTE_HASH_NASKE (PALETTE_HASH_SIZE - 1)

//#define CALL_TEST

#ifdef CALL_TEST
enum {
    CALL_COUNTER_COPY_BITS,
    CALL_COUNTER_BIT_BLT,
    CALL_COUNTER_TEXT_OUT,
    CALL_COUNTER_STROKE_PATH,
    CALL_COUNTER_STRETCH_BLT,
    CALL_COUNTER_STRETCH_BLT_ROP,
    CALL_COUNTER_TRANSPARENT_BLT,
    CALL_COUNTER_ALPHA_BLEND,

    CALL_COUNTER_FILL_PATH,
    CALL_COUNTER_GRADIENT_FILL,
    CALL_COUNTER_LINE_TO,
    CALL_COUNTER_PLG_BLT,
    CALL_COUNTER_STROKE_AND_FILL_PATH,

    NUM_CALL_COUNTERS,
};
#endif

typedef struct QuicData QuicData;

#define IMAGE_KEY_HASH_SIZE (1 << 15)
#define IMAGE_KEY_HASH_MASK (IMAGE_KEY_HASH_SIZE - 1)

typedef struct ImageKey {
    HSURF hsurf;
    UINT64 unique;
    UINT32 key;
} ImageKey;

typedef struct CacheImage {
    struct CacheImage *next;
    RingItem lru_link;
    UINT32 key;
    UINT32 hits;
    UINT8 format;
    UINT32 width;
    UINT32 height;
    struct InternalImage *image;
} CacheImage;

#define NUM_UPDATE_TRACE_ITEMS 10
typedef struct UpdateTrace {
    RingItem link;
    UINT32 last_time;
    RECTL area;
    HSURF hsurf;
    UINT8 count;
} UpdateTrace;

typedef struct PMemSlot {
    MemSlot slot;
    QXLPHYSICAL high_bits;
} PMemSlot;

typedef struct DevResDynamic {
    CacheImage cache_image_pool[IMAGE_POOL_SIZE];
    Ring cache_image_lru;
    Ring cursors_lru;
    Ring palette_lru;
} DevResDynamic;

typedef struct MspaceInfo {
    mspace _mspace;
    UINT8 *mspace_start;
    UINT8 *mspace_end;
} MspaceInfo;

enum {
    MSPACE_TYPE_DEVRAM,
    MSPACE_TYPE_VRAM,

    NUM_MSPACES,
};

typedef struct PDev PDev;

typedef struct DrawArea {
   HSURF bitmap;
   SURFOBJ* surf_obj;
   UINT8 *base_mem;
} DrawArea;

typedef struct SurfaceInfo SurfaceInfo;
struct SurfaceInfo {
    DrawArea draw_area;
    union {
        PDev *pdev;
        SurfaceInfo *next_free;
    } u;
};

typedef struct DevRes {   
    MspaceInfo mspaces[NUM_MSPACES];
    HSEMAPHORE malloc_sem; /* Also protects release ring */

    BOOL need_init;
    UINT64 free_outputs;
    UINT32 update_id;

    DevResDynamic *dynamic;

    ImageKey image_key_lookup[IMAGE_KEY_HASH_SIZE];
    struct CacheImage *image_cache[IMAGE_HASH_SIZE];
    struct InternalCursor *cursor_cache[CURSOR_HASH_SIZE];
    UINT32 num_cursors;
    UINT32 last_cursor_id;
    struct InternalPalette *palette_cache[PALETTE_HASH_SIZE];
    UINT32 num_palettes;

    SurfaceInfo *surfaces_info;
    SurfaceInfo *free_surfaces;

    HANDLE driver;
#ifdef DBG
    int num_free_pages;
    int num_outputs;
    int num_path_pages;
    int num_rects_pages;
    int num_bits_pages;
    int num_buf_pages;
    int num_glyphs_pages;
    int num_cursor_pages;
#endif

#ifdef CALL_TEST
    BOOL count_calls;
    UINT32 total_calls;
    UINT32 call_counters[NUM_CALL_COUNTERS];
#endif
} DevRes;

#define SSE_MASK 15
#define SSE_ALIGN 16

typedef struct PDev {
    HANDLE driver;
    HDEV eng;
    HPALETTE palette;
    HSURF surf;
    UINT8 surf_enable;
    DWORD video_mode_index;
    SIZEL resolution;
    UINT32 max_bitmap_size;
    ULONG bitmap_format;

    ULONG fb_size;
    BYTE* fb;
    UINT64 fb_phys;
    UINT8 vram_slot_initialized;
    UINT8 vram_mem_slot;

    ULONG stride;
    FLONG red_mask;
    FLONG green_mask;
    FLONG blue_mask;
    ULONG fp_state_size;

    QXLPHYSICAL surf_phys;
    UINT8 *surf_base;

    QuicData *quic_data;

    QXLCommandRing *cmd_ring;
    QXLCursorRing *cursor_ring;
    QXLReleaseRing *release_ring;
    UINT32 notify_cmd_port;
    UINT32 notify_cursor_port;
    UINT32 notify_oom_port;
    PEVENT display_event;
    PEVENT cursor_event;
    PEVENT sleep_event;

    UINT32 log_port;
    UINT8 *log_buf;
    UINT32 *log_level;

    HSEMAPHORE print_sem;
    HSEMAPHORE cmd_sem;

    PMemSlot *mem_slots;
    UINT8 num_mem_slot;
    UINT8 main_mem_slot;
    UINT8 slot_id_bits;
    UINT8 slot_gen_bits;
    UINT8 *slots_generation;
    UINT64 *ram_slot_start;
    UINT64 *ram_slot_end;
    QXLPHYSICAL va_slot_mask;

    UINT32 num_io_pages;
    UINT8 *io_pages_virt;
    UINT64 io_pages_phys;

    UINT32 *dev_update_id;

    UINT32 update_area_port;
    QXLRect *update_area;
    UINT32 *update_surface;

    UINT32 *mm_clock;

    UINT32 *compression_level;

    FLONG cursor_trail;

#if (WINVER < 0x0501)
    PQXLWaitForEvent WaitForEvent;
#endif

    UINT32 create_primary_port;
    UINT32 destroy_primary_port;
    UINT32 destroy_surface_wait_port;
    UINT32 memslot_add_port;
    UINT32 memslot_del_port;
    UINT32 destroy_all_surfaces_port;

    UINT8* primary_memory_start;
    UINT32 primary_memory_size;

    QXLSurfaceCreate *primary_surface_create;

    UINT32 dev_id;

    DevRes Res;

    Ring update_trace;
    UpdateTrace update_trace_items[NUM_UPDATE_TRACE_ITEMS];

    UINT8 FPUSave[16 * 4 + 15];

    UINT32 n_surfaces;
    SurfaceInfo surface0_info;

    VIDEOMEMORY *pvmList;
} PDev;


void DebugPrintV(PDev *pdev, const char *message, va_list ap);
void DebugPrint(PDev *pdev, int level, const char *message, ...);

void InitGlobalRes();
void CleanGlobalRes();
void InitResources(PDev *pdev);
void SyncResources(PDev *pdev);

#ifdef CALL_TEST
void CountCall(PDev *pdev, int counter);
#else
#define CountCall(a, b)
#endif

char *BitmapFormatToStr(int format);
char *BitmapTypeToStr(int type);

static _inline void RingInit(Ring *ring)
{
    ring->next = ring->prev = ring;
}

static _inline void RingItemInit(RingItem *item)
{
    item->next = item->prev = NULL;
}

static _inline BOOL RingItemIsLinked(RingItem *item)
{
    return !!item->next;
}

static _inline BOOL RingIsEmpty(PDev *pdev, Ring *ring)
{
    ASSERT(pdev, ring->next != NULL && ring->prev != NULL);
    return ring == ring->next;
}

static _inline void RingAdd(PDev *pdev, Ring *ring, RingItem *item)
{
    ASSERT(pdev, ring->next != NULL && ring->prev != NULL);
    ASSERT(pdev, item->next == NULL && item->prev == NULL);

    item->next = ring->next;
    item->prev = ring;
    ring->next = item->next->prev = item;
}

static _inline void RingRemove(PDev *pdev, RingItem *item)
{
    ASSERT(pdev, item->next != NULL && item->prev != NULL);
    ASSERT(pdev, item->next != item);

    item->next->prev = item->prev;
    item->prev->next = item->next;
    item->prev = item->next = 0;
}

static _inline RingItem *RingGetTail(PDev *pdev, Ring *ring)
{
    RingItem *ret;

    ASSERT(pdev, ring->next != NULL && ring->prev != NULL);

    if (RingIsEmpty(pdev, ring)) {
        return NULL;
    }
    ret = ring->prev;
    return ret;
}

#endif
