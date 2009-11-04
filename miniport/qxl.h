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

#include "winerror.h"
#include "devioctl.h"
#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#include "qxl_driver.h"

enum {
    QXLERR_INT_DELIVERY = 1,
};

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)

#if DBG
#define DEBUG_PRINT(arg) VideoDebugPrint(arg)
#else
#define DEBUG_PRINT(arg)
#endif

