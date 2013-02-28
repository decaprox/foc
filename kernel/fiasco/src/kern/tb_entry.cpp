INTERFACE:

#include "initcalls.h"
#include "l4_error.h"

enum {
  Tbuf_unused             = 0,
  Tbuf_pf,
  Tbuf_ipc,
  Tbuf_ipc_res,
  Tbuf_ipc_trace,
  Tbuf_ke,
  Tbuf_ke_reg,
  Tbuf_exregs,
  Tbuf_breakpoint,
  Tbuf_pf_res,
  Tbuf_preemption,
  Tbuf_ke_bin,
  Tbuf_dynentries,

  Tbuf_max                = 0x80,
  Tbuf_hidden             = 0x80,
};

#include "l4_types.h"

class Tb_entry;
class Context;
class Space;
class Sched_context;
class Syscall_frame;
class Trap_state;
class Tb_entry_formatter;

struct Tb_log_table_entry
{
  char const *name;
  unsigned char *patch;
  Tb_entry_formatter *fmt;
};

extern Tb_log_table_entry _log_table[];
extern Tb_log_table_entry _log_table_end;



class Tb_entry
{
protected:
  Mword		_number;	///< event number
  Address	_ip;		///< instruction pointer
  Context const *_ctx;		///< Context
  Unsigned64	_tsc;		///< time stamp counter
  Unsigned32	_pmc1;		///< performance counter value 1
  Unsigned32	_pmc2;		///< performance counter value 2
  Unsigned32	_kclock;	///< lower 32 bits of kernel clock
  Unsigned8	_type;		///< type of entry
  Unsigned8     _cpu;           ///< CPU

  static Mword (*rdcnt1)();
  static Mword (*rdcnt2)();

public:
  class Group_order
  {
  public:
    Group_order() : _o(0) {} // not grouped
    Group_order(unsigned v) : _o(2 + v) {}
    static Group_order none() { return Group_order(); }
    static Group_order last() { return Group_order(255, true); }
    static Group_order first() { return Group_order(0); }
    static Group_order direct() { return Group_order(1, true); }

    bool not_grouped() const { return _o == 0; }
    bool is_direct() const { return _o == 1; }
    bool is_first() const { return _o == 2; }
    bool is_last() const { return _o == 255; }
    bool grouped() const { return _o >= 2; }
    unsigned char depth() const { return _o - 2; }

  private:
    Group_order(unsigned char v, bool) : _o(v) {}
    unsigned char _o;
  };

  Group_order has_partner() const { return Group_order::none(); }
  Group_order is_partner(Tb_entry const *) const { return Group_order::none(); }
  Mword partner() const { return 0; }

} __attribute__((__packed__, __aligned__(8)));


class Tb_entry_union : public Tb_entry
{
private:
  char _padding[Tb_entry_size - sizeof(Tb_entry)];
};

static_assert(sizeof(Tb_entry_union) == Tb_entry::Tb_entry_size,
              "Tb_entry_union has the wrong size");

struct Tb_entry_empty : public Tb_entry
{
  unsigned print(int, char *) const { return 0; }
};

class Tb_entry_formatter
{
public:
  Tb_entry_formatter() { }
  typedef Tb_entry::Group_order Group_order;

  virtual unsigned print(Tb_entry const *e, int max, char *buf) const = 0;
  virtual Group_order has_partner(Tb_entry const *e) const = 0;
  virtual Group_order is_pair(Tb_entry const *e, Tb_entry const *n) const = 0;
  virtual Mword partner(Tb_entry const *e) const = 0;

  static Tb_entry_formatter const *get_fmt(Tb_entry const *e)
  {
    if (e->type() >= Tbuf_dynentries)
      return _log_table[e->type() - Tbuf_dynentries].fmt;

    return _fixed[e->type()];
  }

private:
  static Tb_entry_formatter const *_fixed[];
};


template< typename T >
class Tb_entry_formatter_t : public Tb_entry_formatter
{
public:
  typedef T const *Const_ptr;
  typedef T *Ptr;

  unsigned print(Tb_entry const *e, int max, char *buf) const
  { return static_cast<Const_ptr>(e)->print(max, buf); }

  Group_order has_partner(Tb_entry const *e) const
  { return static_cast<Const_ptr>(e)->has_partner(); }

  Group_order is_pair(Tb_entry const *e, Tb_entry const *n) const
  {
    //assert (get_fmt(e) == &singleton);

    if (&singleton == get_fmt(n))
      return static_cast<Const_ptr>(e)->is_partner(static_cast<Const_ptr>(n));
    return Tb_entry::Group_order::none();
  }

  Mword partner(Tb_entry const *e) const
  { return static_cast<Const_ptr>(e)->partner(); }

  static Tb_entry_formatter_t const singleton;
};

template<typename T>
Tb_entry_formatter_t<T> const Tb_entry_formatter_t<T>::singleton;


/** logged ipc. */
class Tb_entry_ipc : public Tb_entry
{
private:
  L4_msg_tag	_tag;           ///< message tag
  Mword	_dword[2];	///< first two message words
  L4_obj_ref	_dst;		///< destination id
  Mword       _dbg_id;
  Mword       _label;
  L4_timeout_pair _timeout;	///< timeout
public:
  Tb_entry_ipc() : _timeout(0) {}
  unsigned print(int max, char *buf) const;
};

/** logged ipc result. */
class Tb_entry_ipc_res : public Tb_entry
{
private:
  L4_msg_tag	_tag;		///< message tag
  Mword	_dword[2];	///< first two dwords
  L4_error	_result;	///< result
  Mword	_from;		///< receive descriptor
  Mword	_pair_event;	///< referred event
  Unsigned8	_have_snd;	///< ipc had send part
  Unsigned8	_is_np;		///< next period bit set
public:
  unsigned print(int max, char *buf) const;
};

/** logged ipc for user level tracing with Vampir. */
class Tb_entry_ipc_trace : public Tb_entry
{
private:
  Unsigned64	_snd_tsc;	///< entry tsc
  L4_msg_tag	_result;	///< result
  L4_obj_ref	_snd_dst;	///< send destination
  Mword	_rcv_dst;	///< rcv partner
  Unsigned8	_snd_desc;
  Unsigned8	_rcv_desc;
public:
  unsigned print(int max, char *buf) const;
};

#if 0
/** logged short-cut ipc failed. */
class Tb_entry_ipc_sfl : public Tb_entry_base
{
private:
  Global_id	_from;	///< short ipc rcv descriptor
  L4_timeout_pair	_timeout;	///< ipc timeout
  Global_id	_dst;		///< partner
  Unsigned8	_is_irq, _snd_lst, _dst_ok, _dst_lck, _preempt;
};
#endif

/** logged pagefault. */
class Tb_entry_pf : public Tb_entry
{
private:
  Address	_pfa;		///< pagefault address
  Mword	_error;		///< pagefault error code
  Space	*_space;
public:
  unsigned print(int max, char *buf) const;
};

/** pagefault result. */
class Tb_entry_pf_res : public Tb_entry
{
private:
  Address	_pfa;
  L4_error	_err;
  L4_error	_ret;
public:
  unsigned print(int max, char *buf) const;
};


/** logged kernel event. */
template<typename BASE, unsigned TAG>
class Tb_entry_ke_t : public BASE
{
protected:
  union Msg
  {
    char msg[BASE::Tb_entry_size - sizeof(BASE)];
    struct Ptr
    {
      char tag[2];
      char const *ptr;
    } mptr;
  } _msg;
} __attribute__((__packed__));

typedef Tb_entry_ke_t<Tb_entry, Tbuf_ke> Tb_entry_ke;

class Tb_entry_ke_reg_b : public Tb_entry
{
public:
  Mword v[3];
} __attribute__((__packed__));

class Tb_entry_ke_reg : public Tb_entry_ke_t<Tb_entry_ke_reg_b, Tbuf_ke_reg>
{
};

/** logged breakpoint. */
class Tb_entry_bp : public Tb_entry
{
private:
  Address	_address;	///< breakpoint address
  int		_len;		///< breakpoint length
  Mword	_value;		///< value at address
  int		_mode;		///< breakpoint mode
public:
  unsigned print(int max, char *buf) const;
};

/** logged context switch. */
class Tb_entry_ctx_sw : public Tb_entry
{
public:
  using Tb_entry::_ip;

  Context const *dst;		///< switcher target
  Context const *dst_orig;
  Address kernel_ip;
  Mword lock_cnt;
  Space const *from_space;
  Sched_context const *from_sched;
  Mword from_prio;
  unsigned print(int max, char *buf) const;
} __attribute__((packed));

/** logged scheduling event. */
class Tb_entry_sched : public Tb_entry
{
public:
  unsigned short mode;
  Context const *owner;
  unsigned short id;
  unsigned short prio;
  signed long left;
  unsigned long quantum;

  unsigned print(int max, char *buf) const;
} __attribute__((packed));

/** logged binary kernel event. */
class Tb_entry_ke_bin : public Tb_entry
{
public:
  char _msg[Tb_entry_size - sizeof(Tb_entry)];
  enum { SIZE = 30 };
};



IMPLEMENTATION:

#include <cstring>
#include <cstdarg>

#include "entry_frame.h"
#include "globals.h"
#include "kip.h"
#include "static_init.h"
#include "trap_state.h"


PROTECTED static Mword Tb_entry::dummy_read_pmc() { return 0; }

Mword (*Tb_entry::rdcnt1)() = dummy_read_pmc;
Mword (*Tb_entry::rdcnt2)() = dummy_read_pmc;
Tb_entry_formatter const *Tb_entry_formatter::_fixed[Tbuf_dynentries];


PUBLIC static
void
Tb_entry_formatter::set_fixed(unsigned type, Tb_entry_formatter const *f)
{
  if (type >= Tbuf_dynentries)
    return;

  _fixed[type] = f;
}


PUBLIC static
void
Tb_entry::set_rdcnt(int num, Mword (*f)())
{
  if (!f)
    f = dummy_read_pmc;

  switch (num)
    {
    case 0: rdcnt1 = f; break;
    case 1: rdcnt2 = f; break;
    }
}

PUBLIC inline
void
Tb_entry::clear()
{ _type = Tbuf_unused; }

PUBLIC inline NEEDS["kip.h", "globals.h"]
void
Tb_entry::set_global(char type, Context const *ctx, Address ip)
{
  _type   = type;
  _ctx    = ctx;
  _ip     = ip;
  _kclock = (Unsigned32)Kip::k()->clock;
  _cpu    = current_cpu();
}

PUBLIC inline
void
Tb_entry::hide()
{ _type |= Tbuf_hidden; }

PUBLIC inline
void
Tb_entry::unhide()
{ _type &= ~Tbuf_hidden; }

PUBLIC inline
Address
Tb_entry::ip() const
{ return _ip; }

PUBLIC inline
Context const *
Tb_entry::ctx() const
{ return _ctx; }

PUBLIC inline
Unsigned8
Tb_entry::type() const
{ return _type & (Tbuf_max-1); }

PUBLIC inline
int
Tb_entry::hidden() const
{ return _type & Tbuf_hidden; }

PUBLIC inline
Mword
Tb_entry::number() const
{ return _number; }

PUBLIC inline
void
Tb_entry::number(Mword number)
{ _number = number; }

PUBLIC inline
void
Tb_entry::rdpmc1()
{ _pmc1 = rdcnt1(); }

PUBLIC inline
void
Tb_entry::rdpmc2()
{ _pmc2 = rdcnt2(); }

PUBLIC inline
Unsigned32
Tb_entry::kclock() const
{ return _kclock; }

PUBLIC inline
Unsigned8
Tb_entry::cpu() const
{ return _cpu; }

PUBLIC inline
Unsigned64
Tb_entry::tsc() const
{ return _tsc; }

PUBLIC inline
Unsigned32
Tb_entry::pmc1() const
{ return _pmc1; }

PUBLIC inline
Unsigned32
Tb_entry::pmc2() const
{ return _pmc2; }


PUBLIC inline NEEDS ["entry_frame.h"]
void
Tb_entry_ipc::set(Context const *ctx, Mword ip, Syscall_frame *ipc_regs, Utcb *utcb,
		  Mword dbg_id, Unsigned64 left)
{
  set_global(Tbuf_ipc, ctx, ip);
  _dst       = ipc_regs->ref();
  _label     = ipc_regs->from_spec();


  _dbg_id = dbg_id;

  _timeout   = ipc_regs->timeout();
  _tag       = ipc_regs->tag();
  if (ipc_regs->next_period())
    {
      _dword[0]  = (Unsigned32)(left & 0xffffffff);
      _dword[1]  = (Unsigned32)(left >> 32);
    }
  else
    {
      // hint for gcc
      register Mword tmp0 = utcb->values[0];
      register Mword tmp1 = utcb->values[1];
      _dword[0]  = tmp0;
      _dword[1]  = tmp1;
    }
}

PUBLIC inline
Mword
Tb_entry_ipc::ipc_type() const
{ return _dst.op(); }

PUBLIC inline
Mword
Tb_entry_ipc::dbg_id() const
{ return _dbg_id; }

PUBLIC inline
L4_obj_ref
Tb_entry_ipc::dst() const
{ return _dst; }

PUBLIC inline
L4_timeout_pair
Tb_entry_ipc::timeout() const
{ return _timeout; }

PUBLIC inline
L4_msg_tag
Tb_entry_ipc::tag() const
{ return _tag; }

PUBLIC inline
Mword
Tb_entry_ipc::label() const
{ return _label; }

PUBLIC inline
Mword
Tb_entry_ipc::dword(unsigned index) const
{ return _dword[index]; }


PUBLIC inline NEEDS ["entry_frame.h"]
void
Tb_entry_ipc_res::set(Context const *ctx, Mword ip, Syscall_frame *ipc_regs,
                      Utcb *utcb,
		      Mword result, Mword pair_event, Unsigned8 have_snd,
		      Unsigned8 is_np)
{
  set_global(Tbuf_ipc_res, ctx, ip);
  // hint for gcc
  register Mword tmp0 = utcb->values[0];
  register Mword tmp1 = utcb->values[1];
  _dword[0]   = tmp0;
  _dword[1]   = tmp1;
  _tag        = ipc_regs->tag();
  _pair_event = pair_event;
  _result     = L4_error::from_raw(result);
  _from       = ipc_regs->from_spec();
  _have_snd   = have_snd;
  _is_np      = is_np;
}

PUBLIC inline
int
Tb_entry_ipc_res::have_snd() const
{ return _have_snd; }

PUBLIC inline
int
Tb_entry_ipc_res::is_np() const
{ return _is_np; }

PUBLIC inline
Mword
Tb_entry_ipc_res::from() const
{ return _from; }

PUBLIC inline
L4_error
Tb_entry_ipc_res::result() const
{ return _result; }

PUBLIC inline
L4_msg_tag
Tb_entry_ipc_res::tag() const
{ return _tag; }

PUBLIC inline
Mword
Tb_entry_ipc_res::dword(unsigned index) const
{ return _dword[index]; }

PUBLIC inline
Mword
Tb_entry_ipc_res::pair_event() const
{ return _pair_event; }


PUBLIC inline
void
Tb_entry_ipc_trace::set(Context const *ctx, Mword ip, Unsigned64 snd_tsc,
			L4_obj_ref const &snd_dst, Mword rcv_dst,
			L4_msg_tag result, Unsigned8 snd_desc,
			Unsigned8 rcv_desc)
{
  set_global(Tbuf_ipc_trace, ctx, ip);
  _snd_tsc  = snd_tsc;
  _snd_dst  = snd_dst;
  _rcv_dst  = rcv_dst;
  _result   = result;
  _snd_desc = snd_desc;
  _rcv_desc = rcv_desc;
}

PUBLIC inline
void
Tb_entry_pf::set(Context const *ctx, Address ip, Address pfa,
		 Mword error, Space *spc)
{
  set_global(Tbuf_pf, ctx, ip);
  _pfa   = pfa;
  _error = error;
  _space = spc;
}

PUBLIC inline
Mword
Tb_entry_pf::error() const
{ return _error; }

PUBLIC inline
Address
Tb_entry_pf::pfa() const
{ return _pfa; }

PUBLIC inline
Space*
Tb_entry_pf::space() const
{ return _space; }


PUBLIC inline
void
Tb_entry_pf_res::set(Context const *ctx, Address ip, Address pfa, 
		     L4_error err, L4_error ret)
{
  set_global(Tbuf_pf_res, ctx, ip);
  _pfa = pfa;
  _err = err;
  _ret = ret;
}

PUBLIC inline
Address
Tb_entry_pf_res::pfa() const
{ return _pfa; }

PUBLIC inline
L4_error
Tb_entry_pf_res::err() const
{ return _err; }

PUBLIC inline
L4_error
Tb_entry_pf_res::ret() const
{ return _ret; }


PUBLIC inline
void
Tb_entry_bp::set(Context const *ctx, Address ip,
		 int mode, int len, Mword value, Address address)
{
  set_global(Tbuf_breakpoint, ctx, ip);
  _mode    = mode;
  _len     = len;
  _value   = value;
  _address = address;
}



PUBLIC inline
int
Tb_entry_bp::mode() const
{ return _mode; }

PUBLIC inline
int
Tb_entry_bp::len() const
{ return _len; }

PUBLIC inline
Mword
Tb_entry_bp::value() const
{ return _value; }

PUBLIC inline
Address
Tb_entry_bp::addr() const
{ return _address; }



PUBLIC template<typename BASE, unsigned TAG> inline
void
Tb_entry_ke_t<BASE, TAG>::set(Context const *ctx, Address ip)
{ this->set_global(TAG, ctx, ip); }

PUBLIC template<typename BASE, unsigned TAG> inline
void
Tb_entry_ke_t<BASE, TAG>::set_const(Context const *ctx, Address ip, char const *msg)
{
  this->set_global(TAG, ctx, ip);
  _msg.mptr.tag[0] = 0;
  _msg.mptr.tag[1] = 1;
  _msg.mptr.ptr = msg;
}

PUBLIC template<typename BASE, unsigned TAG> inline
void
Tb_entry_ke_t<BASE, TAG>::set_buf(unsigned i, char c)
{
  if (i < sizeof(_msg.msg)-1)
    _msg.msg[i] = c >= ' ' ? c : '.';
}

PUBLIC template<typename BASE, unsigned TAG> inline
void
Tb_entry_ke_t<BASE, TAG>::term_buf(unsigned i)
{
  _msg.msg[i < sizeof(_msg.msg)-1 ? i : sizeof(_msg.msg)-1] = '\0';
}

PUBLIC template<typename BASE, unsigned TAG> inline
char const *
Tb_entry_ke_t<BASE, TAG>::msg() const
{
  return _msg.mptr.tag[0] == 0 && _msg.mptr.tag[1] == 1 ? _msg.mptr.ptr : _msg.msg;
}

PUBLIC inline
void
Tb_entry_ke_bin::set(Context const *ctx, Address ip)
{ set_global(Tbuf_ke_bin, ctx, ip); }

PUBLIC inline
void
Tb_entry_ke_bin::set_buf(unsigned i, char c)
{
  if (i < sizeof(_msg)-1)
    _msg[i] = c;
}

PUBLIC inline
void
Tb_entry_ke_reg::set_const(Context const *ctx, Mword eip,
                           const char *msg,
                           Mword v1, Mword v2, Mword v3)
{
  Tb_entry_ke_t<Tb_entry_ke_reg_b, Tbuf_ke_reg>::set_const(ctx, eip, msg);
  v[0] = v1;
  v[1] = v2;
  v[2] = v3;
}

