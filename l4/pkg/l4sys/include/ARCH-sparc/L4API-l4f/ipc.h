/**
 * \file
 * \brief   L4 IPC System Calls, Sparc
 * \ingroup api_calls
 */
/*
 * (c) 2010    Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *             Björn Döbel <doebel@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 */
#pragma once

#include_next <l4/sys/ipc.h>

#ifdef __GNUC__

#include <l4/sys/compiler.h>
#include <l4/sys/syscall_defs.h>

L4_INLINE l4_msgtag_t
l4_ipc_call(l4_cap_idx_t dest, l4_utcb_t *utcb,
            l4_msgtag_t tag,
            l4_timeout_t timeout) L4_NOTHROW
{
	(void)dest;
	(void)utcb;
	(void)tag;
	(void)timeout;
	l4_msgtag_t t;
	t.raw = ~0;
	return t;
}

L4_INLINE l4_msgtag_t
l4_ipc_reply_and_wait(l4_utcb_t *utcb, l4_msgtag_t tag,
                      l4_umword_t *label,
                      l4_timeout_t timeout) L4_NOTHROW
{
	(void)utcb;
	(void)tag;
	(void)label;
	(void)timeout;
	l4_msgtag_t t;
	t.raw = ~0;
	return t;
}


L4_INLINE l4_msgtag_t
l4_ipc_send_and_wait(l4_cap_idx_t dest, l4_utcb_t *utcb,
                     l4_msgtag_t tag,
                     l4_umword_t *src,
                     l4_timeout_t timeout) L4_NOTHROW
{
	(void)dest;
	(void)utcb;
	(void)tag;
	(void)src;
	(void)timeout;
	l4_msgtag_t t;
	t.raw = ~0;
	return t;
}

L4_INLINE l4_msgtag_t
l4_ipc_send(l4_cap_idx_t dest, l4_utcb_t *utcb,
            l4_msgtag_t tag,
            l4_timeout_t timeout) L4_NOTHROW
{
	(void)dest;
	(void)utcb;
	(void)tag;
	(void)timeout;
	l4_msgtag_t t;
	t.raw = ~0;
	return t;
}

L4_INLINE l4_msgtag_t
l4_ipc_wait(l4_utcb_t *utcb, l4_umword_t *src,
            l4_timeout_t timeout) L4_NOTHROW
{
	(void)utcb;
	(void)src;
	(void)timeout;
	l4_msgtag_t t;
	t.raw = ~0;
	return t;
}

L4_INLINE l4_msgtag_t
l4_ipc_receive(l4_cap_idx_t src, l4_utcb_t *utcb,
               l4_timeout_t timeout) L4_NOTHROW
{
	(void)utcb;
	(void)src;
	(void)timeout;
	l4_msgtag_t t;
	t.raw = ~0;
	return t;
}

// todo: let all calls above use this single call
L4_INLINE l4_msgtag_t
l4_ipc(l4_cap_idx_t dest, l4_utcb_t *utcb,
       l4_umword_t flags,
       l4_umword_t slabel,
       l4_msgtag_t tag,
       l4_umword_t *rlabel,
       l4_timeout_t timeout) L4_NOTHROW
{
	(void)dest;
	(void)utcb;
	(void)flags;
	(void)slabel;
	(void)tag;
	(void)rlabel;
	(void)timeout;
	l4_msgtag_t t;
	t.raw = ~0;
	return t;
}


#include <l4/sys/ipc-impl.h>

#endif //__GNUC__

