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





#ifndef _KBASE_H_
#define _KBASE_H_

#include <malisw/mali_malisw.h>

#include <mali_kbase_debug.h>

#include <asm/page.h>

#include <linux/atomic.h>
#include <linux/highmem.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/mm_types.h>
#include <linux/mutex.h>
#include <linux/rwsem.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include "mali_base_kernel.h"
#include <mali_kbase_uku.h>
#include <mali_kbase_linux.h>

#include "mali_kbase_pm.h"
#include "mali_kbase_mem_lowlevel.h"
#include "mali_kbase_defs.h"
#include "mali_kbase_trace_timeline.h"
#include "mali_kbase_js.h"
#include "mali_kbase_mem.h"
#include "mali_kbase_security.h"
#include "mali_kbase_utility.h"
#include "mali_kbase_gpu_memory_debugfs.h"
#include "mali_kbase_mem_profile_debugfs.h"
#include "mali_kbase_jd_debugfs.h"
#include "mali_kbase_cpuprops.h"
#include "mali_kbase_gpuprops.h"
#ifdef CONFIG_GPU_TRACEPOINTS
#include <trace/events/gpu.h>
#endif


struct kbase_device *kbase_device_alloc(void);

const struct list_head *kbase_dev_list_get(void);
void kbase_dev_list_put(const struct list_head *dev_list);

mali_error kbase_device_init(struct kbase_device * const kbdev);
void kbase_device_term(struct kbase_device *kbdev);
void kbase_device_free(struct kbase_device *kbdev);
int kbase_device_has_feature(struct kbase_device *kbdev, u32 feature);
struct kbase_device *kbase_find_device(int minor);	
void kbase_release_device(struct kbase_device *kbdev);

void kbase_set_profiling_control(struct kbase_device *kbdev, u32 control, u32 value);

u32 kbase_get_profiling_control(struct kbase_device *kbdev, u32 control);

void kbase_synchronize_irqs(struct kbase_device *kbdev);
void kbase_synchronize_irqs(struct kbase_device *kbdev);

struct kbase_context *
kbase_create_context(struct kbase_device *kbdev, bool is_compat);
void kbase_destroy_context(struct kbase_context *kctx);
mali_error kbase_context_set_create_flags(struct kbase_context *kctx, u32 flags);

mali_error kbase_instr_hwcnt_setup(struct kbase_context *kctx, struct kbase_uk_hwcnt_setup *setup);
mali_error kbase_instr_hwcnt_enable(struct kbase_context *kctx, struct kbase_uk_hwcnt_setup *setup);
mali_error kbase_instr_hwcnt_disable(struct kbase_context *kctx);
mali_error kbase_instr_hwcnt_clear(struct kbase_context *kctx);
mali_error kbase_instr_hwcnt_dump(struct kbase_context *kctx);
mali_error kbase_instr_hwcnt_dump_irq(struct kbase_context *kctx);
mali_bool kbase_instr_hwcnt_dump_complete(struct kbase_context *kctx, mali_bool * const success);
void kbase_instr_hwcnt_suspend(struct kbase_device *kbdev);
void kbase_instr_hwcnt_resume(struct kbase_device *kbdev);

void kbasep_cache_clean_worker(struct work_struct *data);
void kbase_clean_caches_done(struct kbase_device *kbdev);

void kbase_instr_hwcnt_sample_done(struct kbase_device *kbdev);

mali_error kbase_jd_init(struct kbase_context *kctx);
void kbase_jd_exit(struct kbase_context *kctx);
#ifdef BASE_LEGACY_UK6_SUPPORT
mali_error kbase_jd_submit(struct kbase_context *kctx,
		const struct kbase_uk_job_submit *submit_data,
		int uk6_atom);
#else
mali_error kbase_jd_submit(struct kbase_context *kctx,
		const struct kbase_uk_job_submit *submit_data);
#endif
void kbase_jd_done(struct kbase_jd_atom *katom, int slot_nr, ktime_t *end_timestamp,
                   kbasep_js_atom_done_code done_code);
void kbase_jd_cancel(struct kbase_device *kbdev, struct kbase_jd_atom *katom);
void kbase_jd_zap_context(struct kbase_context *kctx);
mali_bool jd_done_nolock(struct kbase_jd_atom *katom);
void kbase_jd_free_external_resources(struct kbase_jd_atom *katom);
mali_bool jd_submit_atom(struct kbase_context *kctx,
			 const struct base_jd_atom_v2 *user_atom,
			 struct kbase_jd_atom *katom);

mali_error kbase_job_slot_init(struct kbase_device *kbdev);
void kbase_job_slot_halt(struct kbase_device *kbdev);
void kbase_job_slot_term(struct kbase_device *kbdev);
void kbase_job_done(struct kbase_device *kbdev, u32 done);
void kbase_job_zap_context(struct kbase_context *kctx);

void kbase_job_slot_softstop(struct kbase_device *kbdev, int js,
		struct kbase_jd_atom *target_katom);
void kbase_job_slot_softstop_swflags(struct kbase_device *kbdev, int js,
		struct kbase_jd_atom *target_katom, u32 sw_flags);
void kbase_job_slot_hardstop(struct kbase_context *kctx, int js,
		struct kbase_jd_atom *target_katom);
void kbase_job_check_enter_disjoint(struct kbase_device *kbdev, u32 action,
		u16 core_reqs, struct kbase_jd_atom *target_katom);
void kbase_job_check_leave_disjoint(struct kbase_device *kbdev,
		struct kbase_jd_atom *target_katom);

void kbase_event_post(struct kbase_context *ctx, struct kbase_jd_atom *event);
int kbase_event_dequeue(struct kbase_context *ctx, struct base_jd_event_v2 *uevent);
int kbase_event_pending(struct kbase_context *ctx);
mali_error kbase_event_init(struct kbase_context *kctx);
void kbase_event_close(struct kbase_context *kctx);
void kbase_event_cleanup(struct kbase_context *kctx);
void kbase_event_wakeup(struct kbase_context *kctx);

int kbase_process_soft_job(struct kbase_jd_atom *katom);
mali_error kbase_prepare_soft_job(struct kbase_jd_atom *katom);
void kbase_finish_soft_job(struct kbase_jd_atom *katom);
void kbase_cancel_soft_job(struct kbase_jd_atom *katom);
void kbase_resume_suspended_soft_jobs(struct kbase_device *kbdev);

bool kbase_replay_process(struct kbase_jd_atom *katom);

void kbase_reg_write(struct kbase_device *kbdev, u16 offset, u32 value, struct kbase_context *kctx);
u32 kbase_reg_read(struct kbase_device *kbdev, u16 offset, struct kbase_context *kctx);
void kbase_device_trace_register_access(struct kbase_context *kctx, enum kbase_reg_access_type type, u16 reg_offset, u32 reg_value);
void kbase_device_trace_buffer_install(struct kbase_context *kctx, u32 *tb, size_t size);
void kbase_device_trace_buffer_uninstall(struct kbase_context *kctx);

void kbase_os_reg_write(struct kbase_device *kbdev, u16 offset, u32 value);
u32 kbase_os_reg_read(struct kbase_device *kbdev, u16 offset);

void kbasep_as_do_poke(struct work_struct *work);

void kbase_report_gpu_fault(struct kbase_device *kbdev, int multiple);

void kbase_job_kill_jobs_from_context(struct kbase_context *kctx);

void kbase_gpu_interrupt(struct kbase_device *kbdev, u32 val);

mali_bool kbase_prepare_to_reset_gpu(struct kbase_device *kbdev);

mali_bool kbase_prepare_to_reset_gpu_locked(struct kbase_device *kbdev);

void kbase_reset_gpu(struct kbase_device *kbdev);

void kbase_reset_gpu_locked(struct kbase_device *kbdev);

const char *kbase_exception_name(u32 exception_code);

static INLINE mali_bool kbase_pm_is_suspending(struct kbase_device *kbdev) {
	return kbdev->pm.suspending;
}

static INLINE int kbase_jd_atom_id(struct kbase_context *kctx, struct kbase_jd_atom *katom)
{
	int result;
	KBASE_DEBUG_ASSERT(kctx);
	KBASE_DEBUG_ASSERT(katom);
	KBASE_DEBUG_ASSERT(katom->kctx == kctx);

	result = katom - &kctx->jctx.atoms[0];
	KBASE_DEBUG_ASSERT(result >= 0 && result <= BASE_JD_ATOM_COUNT);
	return result;
}

void kbase_disjoint_init(struct kbase_device *kbdev);

void kbase_disjoint_event(struct kbase_device *kbdev);

void kbase_disjoint_event_potential(struct kbase_device *kbdev);

u32 kbase_disjoint_event_get(struct kbase_device *kbdev);

void kbase_disjoint_state_up(struct kbase_device *kbdev);

void kbase_disjoint_state_down(struct kbase_device *kbdev);

#define KBASE_DISJOINT_STATE_INTERLEAVED_CONTEXT_COUNT_THRESHOLD 2

#if KBASE_TRACE_ENABLE
#ifndef CONFIG_MALI_SYSTEM_TRACE
#define KBASE_TRACE_ADD_SLOT(kbdev, code, ctx, katom, gpu_addr, jobslot) \
	kbasep_trace_add(kbdev, KBASE_TRACE_CODE(code), ctx, katom, gpu_addr, \
			KBASE_TRACE_FLAG_JOBSLOT, 0, jobslot, 0)

#define KBASE_TRACE_ADD_SLOT_INFO(kbdev, code, ctx, katom, gpu_addr, jobslot, info_val) \
	kbasep_trace_add(kbdev, KBASE_TRACE_CODE(code), ctx, katom, gpu_addr, \
			KBASE_TRACE_FLAG_JOBSLOT, 0, jobslot, info_val)

#define KBASE_TRACE_ADD_REFCOUNT(kbdev, code, ctx, katom, gpu_addr, refcount) \
	kbasep_trace_add(kbdev, KBASE_TRACE_CODE(code), ctx, katom, gpu_addr, \
			KBASE_TRACE_FLAG_REFCOUNT, refcount, 0, 0)
#define KBASE_TRACE_ADD_REFCOUNT_INFO(kbdev, code, ctx, katom, gpu_addr, refcount, info_val) \
	kbasep_trace_add(kbdev, KBASE_TRACE_CODE(code), ctx, katom, gpu_addr, \
			KBASE_TRACE_FLAG_REFCOUNT, refcount, 0, info_val)

#define KBASE_TRACE_ADD(kbdev, code, ctx, katom, gpu_addr, info_val)     \
	kbasep_trace_add(kbdev, KBASE_TRACE_CODE(code), ctx, katom, gpu_addr, \
			0, 0, 0, info_val)

#define KBASE_TRACE_CLEAR(kbdev) \
	kbasep_trace_clear(kbdev)

#define KBASE_TRACE_DUMP(kbdev) \
	kbasep_trace_dump(kbdev)

void kbasep_trace_add(struct kbase_device *kbdev, enum kbase_trace_code code, void *ctx, struct kbase_jd_atom *katom, u64 gpu_addr, u8 flags, int refcount, int jobslot, unsigned long info_val);
void kbasep_trace_clear(struct kbase_device *kbdev);
#else 
#include <mali_linux_kbase_trace.h>
#define KBASE_TRACE_ADD_SLOT(kbdev, code, ctx, katom, gpu_addr, jobslot)\
	trace_mali_##code(jobslot, 0)

#define KBASE_TRACE_ADD_SLOT_INFO(kbdev, code, ctx, katom, gpu_addr, jobslot, info_val)\
	trace_mali_##code(jobslot, info_val)

#define KBASE_TRACE_ADD_REFCOUNT(kbdev, code, ctx, katom, gpu_addr, refcount)\
	trace_mali_##code(refcount, 0)

#define KBASE_TRACE_ADD_REFCOUNT_INFO(kbdev, code, ctx, katom, gpu_addr, refcount, info_val)\
	trace_mali_##code(refcount, info_val)

#define KBASE_TRACE_ADD(kbdev, code, ctx, katom, gpu_addr, info_val)\
	trace_mali_##code(gpu_addr, info_val)

#define KBASE_TRACE_CLEAR(kbdev)\
	do {\
		CSTD_UNUSED(kbdev);\
		CSTD_NOP(0);\
	} while (0)
#define KBASE_TRACE_DUMP(kbdev)\
	do {\
		CSTD_UNUSED(kbdev);\
		CSTD_NOP(0);\
	} while (0)

#endif 
#else
#define KBASE_TRACE_ADD_SLOT(kbdev, code, ctx, katom, gpu_addr, jobslot)\
	do {\
		CSTD_UNUSED(kbdev);\
		CSTD_NOP(code);\
		CSTD_UNUSED(ctx);\
		CSTD_UNUSED(katom);\
		CSTD_UNUSED(gpu_addr);\
		CSTD_UNUSED(jobslot);\
	} while (0)

#define KBASE_TRACE_ADD_SLOT_INFO(kbdev, code, ctx, katom, gpu_addr, jobslot, info_val)\
	do {\
		CSTD_UNUSED(kbdev);\
		CSTD_NOP(code);\
		CSTD_UNUSED(ctx);\
		CSTD_UNUSED(katom);\
		CSTD_UNUSED(gpu_addr);\
		CSTD_UNUSED(jobslot);\
		CSTD_UNUSED(info_val);\
		CSTD_NOP(0);\
	} while (0)

#define KBASE_TRACE_ADD_REFCOUNT(kbdev, code, ctx, katom, gpu_addr, refcount)\
	do {\
		CSTD_UNUSED(kbdev);\
		CSTD_NOP(code);\
		CSTD_UNUSED(ctx);\
		CSTD_UNUSED(katom);\
		CSTD_UNUSED(gpu_addr);\
		CSTD_UNUSED(refcount);\
		CSTD_NOP(0);\
	} while (0)

#define KBASE_TRACE_ADD_REFCOUNT_INFO(kbdev, code, ctx, katom, gpu_addr, refcount, info_val)\
	do {\
		CSTD_UNUSED(kbdev);\
		CSTD_NOP(code);\
		CSTD_UNUSED(ctx);\
		CSTD_UNUSED(katom);\
		CSTD_UNUSED(gpu_addr);\
		CSTD_UNUSED(info_val);\
		CSTD_NOP(0);\
	} while (0)

#define KBASE_TRACE_ADD(kbdev, code, subcode, ctx, katom, val)\
	do {\
		CSTD_UNUSED(kbdev);\
		CSTD_NOP(code);\
		CSTD_UNUSED(subcode);\
		CSTD_UNUSED(ctx);\
		CSTD_UNUSED(katom);\
		CSTD_UNUSED(val);\
		CSTD_NOP(0);\
	} while (0)

#define KBASE_TRACE_CLEAR(kbdev)\
	do {\
		CSTD_UNUSED(kbdev);\
		CSTD_NOP(0);\
	} while (0)
#define KBASE_TRACE_DUMP(kbdev)\
	do {\
		CSTD_UNUSED(kbdev);\
		CSTD_NOP(0);\
	} while (0)
#endif 
void kbasep_trace_dump(struct kbase_device *kbdev);
#endif
