/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */






#ifndef _UMP_COMMON_H_
#define _UMP_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#define UMP_UINT32_MAX (4294967295U)
#define UMP_UINT64_MAX (18446744073709551615ULL)

#ifdef __GNUC__
	#define CHECK_RESULT        __attribute__((__warn_unused_result__))
	#define INLINE	__inline__
#else
	#define CHECK_RESULT
	#define INLINE __inline
#endif

#ifndef STATIC
	#define STATIC static
#endif

#define UMP_VERSION_MAJOR 2
#define UMP_VERSION_MINOR 0

typedef int32_t ump_secure_id;

#define UMP_INVALID_SECURE_ID  ((ump_secure_id)0)

typedef enum
{
	UMP_OK    = 0, 
	UMP_ERROR = 1  
} ump_result;

/**
 * Allocation flag bits.
 *
 * ump_allocate accepts zero or more flags to specify the type of memory to allocate and how to expose it to devices.
 *
 * For each supported device there are 4 flags to control access permissions and give usage characteristic hints to optimize the allocation/mapping.
 * They are;
 * @li @a UMP_PROT_<device>_RD   read permission
 * @li @a UMP_PROT_<device>_WR   write permission
 * @li @a UMP_HINT_<device>_RD   read often
 * @li @a UMP_HINT_<device>_WR   written often
 *
 * 5 devices are currently supported, with a device being the CPU itself.
 * The other 4 devices will be mapped to real devices per SoC design.
 * They are just named W,X,Y,Z by UMP as it has no knowledge of their real names/identifiers.
 * As an example device W could be a camera device while device Z could be an ARM GPU device, leaving X and Y unused.
 *
 * 2 additional flags control the allocation;
 * @li @a UMP_CONSTRAINT_PHYSICALLY_LINEAR   the allocation must be physical linear. Typical for devices without an MMU and no IOMMU to help it.
 * @li @a UMP_PROT_SHAREABLE                 the allocation can be shared with other processes on the system. Without this flag the returned allocation won't be resolvable in other processes.
 *
 * All UMP allocation are growable unless they're @a UMP_PROT_SHAREABLE.
 * The hint bits should be used to indicate the access pattern so the driver can select the most optimal memory type and cache settings based on the what the system supports.
 */
typedef enum
{
	
	UMP_PROT_DEVICE_RD = (1u << 0),
	UMP_PROT_DEVICE_WR = (1u << 1),
	UMP_HINT_DEVICE_RD = (1u << 2),
	UMP_HINT_DEVICE_WR = (1u << 3),
	UMP_DEVICE_MASK = 0xF,
	UMP_DEVICE_CPU_SHIFT = 0,
	UMP_DEVICE_W_SHIFT = 4,
	UMP_DEVICE_X_SHIFT = 8,
	UMP_DEVICE_Y_SHIFT = 12,
	UMP_DEVICE_Z_SHIFT = 16,

	
	UMP_PROT_CPU_RD = (1u <<  0),
	UMP_PROT_CPU_WR = (1u <<  1),
	UMP_HINT_CPU_RD = (1u <<  2),
	UMP_HINT_CPU_WR = (1u <<  3),

	
	UMP_PROT_W_RD = (1u <<  4),
	UMP_PROT_W_WR = (1u <<  5),
	UMP_HINT_W_RD = (1u <<  6),
	UMP_HINT_W_WR = (1u <<  7),

	
	UMP_PROT_X_RD = (1u <<  8),
	UMP_PROT_X_WR = (1u <<  9),
	UMP_HINT_X_RD = (1u << 10),
	UMP_HINT_X_WR = (1u << 11),
	
	
	UMP_PROT_Y_RD = (1u << 12),
	UMP_PROT_Y_WR = (1u << 13),
	UMP_HINT_Y_RD = (1u << 14),
	UMP_HINT_Y_WR = (1u << 15),

	
	UMP_PROT_Z_RD = (1u << 16),
	UMP_PROT_Z_WR = (1u << 17),
	UMP_HINT_Z_RD = (1u << 18),
	UMP_HINT_Z_WR = (1u << 19),

	
	UMPP_ALLOCBITS_UNUSED = (0x7Fu << 20),
	
	UMP_CONSTRAINT_UNCACHED = (1u << 27),
	
	UMP_CONSTRAINT_32BIT_ADDRESSABLE = (1u << 28),
	
	UMP_CONSTRAINT_PHYSICALLY_LINEAR = (1u << 29),
	
	UMP_PROT_SHAREABLE = (1u << 30)
	
} ump_allocation_bits;

typedef uint32_t ump_alloc_flags;


#define UMP_V1_API_DEFAULT_ALLOCATION_FLAGS		UMP_PROT_CPU_RD | UMP_PROT_CPU_WR | \
												UMP_PROT_W_RD | UMP_PROT_W_WR |	\
												UMP_PROT_X_RD | UMP_PROT_X_WR |	\
												UMP_PROT_Y_RD | UMP_PROT_Y_WR |	\
												UMP_PROT_Z_RD | UMP_PROT_Z_WR |	\
												UMP_PROT_SHAREABLE |	\
												UMP_CONSTRAINT_32BIT_ADDRESSABLE

enum
{
	UMP_MSYNC_CLEAN = 1,

	UMP_MSYNC_CLEAN_AND_INVALIDATE

};

typedef uint32_t ump_cpu_msync_op;

enum ump_external_memory_type
{
	UMPP_EXTERNAL_MEM_TYPE_UNUSED = 0, 
	UMP_EXTERNAL_MEM_TYPE_ION = 1,
	UMPP_EXTERNAL_MEM_COUNT
};


typedef enum
{
	
	UMP_REF_DRV_CONSTRAINT_NONE = 0,
	
	UMP_REF_DRV_CONSTRAINT_PHYSICALLY_LINEAR = 1,
	
	UMP_REF_DRV_CONSTRAINT_USE_CACHE = 4
} ump_alloc_constraints;



#ifdef __cplusplus
}
#endif


#endif 
