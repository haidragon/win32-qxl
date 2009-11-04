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

#ifndef WDM_HELPER_H
#define WDM_HELPER_H

#include "os_dep.h"

typedef ULONG (*PQXLWaitForEvent)(PVOID,PLARGE_INTEGER);

LONG QXLInitializeEvent(PVOID * pEvent);
void QXLSetEvent(PVOID pEvent);
void QXLDeleteEvent(PVOID pEvent);
ULONG QXLWaitForEvent(PVOID pEvent,PLARGE_INTEGER Timeout);

#define VideoPortDeleteEvent(dev,pEvent) QXLDeleteEvent(pEvent)
#define VideoPortCreateEvent(dev,flag,reserved,ppEvent) QXLInitializeEvent(ppEvent)
#define VideoPortSetEvent(dev,pEvent) QXLSetEvent(pEvent)

#endif
