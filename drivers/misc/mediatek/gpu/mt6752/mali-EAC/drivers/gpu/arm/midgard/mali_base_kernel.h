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






#ifndef _BASE_KERNEL_H_
#define _BASE_KERNEL_H_

#ifndef __user
#define __user
#endif

#define BASE_LEGACY_UK6_SUPPORT 1

#define BASE_LEGACY_UK7_SUPPORT 1

typedef mali_addr64 base_mem_handle;

#include "mali_base_mem_priv.h"
#include "mali_kbase_profiling_gator_api.h"

#define BASE_JD_ATOM_COUNT              256

#define BASEP_JD_SEM_PER_WORD_LOG2      5
#define BASEP_JD_SEM_PER_WORD           (1 << BASEP_JD_SEM_PER_WORD_LOG2)
#define BASEP_JD_SEM_WORD_NR(x)         ((x) >> BASEP_JD_SEM_PER_WORD_LOG2)
#define BASEP_JD_SEM_MASK_IN_WORD(x)    (1 << ((x) & (BASEP_JD_SEM_PER_WORD - 1)))
#define BASEP_JD_SEM_ARRAY_SIZE         BASEP_JD_SEM_WORD_NR(BASE_JD_ATOM_COUNT)

#define BASE_GPU_NUM_TEXTURE_FEATURES_REGISTERS 3

#define BASE_MAX_COHERENT_GROUPS 16

#if defined CDBG_ASSERT
#define LOCAL_ASSERT CDBG_ASSERT
#elif defined KBASE_DEBUG_ASSERT
#define LOCAL_ASSERT KBASE_DEBUG_ASSERT
#else
#error assert macro not defined!
#endif

#if defined PAGE_MASK
#define LOCAL_PAGE_LSB ~PAGE_MASK
#else
#include <osu/mali_osu.h>

#if defined OSU_CONFIG_CPU_PAGE_SIZE_LOG2
#define LOCAL_PAGE_LSB ((1ul << OSU_CONFIG_CPU_PAGE_SIZE_LOG2) - 1)
#else
#error Failed to find page size
#endif
#endif

typedef union kbase_pointer {
	void __user *value;	  
	u32 compat_value; 
	u64 sizer;	  
} kbase_pointer;



typedef u32 base_mem_alloc_flags;

enum {
	BASE_MEM_PROT_CPU_RD = (1U << 0),      
	BASE_MEM_PROT_CPU_WR = (1U << 1),      
	BASE_MEM_PROT_GPU_RD = (1U << 2),      
	BASE_MEM_PROT_GPU_WR = (1U << 3),      
	BASE_MEM_PROT_GPU_EX = (1U << 4),      

	BASE_MEM_HINT_CPU_RD = (1U << 5),      
	BASE_MEM_HINT_CPU_WR = (1U << 6),      /**< Heavily written CPU side
						    - OBSOLETE */
	BASE_MEM_HINT_GPU_RD = (1U << 7),      
	BASE_MEM_HINT_GPU_WR = (1U << 8),      /**< Heavily written GPU side
						    - OBSOLETE */

	BASE_MEM_GROW_ON_GPF = (1U << 9),      

	BASE_MEM_COHERENT_SYSTEM = (1U << 10), 
	BASE_MEM_COHERENT_LOCAL = (1U << 11),  
	BASE_MEM_CACHED_CPU = (1U << 12),      

	BASE_MEM_SAME_VA = (1U << 13), 
	BASE_MEM_NEED_MMAP = (1U << 14) 
};

#define BASE_MEM_FLAGS_NR_INPUT_BITS  14
#define BASE_MEM_FLAGS_NR_OUTPUT_BITS 1
#define BASE_MEM_FLAGS_NR_TOTAL_BITS  ((BASE_MEM_FLAGS_NR_INPUT_BITS) + (BASE_MEM_FLAGS_NR_OUTPUT_BITS))
#define BASE_MEM_FLAGS_NR_BITS 15

#if BASE_MEM_FLAGS_NR_TOTAL_BITS > BASE_MEM_FLAGS_NR_BITS
#error "Too many flag bits, will require change in cmem"
#endif

typedef enum base_mem_import_type {
	BASE_MEM_IMPORT_TYPE_INVALID = 0,
	
	BASE_MEM_IMPORT_TYPE_UMP = 1,
	
	BASE_MEM_IMPORT_TYPE_UMM = 2
} base_mem_import_type;

#define base_tmem_import_type          base_mem_import_type
#define BASE_TMEM_IMPORT_TYPE_INVALID  BASE_MEM_IMPORT_TYPE_INVALID
#define BASE_TMEM_IMPORT_TYPE_UMP      BASE_MEM_IMPORT_TYPE_UMP
#define BASE_TMEM_IMPORT_TYPE_UMM      BASE_MEM_IMPORT_TYPE_UMM

#define BASE_MEM_INVALID_HANDLE                (0ull  << 12)
#define BASE_MEM_MMU_DUMP_HANDLE               (1ull  << 12)
#define BASE_MEM_TRACE_BUFFER_HANDLE           (2ull  << 12)
#define BASE_MEM_MAP_TRACKING_HANDLE           (3ull  << 12)
#define BASE_MEM_WRITE_ALLOC_PAGES_HANDLE      (4ull  << 12)
#define BASE_MEM_COOKIE_BASE                   (64ul  << 12)
#define BASE_MEM_FIRST_FREE_ADDRESS            ((BITS_PER_LONG << 12) + \
						BASE_MEM_COOKIE_BASE)

#define KBASE_COOKIE_MASK  ~1UL 


typedef enum base_backing_threshold_status {
	BASE_BACKING_THRESHOLD_OK = 0,			    
	BASE_BACKING_THRESHOLD_ERROR_NOT_GROWABLE = -1,	    
	BASE_BACKING_THRESHOLD_ERROR_OOM = -2,		    
	BASE_BACKING_THRESHOLD_ERROR_MAPPED = -3,	    
	BASE_BACKING_THRESHOLD_ERROR_INVALID_ARGUMENTS = -4 
} base_backing_threshold_status;


typedef struct base_syncset {
	struct basep_syncset basep_sset;
} base_syncset;


typedef struct base_import_handle {
	struct {
		mali_addr64 handle;
	} basep;
} base_import_handle;



typedef int platform_fence_type;
#define INVALID_PLATFORM_FENCE ((platform_fence_type)-1)

typedef struct base_stream {
	struct {
		int fd;
	} basep;
} base_stream;

typedef struct base_fence {
	struct {
		int fd;
		int stream_fd;
	} basep;
} base_fence;

typedef struct base_jd_udata {
	u64 blob[2];	 
} base_jd_udata;

struct base_mem_aliasing_info {
	base_mem_handle handle;
	u64 offset;
	u64 length;
};

typedef u8 base_jd_dep_type;


#define BASE_JD_DEP_TYPE_INVALID  (0) 	
#define BASE_JD_DEP_TYPE_DATA     (1U << 0) 	
#define BASE_JD_DEP_TYPE_ORDER    (1U << 1) 	

typedef u16 base_jd_core_req;

#define BASE_JD_REQ_DEP 0	    
#define BASE_JD_REQ_FS  (1U << 0)   
#define BASE_JD_REQ_CS  (1U << 1)
#define BASE_JD_REQ_T   (1U << 2)   
#define BASE_JD_REQ_CF  (1U << 3)   
#define BASE_JD_REQ_V   (1U << 4)   


#define BASE_JD_REQ_FS_AFBC  (1U << 13)

#define BASE_JD_REQ_COHERENT_GROUP  (1U << 6)


#define BASE_JD_REQ_PERMON               (1U << 7)

#define BASE_JD_REQ_EXTERNAL_RESOURCES   (1U << 8)

#define BASE_JD_REQ_SOFT_JOB        (1U << 9)

#define BASE_JD_REQ_SOFT_DUMP_CPU_GPU_TIME      (BASE_JD_REQ_SOFT_JOB | 0x1)
#define BASE_JD_REQ_SOFT_FENCE_TRIGGER          (BASE_JD_REQ_SOFT_JOB | 0x2)
#define BASE_JD_REQ_SOFT_FENCE_WAIT             (BASE_JD_REQ_SOFT_JOB | 0x3)

#define BASE_JD_REQ_SOFT_REPLAY                 (BASE_JD_REQ_SOFT_JOB | 0x4)

#define BASE_JD_REQ_ONLY_COMPUTE    (1U << 10)

#define BASE_JD_REQ_SPECIFIC_COHERENT_GROUP (1U << 11)

#define BASE_JD_REQ_EVENT_ONLY_ON_FAILURE   (1U << 12)

#define BASEP_JD_REQ_EVENT_NEVER (1U << 14)


#define BASEP_JD_REQ_RESERVED_BIT5 (1U << 5)
#define BASEP_JD_REQ_RESERVED_BIT15 (1U << 15)


#define BASEP_JD_REQ_RESERVED (BASEP_JD_REQ_RESERVED_BIT5 | \
				BASEP_JD_REQ_RESERVED_BIT15)

#define BASEP_JD_REQ_ATOM_TYPE (~(BASEP_JD_REQ_RESERVED | BASE_JD_REQ_EVENT_ONLY_ON_FAILURE |\
				BASE_JD_REQ_EXTERNAL_RESOURCES | BASEP_JD_REQ_EVENT_NEVER))

enum kbase_atom_coreref_state {
	
	KBASE_ATOM_COREREF_STATE_NO_CORES_REQUESTED,
	
	KBASE_ATOM_COREREF_STATE_WAITING_FOR_REQUESTED_CORES,
	
	KBASE_ATOM_COREREF_STATE_RECHECK_AFFINITY,
	
	KBASE_ATOM_COREREF_STATE_CHECK_AFFINITY_VIOLATIONS,
	
	KBASE_ATOM_COREREF_STATE_READY
};

enum kbase_jd_atom_state {
	
	KBASE_JD_ATOM_STATE_UNUSED,
	
	KBASE_JD_ATOM_STATE_QUEUED,
	
	KBASE_JD_ATOM_STATE_IN_JS,
	
	KBASE_JD_ATOM_STATE_COMPLETED
};

typedef u8 base_atom_id; 

struct base_dependency {
	base_atom_id  atom_id;               
	base_jd_dep_type dependency_type;    
}; 

typedef struct base_jd_atom_v2 {
	mali_addr64 jc;			    
	struct base_jd_udata udata;		    
	kbase_pointer extres_list;	    
	u16 nr_extres;			    
	base_jd_core_req core_req;	    
	const struct base_dependency pre_dep[2]; 
	base_atom_id atom_number;	    
	s8 prio;			    
	u8 device_nr;			    
	u8 padding[5];
} base_jd_atom_v2;

#ifdef BASE_LEGACY_UK6_SUPPORT
struct base_jd_atom_v2_uk6 {
	mali_addr64 jc;			    
	struct base_jd_udata udata;		    
	kbase_pointer extres_list;	    
	u16 nr_extres;			    
	base_jd_core_req core_req;	    
	base_atom_id pre_dep[2]; 
	base_atom_id atom_number;	    
	s8 prio;			    
	u8 device_nr;			    
	u8 padding[7];
};
#endif

typedef enum base_external_resource_access {
	BASE_EXT_RES_ACCESS_SHARED,
	BASE_EXT_RES_ACCESS_EXCLUSIVE
} base_external_resource_access;

typedef struct base_external_resource {
	u64 ext_resource;
} base_external_resource;

static INLINE void base_jd_atom_dep_set(const struct base_dependency* const_dep, base_atom_id id, base_jd_dep_type dep_type)
{
	struct base_dependency* dep;
	
	LOCAL_ASSERT(const_dep != NULL);
	
	LOCAL_ASSERT( ( id == 0 && dep_type == BASE_JD_DEP_TYPE_INVALID) || 
				(id > 0 && dep_type != BASE_JD_DEP_TYPE_INVALID) );

	dep = REINTERPRET_CAST(struct base_dependency*)const_dep;

	dep->atom_id = id;
	dep->dependency_type = dep_type;
}

/**
 * @brief Make a copy of a dependency structure
 *
 * @param[in,out] dep          The kbase jd atom dependency to be written.
 * @param[in]     from         The dependency to make a copy from.
 *
 */
static INLINE void base_jd_atom_dep_copy(const struct base_dependency* const_dep, const struct base_dependency* from)
{
	LOCAL_ASSERT(const_dep != NULL);

	base_jd_atom_dep_set(const_dep, from->atom_id, from->dependency_type);
}

static INLINE void base_jd_fence_trigger_setup_v2(struct base_jd_atom_v2 *atom, struct base_fence *fence)
{
	LOCAL_ASSERT(atom);
	LOCAL_ASSERT(fence);
	LOCAL_ASSERT(fence->basep.fd == INVALID_PLATFORM_FENCE);
	LOCAL_ASSERT(fence->basep.stream_fd >= 0);
	atom->jc = (uintptr_t) fence;
	atom->core_req = BASE_JD_REQ_SOFT_FENCE_TRIGGER;
}

static INLINE void base_jd_fence_wait_setup_v2(struct base_jd_atom_v2 *atom, struct base_fence *fence)
{
	LOCAL_ASSERT(atom);
	LOCAL_ASSERT(fence);
	LOCAL_ASSERT(fence->basep.fd >= 0);
	atom->jc = (uintptr_t) fence;
	atom->core_req = BASE_JD_REQ_SOFT_FENCE_WAIT;
}

static INLINE void base_external_resource_init(struct base_external_resource * res, struct base_import_handle handle, base_external_resource_access access)
{
	mali_addr64 address;
	address = handle.basep.handle;

	LOCAL_ASSERT(res != NULL);
	LOCAL_ASSERT(0 == (address & LOCAL_PAGE_LSB));
	LOCAL_ASSERT(access == BASE_EXT_RES_ACCESS_SHARED || access == BASE_EXT_RES_ACCESS_EXCLUSIVE);

	res->ext_resource = address | (access & LOCAL_PAGE_LSB);
}

enum {
	BASE_JD_SW_EVENT_KERNEL = (1u << 15), 
	BASE_JD_SW_EVENT = (1u << 14), 
	BASE_JD_SW_EVENT_SUCCESS = (1u << 13), 
	BASE_JD_SW_EVENT_JOB = (0u << 11), 
	BASE_JD_SW_EVENT_BAG = (1u << 11), 
	BASE_JD_SW_EVENT_INFO = (2u << 11), 
	BASE_JD_SW_EVENT_RESERVED = (3u << 11),	
	BASE_JD_SW_EVENT_TYPE_MASK = (3u << 11)	    
};

typedef enum base_jd_event_code {
	

	BASE_JD_EVENT_RANGE_HW_NONFAULT_START = 0,

	
	BASE_JD_EVENT_NOT_STARTED = 0x00, 
	BASE_JD_EVENT_DONE = 0x01,
	BASE_JD_EVENT_STOPPED = 0x03,	  
	BASE_JD_EVENT_TERMINATED = 0x04,  
	BASE_JD_EVENT_ACTIVE = 0x08,	  

	BASE_JD_EVENT_RANGE_HW_NONFAULT_END = 0x40,

	
	BASE_JD_EVENT_RANGE_HW_FAULT_OR_SW_ERROR_START = 0x40,

	
	BASE_JD_EVENT_JOB_CONFIG_FAULT = 0x40,
	BASE_JD_EVENT_JOB_POWER_FAULT = 0x41,
	BASE_JD_EVENT_JOB_READ_FAULT = 0x42,
	BASE_JD_EVENT_JOB_WRITE_FAULT = 0x43,
	BASE_JD_EVENT_JOB_AFFINITY_FAULT = 0x44,
	BASE_JD_EVENT_JOB_BUS_FAULT = 0x48,
	BASE_JD_EVENT_INSTR_INVALID_PC = 0x50,
	BASE_JD_EVENT_INSTR_INVALID_ENC = 0x51,
	BASE_JD_EVENT_INSTR_TYPE_MISMATCH = 0x52,
	BASE_JD_EVENT_INSTR_OPERAND_FAULT = 0x53,
	BASE_JD_EVENT_INSTR_TLS_FAULT = 0x54,
	BASE_JD_EVENT_INSTR_BARRIER_FAULT = 0x55,
	BASE_JD_EVENT_INSTR_ALIGN_FAULT = 0x56,
	BASE_JD_EVENT_DATA_INVALID_FAULT = 0x58,
	BASE_JD_EVENT_TILE_RANGE_FAULT = 0x59,
	BASE_JD_EVENT_STATE_FAULT = 0x5A,
	BASE_JD_EVENT_OUT_OF_MEMORY = 0x60,
	BASE_JD_EVENT_UNKNOWN = 0x7F,

	
	BASE_JD_EVENT_DELAYED_BUS_FAULT = 0x80,
	BASE_JD_EVENT_SHAREABILITY_FAULT = 0x88,

	
	BASE_JD_EVENT_TRANSLATION_FAULT_LEVEL1 = 0xC1,
	BASE_JD_EVENT_TRANSLATION_FAULT_LEVEL2 = 0xC2,
	BASE_JD_EVENT_TRANSLATION_FAULT_LEVEL3 = 0xC3,
	BASE_JD_EVENT_TRANSLATION_FAULT_LEVEL4 = 0xC4,
	BASE_JD_EVENT_PERMISSION_FAULT = 0xC8,
	BASE_JD_EVENT_TRANSTAB_BUS_FAULT_LEVEL1 = 0xD1,
	BASE_JD_EVENT_TRANSTAB_BUS_FAULT_LEVEL2 = 0xD2,
	BASE_JD_EVENT_TRANSTAB_BUS_FAULT_LEVEL3 = 0xD3,
	BASE_JD_EVENT_TRANSTAB_BUS_FAULT_LEVEL4 = 0xD4,
	BASE_JD_EVENT_ACCESS_FLAG = 0xD8,

	
	BASE_JD_EVENT_MEM_GROWTH_FAILED	= BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_JOB | 0x000,
	BASE_JD_EVENT_TIMED_OUT		= BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_JOB | 0x001,
	BASE_JD_EVENT_JOB_CANCELLED	= BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_JOB | 0x002,
	BASE_JD_EVENT_JOB_INVALID	= BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_JOB | 0x003,
	BASE_JD_EVENT_PM_EVENT		= BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_JOB | 0x004,
	BASE_JD_EVENT_FORCE_REPLAY	= BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_JOB | 0x005,

	BASE_JD_EVENT_BAG_INVALID	= BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_BAG | 0x003,

	
	BASE_JD_EVENT_RANGE_HW_FAULT_OR_SW_ERROR_END = BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_RESERVED | 0x3FF,

	
	BASE_JD_EVENT_RANGE_SW_SUCCESS_START = BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_SUCCESS | 0x000,

	BASE_JD_EVENT_PROGRESS_REPORT = BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_SUCCESS | BASE_JD_SW_EVENT_JOB | 0x000,
	BASE_JD_EVENT_BAG_DONE = BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_SUCCESS | BASE_JD_SW_EVENT_BAG | 0x000,
	BASE_JD_EVENT_DRV_TERMINATED = BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_SUCCESS | BASE_JD_SW_EVENT_INFO | 0x000,

	
	BASE_JD_EVENT_RANGE_SW_SUCCESS_END = BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_SUCCESS | BASE_JD_SW_EVENT_RESERVED | 0x3FF,

	
	BASE_JD_EVENT_RANGE_KERNEL_ONLY_START = BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_KERNEL | 0x000,
	BASE_JD_EVENT_REMOVED_FROM_NEXT = BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_KERNEL | BASE_JD_SW_EVENT_JOB | 0x000,

	
	BASE_JD_EVENT_RANGE_KERNEL_ONLY_END = BASE_JD_SW_EVENT | BASE_JD_SW_EVENT_KERNEL | BASE_JD_SW_EVENT_RESERVED | 0x3FF
} base_jd_event_code;

typedef struct base_jd_event_v2 {
	base_jd_event_code event_code;  
	base_atom_id atom_number;       
	struct base_jd_udata udata;     
} base_jd_event_v2;


#define BASE_CPU_GPU_CACHE_LINE_PADDING (36)



typedef struct base_dump_cpu_gpu_counters {
	u64 system_time;
	u64 cycle_counter;
	u64 sec;
	u32 usec;
	u8 padding[BASE_CPU_GPU_CACHE_LINE_PADDING];
} base_dump_cpu_gpu_counters;




#ifdef __KERNEL__
typedef u32 midg_product_id;
typedef u32 midg_cache_features;
typedef u32 midg_tiler_features;
typedef u32 midg_mem_features;
typedef u32 midg_mmu_features;
typedef u32 midg_js_features;
typedef u32 midg_as_present;
typedef u32 midg_js_present;

#define MIDG_MAX_JOB_SLOTS 16

#else
#include <midg/mali_midg.h>
#endif

/**
 * @page page_base_user_api_gpuprops User-side Base GPU Property Query API
 *
 * The User-side Base GPU Property Query API encapsulates two
 * sub-modules:
 *
 * - @ref base_user_api_gpuprops_dyn "Dynamic GPU Properties"
 * - @ref base_plat_config_gpuprops "Base Platform Config GPU Properties"
 *
 * There is a related third module outside of Base, which is owned by the MIDG
 * module:
 * - @ref midg_gpuprops_static "Midgard Compile-time GPU Properties"
 *
 * Base only deals with properties that vary between different Midgard
 * implementations - the Dynamic GPU properties and the Platform Config
 * properties.
 *
 * For properties that are constant for the Midgard Architecture, refer to the
 * MIDG module. However, we will discuss their relevance here <b>just to
 * provide background information.</b>
 *
 * @section sec_base_user_api_gpuprops_about About the GPU Properties in Base and MIDG modules
 *
 * The compile-time properties (Platform Config, Midgard Compile-time
 * properties) are exposed as pre-processor macros.
 *
 * Complementing the compile-time properties are the Dynamic GPU
 * Properties, which act as a conduit for the Midgard Configuration
 * Discovery.
 *
 * In general, the dynamic properties are present to verify that the platform
 * has been configured correctly with the right set of Platform Config
 * Compile-time Properties.
 *
 * As a consistant guide across the entire DDK, the choice for dynamic or
 * compile-time should consider the following, in order:
 * -# Can the code be written so that it doesn't need to know the
 * implementation limits at all?
 * -# If you need the limits, get the information from the Dynamic Property
 * lookup. This should be done once as you fetch the context, and then cached
 * as part of the context data structure, so it's cheap to access.
 * -# If there's a clear and arguable inefficiency in using Dynamic Properties,
 * then use a Compile-Time Property (Platform Config, or Midgard Compile-time
 * property). Examples of where this might be sensible follow:
 *  - Part of a critical inner-loop
 *  - Frequent re-use throughout the driver, causing significant extra load
 * instructions or control flow that would be worthwhile optimizing out.
 *
 * We cannot provide an exhaustive set of examples, neither can we provide a
 * rule for every possible situation. Use common sense, and think about: what
 * the rest of the driver will be doing; how the compiler might represent the
 * value if it is a compile-time constant; whether an OEM shipping multiple
 * devices would benefit much more from a single DDK binary, instead of
 * insignificant micro-optimizations.
 *
 * @section sec_base_user_api_gpuprops_dyn Dynamic GPU Properties
 *
 * Dynamic GPU properties are presented in two sets:
 * -# the commonly used properties in @ref base_gpu_props, which have been
 * unpacked from GPU register bitfields.
 * -# The full set of raw, unprocessed properties in @ref midg_raw_gpu_props
 * (also a member of @ref base_gpu_props). All of these are presented in
 * the packed form, as presented by the GPU  registers themselves.
 *
 * @usecase The raw properties in @ref midg_raw_gpu_props are necessary to
 * allow a user of the Mali Tools (e.g. PAT) to determine "Why is this device
 * behaving differently?". In this case, all information about the
 * configuration is potentially useful, but it <b>does not need to be processed
 * by the driver</b>. Instead, the raw registers can be processed by the Mali
 * Tools software on the host PC.
 *
 * The properties returned extend the Midgard Configuration Discovery
 * registers. For example, GPU clock speed is not specified in the Midgard
 * Architecture, but is <b>necessary for OpenCL's clGetDeviceInfo() function</b>.
 *
 * The GPU properties are obtained by a call to
 * _mali_base_get_gpu_props(). This simply returns a pointer to a const
 * base_gpu_props structure. It is constant for the life of a base
 * context. Multiple calls to _mali_base_get_gpu_props() to a base context
 * return the same pointer to a constant structure. This avoids cache pollution
 * of the common data.
 *
 * This pointer must not be freed, because it does not point to the start of a
 * region allocated by the memory allocator; instead, just close the @ref
 * base_context.
 *
 *
 * @section sec_base_user_api_gpuprops_config Platform Config Compile-time Properties
 *
 * The Platform Config File sets up gpu properties that are specific to a
 * certain platform. Properties that are 'Implementation Defined' in the
 * Midgard Architecture spec are placed here.
 *
 * @note Reference configurations are provided for Midgard Implementations, such as
 * the Mali-T600 family. The customer need not repeat this information, and can select one of
 * these reference configurations. For example, VA_BITS, PA_BITS and the
 * maximum number of samples per pixel might vary between Midgard Implementations, but
 * \b not for platforms using the Mali-T604. This information is placed in
 * the reference configuration files.
 *
 * The System Integrator creates the following structure:
 * - platform_XYZ
 * - platform_XYZ/plat
 * - platform_XYZ/plat/plat_config.h
 *
 * They then edit plat_config.h, using the example plat_config.h files as a
 * guide.
 *
 * At the very least, the customer must set @ref CONFIG_GPU_CORE_TYPE, and will
 * receive a helpful \#error message if they do not do this correctly. This
 * selects the Reference Configuration for the Midgard Implementation. The rationale
 * behind this decision (against asking the customer to write \#include
 * <gpus/mali_t600.h> in their plat_config.h) is as follows:
 * - This mechanism 'looks' like a regular config file (such as Linux's
 * .config)
 * - It is difficult to get wrong in a way that will produce strange build
 * errors:
 *  - They need not know where the mali_t600.h, other_midg_gpu.h etc. files are stored - and
 *  so they won't accidentally pick another file with 'mali_t600' in its name
 *  - When the build doesn't work, the System Integrator may think the DDK is
 *  doesn't work, and attempt to fix it themselves:
 *   - For the @ref CONFIG_GPU_CORE_TYPE mechanism, the only way to get past the
 *   error is to set @ref CONFIG_GPU_CORE_TYPE, and this is what the \#error tells
 *   you.
 *   - For a \#include mechanism, checks must still be made elsewhere, which the
 *   System Integrator may try working around by setting \#defines (such as
 *   VA_BITS) themselves in their plat_config.h. In the  worst case, they may
 *   set the prevention-mechanism \#define of
 *   "A_CORRECT_MIDGARD_CORE_WAS_CHOSEN".
 *   - In this case, they would believe they are on the right track, because
 *   the build progresses with their fix, but with errors elsewhere.
 *
 * However, there is nothing to prevent the customer using \#include to organize
 * their own configurations files hierarchically.
 *
 * The mechanism for the header file processing is as follows:
 *
 * @dot
   digraph plat_config_mechanism {
	   rankdir=BT
	   size="6,6"

       "mali_base.h";
	   "midg/midg.h";

	   node [ shape=box ];
	   {
	       rank = same; ordering = out;

		   "midg/midg_gpu_props.h";
		   "base/midg_gpus/mali_t600.h";
		   "base/midg_gpus/other_midg_gpu.h";
	   }
	   { rank = same; "plat/plat_config.h"; }
	   {
	       rank = same;
		   "midg/midg.h" [ shape=box ];
		   gpu_chooser [ label="" style="invisible" width=0 height=0 fixedsize=true ];
		   select_gpu [ label="Mali-T600 | Other\n(select_gpu.h)" shape=polygon,sides=4,distortion=0.25 width=3.3 height=0.99 fixedsize=true ] ;
	   }
	   node [ shape=box ];
	   { rank = same; "plat/plat_config.h"; }
	   { rank = same; "mali_base.h"; }

	   "mali_base.h" -> "midg/midg.h" -> "midg/midg_gpu_props.h";
	   "mali_base.h" -> "plat/plat_config.h" ;
	   "mali_base.h" -> select_gpu ;

	   "plat/plat_config.h" -> gpu_chooser [style="dotted,bold" dir=none weight=4] ;
	   gpu_chooser -> select_gpu [style="dotted,bold"] ;

	   select_gpu -> "base/midg_gpus/mali_t600.h" ;
	   select_gpu -> "base/midg_gpus/other_midg_gpu.h" ;
   }
   @enddot
 *
 *
 * @section sec_base_user_api_gpuprops_kernel Kernel Operation
 *
 * During Base Context Create time, user-side makes a single kernel call:
 * - A call to fill user memory with GPU information structures
 *
 * The kernel-side will fill the provided the entire processed @ref base_gpu_props
 * structure, because this information is required in both
 * user and kernel side; it does not make sense to decode it twice.
 *
 * Coherency groups must be derived from the bitmasks, but this can be done
 * kernel side, and just once at kernel startup: Coherency groups must already
 * be known kernel-side, to support chains that specify a 'Only Coherent Group'
 * SW requirement, or 'Only Coherent Group with Tiler' SW requirement.
 *
 * @section sec_base_user_api_gpuprops_cocalc Coherency Group calculation
 * Creation of the coherent group data is done at device-driver startup, and so
 * is one-time. This will most likely involve a loop with CLZ, shifting, and
 * bit clearing on the L2_PRESENT or L3_PRESENT masks, depending on whether the
 * system is L2 or L2+L3 Coherent. The number of shader cores is done by a
 * population count, since faulty cores may be disabled during production,
 * producing a non-contiguous mask.
 *
 * The memory requirements for this algoirthm can be determined either by a u64
 * population count on the L2/L3_PRESENT masks (a LUT helper already is
 * requried for the above), or simple assumption that there can be no more than
 * 16 coherent groups, since core groups are typically 4 cores.
 */



#define BASE_GPU_NUM_TEXTURE_FEATURES_REGISTERS 3

#define BASE_MAX_COHERENT_GROUPS 16

struct mali_base_gpu_core_props {
	midg_product_id product_id;

	u16 version_status;

	u16 minor_revision;

	u16 major_revision;

	u16 padding;

	u32 gpu_speed_mhz;

	u32 gpu_freq_khz_max;
	u32 gpu_freq_khz_min;

	u32 log2_program_counter_size;

	u32 texture_features[BASE_GPU_NUM_TEXTURE_FEATURES_REGISTERS];

	u64 gpu_available_memory_size;
};

struct mali_base_gpu_l2_cache_props {
	u8 log2_line_size;
	u8 log2_cache_size;
	u8 num_l2_slices; 
	u8 padding[5];
};

struct mali_base_gpu_l3_cache_props {
	u8 log2_line_size;
	u8 log2_cache_size;
	u8 padding[6];
};

struct mali_base_gpu_tiler_props {
	u32 bin_size_bytes;	
	u32 max_active_levels;	
};

struct mali_base_gpu_thread_props {
	u32 max_threads;             
	u32 max_workgroup_size;     
	u32 max_barrier_size;       
	u16 max_registers;			
	u8  max_task_queue;			
	u8  max_thread_group_split;	
	u8  impl_tech;		    	
	u8  padding[7];
};

struct mali_base_gpu_coherent_group {
	u64 core_mask;	       
	u16 num_cores;	       
	u16 padding[3];
};

struct mali_base_gpu_coherent_group_info {
	u32 num_groups;

	u32 num_core_groups;

	midg_mem_features coherency;

	u32 padding;

	struct mali_base_gpu_coherent_group group[BASE_MAX_COHERENT_GROUPS];
};

struct midg_raw_gpu_props {
	u64 shader_present;
	u64 tiler_present;
	u64 l2_present;
	u64 l3_present;

	midg_cache_features l2_features;
	midg_cache_features l3_features;
	midg_mem_features mem_features;
	midg_mmu_features mmu_features;

	midg_as_present as_present;

	u32 js_present;
	midg_js_features js_features[MIDG_MAX_JOB_SLOTS];
	midg_tiler_features tiler_features;
	u32 texture_features[3];

	u32 gpu_id;
	
	u32 thread_max_threads;
	u32 thread_max_workgroup_size;
	u32 thread_max_barrier_size;
	u32 thread_features;

	u32 padding;
};

typedef struct mali_base_gpu_props {
	struct mali_base_gpu_core_props core_props;
	struct mali_base_gpu_l2_cache_props l2_props;
	struct mali_base_gpu_l3_cache_props l3_props;
	struct mali_base_gpu_tiler_props tiler_props;
	struct mali_base_gpu_thread_props thread_props;

	
	struct midg_raw_gpu_props raw_props;

	
	struct mali_base_gpu_coherent_group_info coherency_info;
} base_gpu_props;




enum base_context_create_flags {
	
	BASE_CONTEXT_CREATE_FLAG_NONE = 0,

	
	BASE_CONTEXT_CCTX_EMBEDDED = (1u << 0),

	BASE_CONTEXT_SYSTEM_MONITOR_SUBMIT_DISABLED = (1u << 1),

	BASE_CONTEXT_HINT_ONLY_COMPUTE = (1u << 2)
};

#define BASE_CONTEXT_CREATE_ALLOWED_FLAGS \
	(((u32)BASE_CONTEXT_CCTX_EMBEDDED) | \
	  ((u32)BASE_CONTEXT_SYSTEM_MONITOR_SUBMIT_DISABLED) | \
	  ((u32)BASE_CONTEXT_HINT_ONLY_COMPUTE))

#define BASE_CONTEXT_CREATE_KERNEL_FLAGS \
	(((u32)BASE_CONTEXT_SYSTEM_MONITOR_SUBMIT_DISABLED) | \
	  ((u32)BASE_CONTEXT_HINT_ONLY_COMPUTE))

enum basep_context_private_flags {
	
	BASEP_CONTEXT_FLAG_JOB_DUMP_DISABLED = (1 << 31)
};






typedef struct base_jd_replay_payload {
	mali_addr64 tiler_jc_list;

	mali_addr64 fragment_jc;

	mali_addr64 tiler_heap_free;

	u16 fragment_hierarchy_mask;

	u16 tiler_hierarchy_mask;

	u32 hierarchy_default_weight;

	base_jd_core_req tiler_core_req;

	base_jd_core_req fragment_core_req;

	u8 padding[4];
} base_jd_replay_payload;

typedef struct base_jd_replay_jc {
	mali_addr64 next;

	mali_addr64 jc;

} base_jd_replay_jc;

#define BASE_JD_REPLAY_F_CHAIN_JOB_LIMIT 256


typedef struct base_profiling_controls {
	u32 profiling_controls[FBDUMP_CONTROL_MAX];
} base_profiling_controls;

#endif				
