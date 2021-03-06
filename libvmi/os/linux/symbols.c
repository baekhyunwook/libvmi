/* The LibVMI Library is an introspection library that simplifies access to
 * memory in a target virtual machine or in a file containing a dump of
 * a system's physical memory.  LibVMI is based on the XenAccess Library.
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government
 * retains certain rights in this software.
 *
 * Author: Bryan D. Payne (bdpayne@acm.org)
 *
 * This file is part of LibVMI.
 *
 * LibVMI is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * LibVMI is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with LibVMI.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "private.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "os/linux/linux.h"

#define MAX_ROW_LENGTH 500

static int
get_symbol_row(
    FILE * f,
    char *row,
    const char *symbol,
    int position)
{
    int ret = VMI_FAILURE;

    while (fgets(row, MAX_ROW_LENGTH, f) != NULL) {
        char *token = NULL;

        /* find the correct token to check */
        int curpos = 0;
        int position_copy = position;

        while (position_copy > 0 && curpos < MAX_ROW_LENGTH) {
            if (isspace(row[curpos])) {
                while (isspace(row[curpos])) {
                    row[curpos] = '\0';
                    ++curpos;
                }
                --position_copy;
                continue;
            }
            ++curpos;
        }
        if (position_copy == 0) {
            token = row + curpos;
            while (curpos < MAX_ROW_LENGTH) {
                if (isspace(row[curpos])) {
                    row[curpos] = '\0';
                    break;
                }
                ++curpos;
            }
        }
        else {  /* some went wrong in the loop above */
            goto error_exit;
        }

        /* check the token */
        if (strncmp(token, symbol, MAX_ROW_LENGTH) == 0) {
            ret = VMI_SUCCESS;
            break;
        }
    }

error_exit:
    if (ret == VMI_FAILURE) {
        memset(row, 0, MAX_ROW_LENGTH);
    }
    return ret;
}

status_t
linux_system_map_symbol_to_address(
    vmi_instance_t vmi,
    const char *symbol,
    addr_t *kernel_base_vaddr,
    addr_t *address)
{
    FILE *f = NULL;
    char *row = NULL;
    status_t ret;

    linux_instance_t linux_instance = vmi->os_data;

    if (linux_instance == NULL) {
        errprint("VMI_ERROR: OS instance not initialized\n");
        return 0;
    }

    if ((NULL == linux_instance->sysmap) || (strlen(linux_instance->sysmap) == 0)) {
        errprint("VMI_WARNING: No linux sysmap configured\n");
        return 0;
    }

    row = safe_malloc(MAX_ROW_LENGTH);
    if ((f = fopen(linux_instance->sysmap, "r")) == NULL) {
        fprintf(stderr,
                "ERROR: could not find System.map file after checking:\n");
        fprintf(stderr, "\t%s\n", linux_instance->sysmap);
        fprintf(stderr,
                "To fix this problem, add the correct sysmap entry to /etc/libvmi.conf\n");
        address = 0;
        goto error_exit;
    }
    if (get_symbol_row(f, row, symbol, 2) == VMI_FAILURE) {
        address = 0;
        goto error_exit;
    }

    if (kernel_base_vaddr) {
        (*kernel_base_vaddr) = 0;
    }
    (*address) = (addr_t) strtoull(row, NULL, 16);

    return VMI_SUCCESS;
error_exit:
    if (row)
        free(row);
    if (f)
        fclose(f);
    return VMI_FAILURE;
}
