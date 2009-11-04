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

#if (WINVER < 0x0501)
#include <ntddk.h>
#include "wdmhelper.h"

void QXLDeleteEvent(PVOID pEvent)
{
    if(pEvent) {
        ExFreePool(pEvent);
    }
}

LONG QXLInitializeEvent(PVOID * pEvent)
{
    if(pEvent) {
        *pEvent = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), '_lxq');

        if(*pEvent) {
            KeInitializeEvent((PRKEVENT)*pEvent, SynchronizationEvent, FALSE);
        }
    }

    return 0;
}

void QXLSetEvent(PVOID pEvent)
{
    //Pririty boost can be switched to IO_NO_INCREMENT
    if(pEvent) {
        KeSetEvent((PRKEVENT)pEvent, IO_VIDEO_INCREMENT, FALSE);
    }
}

ULONG QXLWaitForEvent(PVOID pEvent, PLARGE_INTEGER Timeout)
{
    if(pEvent) {
        return NT_SUCCESS(KeWaitForSingleObject(pEvent, Executive, KernelMode, TRUE, Timeout));
    }

    return FALSE;
}

#endif

