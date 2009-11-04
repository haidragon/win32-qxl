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
#include "qxl.h"
#if (WINVER < 0x0501)
#include "wdmhelper.h"
#endif

VP_STATUS FindAdapter(PVOID dev_extension,
                      PVOID reserved,
                      PWSTR arg_str,
                      PVIDEO_PORT_CONFIG_INFO conf_info,
                      PUCHAR again);

BOOLEAN Initialize(PVOID dev_extension);

VP_STATUS GetPowerState(PVOID dev_extension,
                        ULONG hw_id,
                        PVIDEO_POWER_MANAGEMENT state);

VP_STATUS SetPowerState(PVOID dev_extension,
                        ULONG hw_wId,
                        PVIDEO_POWER_MANAGEMENT state);

VP_STATUS GetChildDescriptor(IN PVOID dev_extension,
                             IN PVIDEO_CHILD_ENUM_INFO  enum_info,
                             OUT PVIDEO_CHILD_TYPE  type,
                             OUT PUCHAR descriptor,
                             OUT PULONG uid,
                             OUT PULONG unused);

BOOLEAN StartIO(PVOID dev_extension, PVIDEO_REQUEST_PACKET packet);

BOOLEAN Interrupt(PVOID  HwDeviceExtension);

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, FindAdapter)
#pragma alloc_text(PAGE, Initialize)
#pragma alloc_text(PAGE, GetPowerState)
#pragma alloc_text(PAGE, SetPowerState)
#pragma alloc_text(PAGE, GetChildDescriptor)
#pragma alloc_text(PAGE, StartIO)
#endif

typedef struct QXLExtension {
    PVOID io_base;
    ULONG io_port;

    QXLRom *rom;
    ULONG rom_size;

    PHYSICAL_ADDRESS ram_physical;
    UINT8 *ram_start;
    QXLRam *ram_header;
    ULONG ram_size;

    PHYSICAL_ADDRESS vram_physical;
    ULONG vram_size;

    ULONG n_modes;
    PVIDEO_MODE_INFORMATION modes;

    PEVENT display_event;
    PEVENT cursor_event;
    PEVENT sleep_event;

} QXLExtension;

#define QXL_ALLOC_TAG '_lxq'


ULONG DriverEntry(PVOID context1, PVOID context2)
{
    VIDEO_HW_INITIALIZATION_DATA init_data;
    ULONG ret;

    PAGED_CODE();

    DEBUG_PRINT((0, "%s: enter\n", __FUNCTION__));

    VideoPortZeroMemory(&init_data, sizeof(VIDEO_HW_INITIALIZATION_DATA));
    init_data.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);
    init_data.HwDeviceExtensionSize = sizeof(QXLExtension);

    init_data.HwFindAdapter = FindAdapter;
    init_data.HwInitialize = Initialize;
    init_data.HwGetPowerState = GetPowerState;
    init_data.HwSetPowerState = SetPowerState;
    init_data.HwGetVideoChildDescriptor = GetChildDescriptor;
    init_data.HwStartIO = StartIO;
    init_data.HwInterrupt = Interrupt;

    ret = VideoPortInitialize(context1, context2, &init_data, NULL);

    if (ret != NO_ERROR) {
        DEBUG_PRINT((0, "%s: try W2K %u\n", __FUNCTION__, ret));
        init_data.HwInitDataSize = SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA;
        ret = VideoPortInitialize(context1, context2, &init_data, NULL);
    }
    DEBUG_PRINT((0, "%s: exit %u\n", __FUNCTION__, ret));
    return ret;
}


#if defined(ALLOC_PRAGMA)
VP_STATUS InitIO(QXLExtension *dev, PVIDEO_ACCESS_RANGE range);
#pragma alloc_text(PAGE, InitIO)
#endif

VP_STATUS InitIO(QXLExtension *dev, PVIDEO_ACCESS_RANGE range)
{
    PVOID io_base;

    PAGED_CODE();
    DEBUG_PRINT((0, "%s\n", __FUNCTION__));

    if (range->RangeLength < QXL_IO_RANGE_SIZE
        || !range->RangeInIoSpace) {
        DEBUG_PRINT((0, "%s: bad io range\n", __FUNCTION__));
        return ERROR_INVALID_DATA;
    }

    io_base = VideoPortGetDeviceBase(dev, range->RangeStart, range->RangeLength,
                                     range->RangeInIoSpace);

    if (!io_base) {
        DEBUG_PRINT((0, "%s: get io base failed\n", __FUNCTION__));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dev->io_base = io_base;
    dev->io_port = range->RangeStart.LowPart;

    DEBUG_PRINT((0, "%s: OK, io 0x%x size %lu\n", __FUNCTION__,
                 (ULONG)range->RangeStart.LowPart, range->RangeLength));

    return NO_ERROR;
}

#if defined(ALLOC_PRAGMA)
VP_STATUS InitRom(QXLExtension *dev, PVIDEO_ACCESS_RANGE range);
#pragma alloc_text(PAGE, InitRom)
#endif

VP_STATUS InitRom(QXLExtension *dev, PVIDEO_ACCESS_RANGE range)
{
    PVOID rom = NULL;
    ULONG rom_size = range->RangeLength;
    ULONG io_space = VIDEO_MEMORY_SPACE_MEMORY;
    VP_STATUS error;

    PAGED_CODE();
    DEBUG_PRINT((0, "%s\n", __FUNCTION__));

    if (rom_size < sizeof(QXLRom) || range->RangeInIoSpace) {
        DEBUG_PRINT((0, "%s: bad rom range\n", __FUNCTION__));
        return ERROR_INVALID_DATA;
    }
    if ((error = VideoPortMapMemory(dev, range->RangeStart,
                                    &rom_size, &io_space,
                                    &rom)) != NO_ERROR ) {
        DEBUG_PRINT((0, "%s: map rom filed\n", __FUNCTION__));
        return error;
    }

    if (rom_size < range->RangeLength) {
        DEBUG_PRINT((0, "%s: short rom map\n", __FUNCTION__));
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto err;
    }

    if (((QXLRom*)rom)->magic != QXL_ROM_MAGIC) {
        DEBUG_PRINT((0, "%s: bad rom magic\n", __FUNCTION__));
        error = ERROR_INVALID_DATA;
        goto err;
    }

    dev->rom = rom;
    dev->rom_size = range->RangeLength;
    DEBUG_PRINT((0, "%s OK: rom 0x%lx size %lu\n",
                 __FUNCTION__, (ULONG)range->RangeStart.QuadPart, range->RangeLength));
    return NO_ERROR;

    err:
    VideoPortUnmapMemory(dev, rom, NULL);
    DEBUG_PRINT((0, "%s ERR\n", __FUNCTION__));
    return error;
}

#if defined(ALLOC_PRAGMA)
VP_STATUS InitRam(QXLExtension *dev, PVIDEO_ACCESS_RANGE range);
#pragma alloc_text(PAGE, InitRam)
#endif

VP_STATUS InitRam(QXLExtension *dev, PVIDEO_ACCESS_RANGE range)
{
    UINT8 *ram = NULL;
    QXLRam *ram_header;
    ULONG ram_size = range->RangeLength;
    ULONG io_space = VIDEO_MEMORY_SPACE_MEMORY;
    VP_STATUS error;

    PAGED_CODE();
    DEBUG_PRINT((0, "%s\n", __FUNCTION__));

    if (ram_size < sizeof(QXLRam) + dev->rom->ram_header_offset || range->RangeInIoSpace) {
        DEBUG_PRINT((0, "%s: bad ram range\n", __FUNCTION__));
        return ERROR_INVALID_DATA;
    }

    if (ram_size < dev->rom->pages_offset + (dev->rom->num_io_pages << PAGE_SHIFT) ) {
        DEBUG_PRINT((0, "%s: bad ram size\n", __FUNCTION__));
        return ERROR_INVALID_DATA;
    }

    if ((error = VideoPortMapMemory(dev, range->RangeStart,
                                    &ram_size, &io_space,
                                    &ram)) != NO_ERROR ) {
        DEBUG_PRINT((0, "%s: map ram filed\n", __FUNCTION__));
        return error;
    }

    if (ram_size < range->RangeLength) {
        DEBUG_PRINT((0, "%s: short ram map\n", __FUNCTION__));
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto err;
    }
    ram_header = (QXLRam *)(ram + dev->rom->ram_header_offset);
    if (ram_header->magic != QXL_RAM_MAGIC) {
        DEBUG_PRINT((0, "%s: bad ram magic\n", __FUNCTION__));
        error = ERROR_INVALID_DATA;
        goto err;
    }

    dev->ram_physical = range->RangeStart;
    dev->ram_start = ram;
    dev->ram_header = ram_header;
    dev->ram_size = range->RangeLength;
    DEBUG_PRINT((0, "%s OK: ram 0x%lx size %lu\n",
                 __FUNCTION__, (ULONG)range->RangeStart.QuadPart, range->RangeLength));
    return NO_ERROR;

    err:
    VideoPortUnmapMemory(dev, ram, NULL);
    DEBUG_PRINT((0, "%s ERR\n", __FUNCTION__));
    return error;
}


#if defined(ALLOC_PRAGMA)
VP_STATUS InitVRAM(QXLExtension *dev, PVIDEO_ACCESS_RANGE range);
#pragma alloc_text(PAGE, InitVRAM)
#endif

VP_STATUS InitVRAM(QXLExtension *dev, PVIDEO_ACCESS_RANGE range)
{
    PAGED_CODE();
    DEBUG_PRINT((0, "%s\n", __FUNCTION__));

    if (range->RangeLength == 0 || range->RangeInIoSpace) {
        DEBUG_PRINT((0, "%s: bad mem range\n", __FUNCTION__));
        return ERROR_INVALID_DATA;
    }

    dev->vram_physical = range->RangeStart;
    dev->vram_size = range->RangeLength;
    DEBUG_PRINT((0, "%s: OK, vram 0x%lx size %lu\n",
                 __FUNCTION__, (ULONG)range->RangeStart.QuadPart, range->RangeLength));
    return NO_ERROR;
}

#if defined(ALLOC_PRAGMA)
VP_STATUS Prob(QXLExtension *dev, VIDEO_PORT_CONFIG_INFO *conf_info,
               PVIDEO_ACCESS_RANGE ranges, int n_ranges);
#pragma alloc_text(PAGE, Prob)
#endif

VP_STATUS Prob(QXLExtension *dev, VIDEO_PORT_CONFIG_INFO *conf_info,
               PVIDEO_ACCESS_RANGE ranges, int n_ranges)
{
    PCI_COMMON_CONFIG pci_conf;
    ULONG  bus_data_size;
    VP_STATUS error;

    PAGED_CODE();
    DEBUG_PRINT((0, "%s\n", __FUNCTION__));

    bus_data_size = VideoPortGetBusData(dev,
                                        PCIConfiguration,
                                        0,
                                        &pci_conf,
                                        0,
                                        sizeof(PCI_COMMON_CONFIG));

    if (bus_data_size != sizeof(PCI_COMMON_CONFIG)) {
        DEBUG_PRINT((0,  "%s: GetBusData size %d expectes %d\n",
                     __FUNCTION__, bus_data_size, sizeof(PCI_COMMON_CONFIG)));
        return ERROR_INVALID_PARAMETER;
    }

    if (pci_conf.VendorID != REDHAT_PCI_VENDOR_ID) {
        DEBUG_PRINT((0,  "%s: bad vendor id 0x%x expectes 0x%x\n",
                     __FUNCTION__, pci_conf.VendorID, REDHAT_PCI_VENDOR_ID));
        return ERROR_INVALID_PARAMETER;
    }

    if (pci_conf.DeviceID != QXL_DEVICE_ID) {
        DEBUG_PRINT((0,  "%s: bad vendor id 0x%x expectes 0x%x\n",
                     __FUNCTION__, pci_conf.DeviceID, QXL_DEVICE_ID));
        return ERROR_INVALID_PARAMETER;
    }

    if (pci_conf.RevisionID != QXL_REVISION) {
        DEBUG_PRINT((0,  "%s: bad revision 0x%x expectes 0x%x\n",
                     __FUNCTION__, pci_conf.RevisionID, QXL_REVISION));
        return ERROR_INVALID_PARAMETER;
    }

    VideoPortZeroMemory(ranges, sizeof(VIDEO_ACCESS_RANGE) * n_ranges);
    if ((error = VideoPortGetAccessRanges(dev, 0, NULL, n_ranges,
                                          ranges, NULL, NULL,
                                          NULL)) != NO_ERROR ) {
        DEBUG_PRINT((0, "%s: get access ranges failed status %u\n", __FUNCTION__, error));
    }

    if (conf_info->BusInterruptLevel == 0 && conf_info->BusInterruptVector == 0) {
        DEBUG_PRINT((0, "%s: no interrupt\n", __FUNCTION__));
        error = ERROR_INVALID_DATA;
    }

#ifdef DBG
    if (error == NO_ERROR) {
        int i;

        DEBUG_PRINT((0, "%s: interrupt: vector %lu level %lu mode %s\n",
                     __FUNCTION__,
                     conf_info->BusInterruptVector,
                     conf_info->BusInterruptLevel,
                     (conf_info->InterruptMode == LevelSensitive) ? "LevelSensitive" : "Latched"));

        for (i = 0; i < n_ranges; i++) {
            DEBUG_PRINT((0, "%s: range %d start 0x%lx length %lu space %lu\n", __FUNCTION__, i,
                         (ULONG)ranges[i].RangeStart.QuadPart,
                         ranges[i].RangeLength,
                         (ULONG)ranges[i].RangeInIoSpace));
        }
    }
#endif

    DEBUG_PRINT((0, "%s exit %lu\n", __FUNCTION__, error));
    return error;
}

#if defined(ALLOC_PRAGMA)
VP_STATUS SetVideoModeInfo(PVIDEO_MODE_INFORMATION video_mode, QXLMode *qxl_mode);
#pragma alloc_text(PAGE, SetVideoModeInfo)
#endif

VP_STATUS SetVideoModeInfo(PVIDEO_MODE_INFORMATION video_mode, QXLMode *qxl_mode)
{
    ULONG color_bits;
    PAGED_CODE();
    DEBUG_PRINT((0, "%s: x %u y %u bits %u stride %u orientation %u\n",
                 __FUNCTION__, qxl_mode->x_res, qxl_mode->y_res,
                 qxl_mode->bits, qxl_mode->stride, qxl_mode->orientation));

    video_mode->Length = sizeof(VIDEO_MODE_INFORMATION);
    video_mode->ModeIndex = qxl_mode->id;
    video_mode->VisScreenWidth = qxl_mode->x_res;
    video_mode->VisScreenHeight = qxl_mode->y_res;
    video_mode->ScreenStride = qxl_mode->stride;
    video_mode->NumberOfPlanes = 1;
    video_mode->BitsPerPlane = qxl_mode->bits;
    video_mode->Frequency = 100;
    video_mode->XMillimeter = qxl_mode->x_mili;
    video_mode->YMillimeter = qxl_mode->y_mili;
    color_bits = (qxl_mode->bits == 16) ? 5 : 8;
    video_mode->NumberRedBits = color_bits;
    video_mode->NumberGreenBits = color_bits;
    video_mode->NumberBlueBits = color_bits;

    video_mode->BlueMask = (1 << color_bits) - 1;
    video_mode->GreenMask = video_mode->BlueMask << color_bits;
    video_mode->RedMask = video_mode->GreenMask << color_bits;

    video_mode->AttributeFlags = VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS;
    video_mode->VideoMemoryBitmapWidth = qxl_mode->x_res;
    video_mode->VideoMemoryBitmapHeight = qxl_mode->y_res;
    video_mode->DriverSpecificAttributeFlags = qxl_mode->orientation;
    DEBUG_PRINT((0, "%s OK\n", __FUNCTION__));
    return NO_ERROR;
}


#if defined(ALLOC_PRAGMA)
VP_STATUS InitModes(QXLExtension *dev);
#pragma alloc_text(PAGE, InitModes)
#endif

VP_STATUS InitModes(QXLExtension *dev)
{
    QXLRom *rom;
    QXLModes *modes;
    PVIDEO_MODE_INFORMATION modes_info;
    ULONG n_modes;
    ULONG i;
    VP_STATUS error;

    PAGED_CODE();
    DEBUG_PRINT((0, "%s\n", __FUNCTION__));
    rom = dev->rom;
    modes = (QXLModes *)((UCHAR*)rom + rom->modes_offset);
    if (dev->rom_size < rom->modes_offset + sizeof(QXLModes) ||
        (n_modes = modes->n_modes) == 0 || dev->rom_size <
        rom->modes_offset + sizeof(QXLModes) + n_modes * sizeof(QXLMode)) {
        DEBUG_PRINT((0, "%s: bad rom size\n", __FUNCTION__));
        return ERROR_INVALID_DATA;
    }

#if (WINVER < 0x0501) //Win2K
    error = VideoPortAllocateBuffer(dev, n_modes * sizeof(VIDEO_MODE_INFORMATION), &modes_info);

    if(!modes_info || error != NO_ERROR) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
#else
    if (!(modes_info = VideoPortAllocatePool(dev, VpPagedPool,
                                             n_modes * sizeof(VIDEO_MODE_INFORMATION),
                                             QXL_ALLOC_TAG))) {
        DEBUG_PRINT((0, "%s: alloc mem failed\n", __FUNCTION__));
        return ERROR_NOT_ENOUGH_MEMORY;
    }
#endif
    VideoPortZeroMemory(modes_info, sizeof(VIDEO_MODE_INFORMATION) * n_modes);
    for (i = 0; i < n_modes; i++) {
        error = SetVideoModeInfo(&modes_info[i], &modes->modes[i]);
        if (error != NO_ERROR) {
            VideoPortFreePool(dev, modes_info);
            DEBUG_PRINT((0, "%s: set video mode failed\n", __FUNCTION__));
            return error;
        }
    }
    dev->n_modes = n_modes;
    dev->modes = modes_info;
    DEBUG_PRINT((0, "%s OK\n", __FUNCTION__));
    return NO_ERROR;
}

#if defined(ALLOC_PRAGMA)
void DevExternsionCleanup(QXLExtension *dev);
#pragma alloc_text(PAGE, DevExternsionCleanup)
#endif

void DevExternsionCleanup(QXLExtension *dev)
{
    if (dev->sleep_event) {
        VideoPortDeleteEvent(dev, dev->sleep_event);
    }

    if (dev->cursor_event) {
        VideoPortDeleteEvent(dev, dev->cursor_event);
    }

    if (dev->display_event) {
        VideoPortDeleteEvent(dev, dev->display_event);
    }

    if (dev->rom) {
        VideoPortUnmapMemory(dev, dev->rom, NULL);
    }

    if (dev->ram_start) {
        VideoPortUnmapMemory(dev, dev->ram_start, NULL);
    }

    if (dev->modes) {
        VideoPortFreePool(dev, dev->modes);
    }

    VideoPortZeroMemory(dev, sizeof(QXLExtension));
}

VP_STATUS FindAdapter(PVOID dev_extension,
                      PVOID reserved,
                      PWSTR arg_str,
                      PVIDEO_PORT_CONFIG_INFO conf_info,
                      PUCHAR again)
{
    QXLExtension *dev_ext = dev_extension;
    VP_STATUS status;
    VIDEO_ACCESS_RANGE ranges[QXL_PCI_RANGES];
    PEVENT display_event = NULL;
    PEVENT cursor_event = NULL;
    PEVENT sleep_event = NULL;
#if (WINVER >= 0x0501)
    VPOSVERSIONINFO  sys_info;
#endif

    PAGED_CODE();

    DEBUG_PRINT((0, "%s: enter\n", __FUNCTION__));

#if (WINVER >= 0x0501)
    VideoPortZeroMemory(&sys_info, sizeof(VPOSVERSIONINFO));
    sys_info.Size = sizeof(VPOSVERSIONINFO);
    if ((status = VideoPortGetVersion(dev_ext, &sys_info)) != NO_ERROR ||
        sys_info.MajorVersion < 5 || (sys_info.MajorVersion == 5 && sys_info.MinorVersion < 1) ) {
        DEBUG_PRINT((0, "%s: err not supported, status %lu major %lu minor %lu\n",
                     __FUNCTION__, status, sys_info.MajorVersion, sys_info.MinorVersion));
        return ERROR_NOT_SUPPORTED;
    }
#endif

    if (conf_info->Length < sizeof(VIDEO_PORT_CONFIG_INFO)) {
        DEBUG_PRINT((0, "%s: err configInfo size\n", __FUNCTION__));
        return ERROR_INVALID_PARAMETER;
    }

    if (conf_info->AdapterInterfaceType != PCIBus) {
        DEBUG_PRINT((0,  "%s: not a PCI device %d\n",
                     __FUNCTION__, conf_info->AdapterInterfaceType));
        return ERROR_DEV_NOT_EXIST;
    }

    if ((status = VideoPortCreateEvent(dev_ext, 0, NULL, &display_event)) != NO_ERROR) {
        DEBUG_PRINT((0,  "%s: create display event failed %lu\n",
                     __FUNCTION__, status));
        return status;
    }

    if ((status = VideoPortCreateEvent(dev_ext, 0, NULL, &cursor_event)) != NO_ERROR) {
        DEBUG_PRINT((0,  "%s: create cursor event failed %lu\n",
                     __FUNCTION__, status));
        VideoPortDeleteEvent(dev_ext, display_event);
        return status;
    }

    if ((status = VideoPortCreateEvent(dev_ext, 0, NULL, &sleep_event)) != NO_ERROR) {
        DEBUG_PRINT((0,  "%s: create sleep event failed %lu\n",
                     __FUNCTION__, status));
        VideoPortDeleteEvent(dev_ext, display_event);
        VideoPortDeleteEvent(dev_ext, cursor_event);
        return status;
    }

    dev_ext->display_event = display_event;
    dev_ext->cursor_event = cursor_event;
    dev_ext->sleep_event = sleep_event;

    if ((status = Prob(dev_ext, conf_info, ranges, QXL_PCI_RANGES)) != NO_ERROR ||
        (status = InitIO(dev_ext, &ranges[QXL_IO_RANGE_INDEX])) != NO_ERROR ||
        (status = InitRom(dev_ext, &ranges[QXL_ROM_RANGE_INDEX])) != NO_ERROR ||
        (status = InitRam(dev_ext, &ranges[QXL_RAM_RANGE_INDEX])) != NO_ERROR ||
        (status = InitVRAM(dev_ext, &ranges[QXL_VRAM_RANGE_INDEX])) != NO_ERROR ||
        (status = InitModes(dev_ext)) != NO_ERROR ) {
        DEBUG_PRINT((0,  "%s: findAdapter failed\n", __FUNCTION__));
        DevExternsionCleanup(dev_ext);
    }

    if (VideoPortSetRegistryParameters(dev_extension, L"QxlDeviceID",
                                       &dev_ext->rom->id, sizeof(UINT32)) != NO_ERROR) {
        DEBUG_PRINT((0, "%s: write QXL ID failed\n", __FUNCTION__));
    }

    DEBUG_PRINT((0, "%s: exit %lu\n", __FUNCTION__, status));
    return status;
}


#if defined(ALLOC_PRAGMA)
void HWReset(QXLExtension *dev_ext);
#pragma alloc_text(PAGE, HWReset)
#endif

void HWReset(QXLExtension *dev_ext)
{
    PAGED_CODE();
    DEBUG_PRINT((0, "%s\n", __FUNCTION__));
    VideoPortWritePortUchar((PUCHAR)dev_ext->io_base + QXL_IO_RESET, 0);
    DEBUG_PRINT((0, "%s: done\n", __FUNCTION__));
}

BOOLEAN Initialize(PVOID dev_ext)
{
    PAGED_CODE();
    DEBUG_PRINT((0, "%s: enter\n", __FUNCTION__));
    HWReset(dev_ext);
    DEBUG_PRINT((0, "%s: done\n", __FUNCTION__));
    return TRUE;
}

VP_STATUS GetPowerState(PVOID dev_extension,
                        ULONG hw_id,
                        PVIDEO_POWER_MANAGEMENT pm_stat)
{
    PAGED_CODE();
    DEBUG_PRINT((0, "%s: %lu\n", __FUNCTION__, pm_stat->PowerState));

    switch (hw_id) {
    case DISPLAY_ADAPTER_HW_ID:
        switch (pm_stat->PowerState) {
        case VideoPowerOn:
        case VideoPowerStandBy:
        case VideoPowerSuspend:
        case VideoPowerOff:
        case VideoPowerShutdown:
        case VideoPowerHibernate:
            DEBUG_PRINT((0, "%s: OK\n", __FUNCTION__));
            return NO_ERROR;
        }
        break;
    default:
        DEBUG_PRINT((0, "%s: unexpected hw_id %lu\n", __FUNCTION__, hw_id));
    }
    DEBUG_PRINT((0, "%s: ERROR_DEVICE_REINITIALIZATION_NEEDED\n", __FUNCTION__));
    return ERROR_DEVICE_REINITIALIZATION_NEEDED;
}

VP_STATUS SetPowerState(PVOID dev_extension,
                        ULONG hw_id,
                        PVIDEO_POWER_MANAGEMENT pm_stat)
{
    PAGED_CODE();
    DEBUG_PRINT((0, "%s: %lu\n", __FUNCTION__, pm_stat->PowerState));

    switch (hw_id) {
    case DISPLAY_ADAPTER_HW_ID:
        switch (pm_stat->PowerState) {
        case VideoPowerOn:
        case VideoPowerStandBy:
        case VideoPowerSuspend:
        case VideoPowerOff:
        case VideoPowerShutdown:
        case VideoPowerHibernate:
            DEBUG_PRINT((0, "%s: OK\n", __FUNCTION__));
            return NO_ERROR;
        }
        break;
    default:
        DEBUG_PRINT((0, "%s: unexpected hw_id %lu\n", __FUNCTION__, hw_id));
    }
    DEBUG_PRINT((0, "%s: ERROR_DEVICE_REINITIALIZATION_NEEDED\n", __FUNCTION__));
    return ERROR_DEVICE_REINITIALIZATION_NEEDED;
}

VP_STATUS GetChildDescriptor(IN PVOID dev_extension,
                             IN PVIDEO_CHILD_ENUM_INFO enum_info,
                             OUT PVIDEO_CHILD_TYPE type,
                             OUT PUCHAR descriptor,
                             OUT PULONG uid,
                             OUT PULONG unused)
{
    PAGED_CODE();
    DEBUG_PRINT((0, "%s: enter\n", __FUNCTION__));

    switch (enum_info->ChildIndex) {
    case 0:
        DEBUG_PRINT((0, "%s: ACPI id %u\n", __FUNCTION__, enum_info->ACPIHwId));
        return ERROR_NO_MORE_DEVICES;
    case 1:
        DEBUG_PRINT((0, "%s: Monitor\n", __FUNCTION__));
        /*
        *pChildType = Monitor;
        todo: handle EDID
        return ERROR_MORE_DATA;
        */
        return ERROR_NO_MORE_DEVICES;
    }
    DEBUG_PRINT((0, "%s: ERROR_NO_MORE_DEVICES\n", __FUNCTION__));
    return ERROR_NO_MORE_DEVICES;
}

#if defined(ALLOC_PRAGMA)
PVIDEO_MODE_INFORMATION FindMode(QXLExtension *dev_ext, ULONG mode);
#pragma alloc_text(PAGE, FindMode)
#endif

#define IsValidMode(dev, mode) (FindMode(dev, mode) != NULL)

PVIDEO_MODE_INFORMATION FindMode(QXLExtension *dev_ext, ULONG mode)
{
    VIDEO_MODE_INFORMATION *inf;
    VIDEO_MODE_INFORMATION *end;

    PAGED_CODE();
    DEBUG_PRINT((0, "%s\n", __FUNCTION__));

    inf = dev_ext->modes;
    end = inf + dev_ext->n_modes;
    for (; inf < end; inf++) {
        if (inf->ModeIndex == mode) {
            DEBUG_PRINT((0, "%s: OK mode %lu res %lu-%lu orientation %lu\n", __FUNCTION__,
                         mode, inf->VisScreenWidth, inf->VisScreenHeight,
                         inf->DriverSpecificAttributeFlags ));
            return inf;
        }
    }
    DEBUG_PRINT((0, "%s: mod info not found\n", __FUNCTION__));
    return NULL;
}

BOOLEAN StartIO(PVOID dev_extension, PVIDEO_REQUEST_PACKET packet)
{
    QXLExtension *dev_ext = dev_extension;
    VP_STATUS error;

    PAGED_CODE();
    DEBUG_PRINT((0, "%s\n", __FUNCTION__));

    switch (packet->IoControlCode) {
    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
        DEBUG_PRINT((0, "%s: IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES\n", __FUNCTION__));
        if (packet->OutputBufferLength < (packet->StatusBlock->Information =
                                          sizeof(VIDEO_NUM_MODES))) {
            error = ERROR_INSUFFICIENT_BUFFER;
            goto err;
        }
        ((PVIDEO_NUM_MODES)packet->OutputBuffer)->NumModes = dev_ext->n_modes;
        ((PVIDEO_NUM_MODES)packet->OutputBuffer)->ModeInformationLength =
        sizeof(VIDEO_MODE_INFORMATION);
        break;
    case IOCTL_VIDEO_QUERY_AVAIL_MODES: {
            VIDEO_MODE_INFORMATION *inf;
            VIDEO_MODE_INFORMATION *end;
            VIDEO_MODE_INFORMATION *out;

            DEBUG_PRINT((0, "%s: IOCTL_VIDEO_QUERY_AVAIL_MODES\n", __FUNCTION__));
            if (packet->OutputBufferLength < (packet->StatusBlock->Information =
                                              dev_ext->n_modes * sizeof(VIDEO_MODE_INFORMATION))) {
                error = ERROR_INSUFFICIENT_BUFFER;
                goto err;
            }
            out = packet->OutputBuffer;
            inf = dev_ext->modes;
            end = inf + dev_ext->n_modes;
            for ( ;inf < end; out++, inf++) {
                *out = *inf;
            }
        }
        break;
    case IOCTL_VIDEO_SET_CURRENT_MODE: {
            ULONG request_mode;
            DEBUG_PRINT((0, "%s: IOCTL_VIDEO_SET_CURRENT_MODE\n", __FUNCTION__));
            if (packet->InputBufferLength < sizeof(VIDEO_MODE)) {
                error = ERROR_INSUFFICIENT_BUFFER;
                goto err;
            }
            request_mode = ((PVIDEO_MODE)packet->InputBuffer)->RequestedMode;
            DEBUG_PRINT((0, "%s: mode %u\n", __FUNCTION__, request_mode));
            if (!IsValidMode(dev_ext, request_mode)) {
                error = ERROR_INVALID_PARAMETER;
                goto err;
            }
            VideoPortWritePortUchar((PUCHAR)dev_ext->io_base + QXL_IO_SET_MODE,
                                    (UCHAR)request_mode);
            dev_ext->ram_header->int_mask = ~0;
            VideoPortWritePortUchar((PUCHAR)dev_ext->io_base + QXL_IO_UPDATE_IRQ, 0);
        }
        break;
    case IOCTL_VIDEO_QUERY_CURRENT_MODE: {
            PVIDEO_MODE_INFORMATION inf;

            DEBUG_PRINT((0, "%s: IOCTL_VIDEO_QUERY_CURRENT_MODE\n", __FUNCTION__));

            if (packet->OutputBufferLength < (packet->StatusBlock->Information =
                                              sizeof(VIDEO_MODE_INFORMATION))) {
                error = ERROR_INSUFFICIENT_BUFFER;
                goto err;
            }

            if ((inf = FindMode(dev_ext, dev_ext->rom->mode)) == NULL) {
                DEBUG_PRINT((0, "%s: mod info not found\n", __FUNCTION__));
                error = ERROR_INVALID_DATA;
                goto err;
            }
            *(PVIDEO_MODE_INFORMATION)packet->OutputBuffer = *inf;
        }
        break;
    case IOCTL_VIDEO_MAP_VIDEO_MEMORY: {
            PVIDEO_MEMORY_INFORMATION mem_info;
            ULONG fb_io_space;

            DEBUG_PRINT((0, "%s: IOCTL_VIDEO_MAP_VIDEO_MEMORY\n", __FUNCTION__));

            if (packet->OutputBufferLength < (packet->StatusBlock->Information =
                                              sizeof(VIDEO_MEMORY_INFORMATION)) ||
                                            ( packet->InputBufferLength < sizeof(VIDEO_MEMORY) ) ) {
                error = ERROR_INSUFFICIENT_BUFFER;
                goto err;
            }
            mem_info = packet->OutputBuffer;

            mem_info->VideoRamBase =
                                    ((PVIDEO_MEMORY)(packet->InputBuffer))->RequestedVirtualAddress;
            ASSERT(mem_info->VideoRamBase == NULL);
            mem_info->VideoRamLength = dev_ext->vram_size;
            fb_io_space = VIDEO_MEMORY_SPACE_MEMORY;

            if ((error = VideoPortMapMemory(dev_ext, dev_ext->vram_physical,
                                            &mem_info->VideoRamLength,
                                            &fb_io_space, &mem_info->VideoRamBase)) != NO_ERROR) {
                DEBUG_PRINT((0, "%s: map filed\n", __FUNCTION__));
                goto err;
            }
            DEBUG_PRINT((0, "%s: vram size %lu ret size %lu fb vaddr 0x%lx\n",
                         __FUNCTION__,
                         dev_ext->vram_size,
                         mem_info->VideoRamLength,
                         mem_info->VideoRamBase));
            if (mem_info->VideoRamLength < dev_ext->vram_size) {
                DEBUG_PRINT((0, "%s: fb shrink\n", __FUNCTION__));
                VideoPortUnmapMemory(dev_ext, mem_info->VideoRamBase, NULL);
                mem_info->VideoRamBase = NULL;
                mem_info->VideoRamLength = 0;
                error = ERROR_NOT_ENOUGH_MEMORY;
                goto err;
            }
            mem_info->FrameBufferBase = mem_info->VideoRamBase;
            mem_info->FrameBufferLength = mem_info->VideoRamLength;
#ifdef DBG
            DEBUG_PRINT((0, "%s: zap\n", __FUNCTION__));
            VideoPortZeroMemory(mem_info->VideoRamBase, mem_info->VideoRamLength);
            DEBUG_PRINT((0, "%s: zap done\n", __FUNCTION__));
#endif
        }
        break;
    case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY: {
            PVOID addr;

            DEBUG_PRINT((0, "%s: IOCTL_VIDEO_UNMAP_VIDEO_MEMORY\n", __FUNCTION__));

            if (packet->InputBufferLength < sizeof(VIDEO_MEMORY)) {
                error = ERROR_INSUFFICIENT_BUFFER;
                goto err;
            }
            addr = ((PVIDEO_MEMORY)(packet->InputBuffer))->RequestedVirtualAddress;
            if ((error = VideoPortUnmapMemory(dev_ext, addr, NULL)) != NO_ERROR) {
                DEBUG_PRINT((0, "%s: unmap failed\n", __FUNCTION__));
            }
        }
        break;
    case IOCTL_VIDEO_RESET_DEVICE:
        DEBUG_PRINT((0, "%s: IOCTL_VIDEO_RESET_DEVICE\n", __FUNCTION__));
        HWReset(dev_ext);
        break;
    case IOCTL_QXL_GET_INFO: {
            QXLDriverInfo *driver_info;
            DEBUG_PRINT((0, "%s: IOCTL_QXL_GET_INFO\n", __FUNCTION__));

            if (packet->OutputBufferLength < (packet->StatusBlock->Information =
                                              sizeof(QXLDriverInfo))) {
                error = ERROR_INSUFFICIENT_BUFFER;
                goto err;
            }

            driver_info = packet->OutputBuffer;
            driver_info->version = QXL_DRIVER_INFO_VERSION;
            driver_info->display_event = dev_ext->display_event;
            driver_info->cursor_event = dev_ext->cursor_event;
            driver_info->sleep_event = dev_ext->sleep_event;
            driver_info->cmd_ring = &dev_ext->ram_header->cmd_ring;
            driver_info->cursor_ring = &dev_ext->ram_header->cursor_ring;
            driver_info->release_ring = &dev_ext->ram_header->release_ring;
            driver_info->notify_cmd_port = dev_ext->io_port + QXL_IO_NOTIFY_CMD;
            driver_info->notify_cursor_port = dev_ext->io_port + QXL_IO_NOTIFY_CURSOR;
            driver_info->notify_oom_port = dev_ext->io_port + QXL_IO_NOTIFY_OOM;
            driver_info->log_port = dev_ext->io_port + QXL_IO_LOG;
            driver_info->log_buf = dev_ext->ram_header->log_buf;

            driver_info->draw_area = dev_ext->ram_start + dev_ext->rom->draw_area_offset;
            driver_info->draw_area_size = dev_ext->rom->draw_area_size;
            driver_info->update_id = &dev_ext->rom->update_id;
            driver_info->mm_clock = &dev_ext->rom->mm_clock;
            driver_info->compression_level = &dev_ext->rom->compression_level;
            driver_info->log_level = &dev_ext->rom->log_level;
            driver_info->update_area_port = dev_ext->io_port + QXL_IO_UPDATE_AREA;
            driver_info->update_area = &dev_ext->ram_header->update_area;

            driver_info->num_io_pages = dev_ext->rom->num_io_pages;
            driver_info->io_pages_virt = dev_ext->ram_start + dev_ext->rom->pages_offset;
            driver_info->io_pages_phys = dev_ext->ram_physical.QuadPart +
                                                                dev_ext->rom->pages_offset;
#if (WINVER < 0x0501)
            driver_info->WaitForEvent = QXLWaitForEvent;
#endif
        }
        break;
    default:
        DEBUG_PRINT((0, "%s: invalid command 0x%lx\n", __FUNCTION__, packet->IoControlCode));
        error = ERROR_INVALID_FUNCTION;
        goto err;
    }
    packet->StatusBlock->Status = NO_ERROR;
    DEBUG_PRINT((0, "%s: OK\n", __FUNCTION__));
    return TRUE;
    err:
    packet->StatusBlock->Information = 0;
    packet->StatusBlock->Status = error;
    DEBUG_PRINT((0, "%s: ERR\n", __FUNCTION__));
    return TRUE;
}

VOID InterruptCallback(PVOID dev_extension, PVOID Context)
{
    QXLExtension *dev_ext = dev_extension;
    UINT32 pending = VideoPortInterlockedExchange(&dev_ext->ram_header->int_pending, 0);

    if (pending & QXL_INTERRUPT_DISPLAY) {
        VideoPortSetEvent(dev_ext, dev_ext->display_event);
    } if (pending & QXL_INTERRUPT_CURSOR) {
        VideoPortSetEvent(dev_ext, dev_ext->cursor_event);
    }

    dev_ext->ram_header->int_mask = ~0;
    VideoPortWritePortUchar((PUCHAR)dev_ext->io_base + QXL_IO_UPDATE_IRQ, 0);
}

BOOLEAN Interrupt(PVOID dev_extension)
{
    QXLExtension *dev_ext = dev_extension;

    if (!(dev_ext->ram_header->int_pending & dev_ext->ram_header->int_mask)) {
        return FALSE;
    }
    dev_ext->ram_header->int_mask = 0;
    VideoPortWritePortUchar((PUCHAR)dev_ext->io_base + QXL_IO_UPDATE_IRQ, 0);

    if (!VideoPortQueueDpc(dev_extension, InterruptCallback, NULL)) {
        VideoPortLogError(dev_extension, NULL, E_UNEXPECTED, QXLERR_INT_DELIVERY);
        dev_ext->ram_header->int_mask = ~0;
        VideoPortWritePortUchar((PUCHAR)dev_ext->io_base + QXL_IO_UPDATE_IRQ, 0);
    }
    return TRUE;
}
