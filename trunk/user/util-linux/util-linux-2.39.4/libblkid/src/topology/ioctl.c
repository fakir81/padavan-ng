/*
 * ioctl based topology -- gathers topology information
 *
 * Copyright (C) 2009 Karel Zak <kzak@redhat.com>
 *
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "topology.h"

/*
 * ioctl topology values
 */
static const struct topology_val {

	long  ioc;
	size_t kernel_size;

	/* functions to set probing result */
	int (*set_ulong)(blkid_probe, unsigned long);
	int (*set_int)(blkid_probe, int);
	int (*set_u64)(blkid_probe, uint64_t);

} topology_vals[] = {
	{ BLKALIGNOFF, sizeof(int),
	  .set_int = blkid_topology_set_alignment_offset },
	{ BLKIOMIN, sizeof(int),
	  .set_ulong = blkid_topology_set_minimum_io_size },
	{ BLKIOOPT, sizeof(int),
	  .set_ulong = blkid_topology_set_optimal_io_size },
	{ BLKPBSZGET, sizeof(int),
	  .set_ulong = blkid_topology_set_physical_sector_size },
	{ BLKGETDISKSEQ, sizeof(uint64_t),
	  .set_u64 = blkid_topology_set_diskseq },
	/* we read BLKSSZGET in topology.c */
};

static int probe_ioctl_tp(blkid_probe pr,
		const struct blkid_idmag *mag __attribute__((__unused__)))
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(topology_vals); i++) {
		const struct topology_val *val = &topology_vals[i];
		int rc = 1;
		union {
			int s32;
			uint64_t u64;
		} data = { 0 };

		if (ioctl(pr->fd, val->ioc, &data) == -1)
			goto nothing;

		/* Convert from kernel to libblkid type */
		if (val->kernel_size == 4)
			data.u64 = data.s32;

		if (val->set_int)
			rc = val->set_int(pr, data.u64);
		else if (val->set_ulong)
			rc = val->set_ulong(pr, data.u64);
		else
			rc = val->set_u64(pr, data.u64);

		if (rc)
			goto err;
	}

	return 0;
nothing:
	return 1;
err:
	return -1;
}

const struct blkid_idinfo ioctl_tp_idinfo =
{
	.name		= "ioctl",
	.probefunc	= probe_ioctl_tp,
	.magics		= BLKID_NONE_MAGIC
};

