/******************************************************************************

 MIT License

 Copyright (c) 2018 kieme, frits.germs@gmx.net

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

******************************************************************************/

#ifndef _DAINTY_MT_EVENT_DISPATCHER_H_
#define _DAINTY_MT_EVENT_DISPATCHER_H_

#include <vector>
#include "dainty_os_fdbased.h"
#include "dainty_container_freelist.h"
#include "dainty_named_string.h"
#include "dainty_mt_err.h"

namespace dainty
{
namespace mt
{
namespace event_dispatcher
{
///////////////////////////////////////////////////////////////////////////////

  using named::t_void;
  using named::t_bool;
  using named::t_n;
  using named::t_ix;
  using named::string::t_string;
  using named::string::FMT;
  using named::t_validity;
  using named::VALID;
  using named::INVALID;
  using os::t_fd;
  using os::BAD_FD;

  using container::freelist::t_id;
  using t_ids = std::vector<t_id>;
  using r_ids = named::t_prefix<t_ids>::r_;

  enum  t_event_user_tag_ {};
  using t_event_user = named::t_user<t_event_user_tag_>;

  enum  t_service_name_tag_ {};
  using t_service_name = t_string<t_service_name_tag_, 20>;
  using R_service_name = named::t_prefix<t_service_name>::R_;

  enum  t_hook_name_tag_ {};
  using t_hook_name = t_string<t_hook_name_tag_, 20>;
  using R_hook_name = named::t_prefix<t_hook_name>::R_;

  using t_event_prio   = named::t_uchar;
  using t_microseconds = named::t_uint;
  using t_quit         = named::t_bool;
  enum  t_event_type { RD, WR };
  enum  t_cmd        { QUIT_EVENT_LOOP, REMOVE_EVENT, CONTINUE };

  using t_errno = named::t_uint32; //XXX

///////////////////////////////////////////////////////////////////////////////

  t_n            get_supported_services();
  t_service_name get_supported_service (t_ix);

///////////////////////////////////////////////////////////////////////////////

  class t_event_params {
  public:
    const t_event_type type;
    const t_event_prio prio;
          t_event_user user;

    inline
    t_event_params(t_event_type _type, t_event_prio _prio, t_event_user _user)
      : type(_type), prio(_prio), user(_user) {
    }
  };

  using r_event_params = named::t_prefix<t_event_params>::r_;
  using R_event_params = named::t_prefix<t_event_params>::R_;

///////////////////////////////////////////////////////////////////////////////

  class t_event_hook;
  using p_event_hook = named::t_prefix<t_event_hook>::p_;

  class t_return {
  public:
    t_cmd        cmd;
    p_event_hook hook;

    inline
    t_return(t_cmd _cmd = REMOVE_EVENT, p_event_hook _hook = nullptr)
     : cmd(_cmd), hook(_hook) {
    }
  };

///////////////////////////////////////////////////////////////////////////////

  class t_event_hook {
  public:
    virtual ~t_event_hook() { }
    virtual t_hook_name get_name() const = 0;
    virtual t_return    notify_event(t_fd, r_event_params) = 0;
  };

///////////////////////////////////////////////////////////////////////////////

  class t_event_info {
  public:
    t_id           id = t_id{0};
    t_fd           fd;
    p_event_hook   hook;
    t_event_params params;

    inline
    t_event_info(t_fd _fd, p_event_hook _hook, R_event_params _params)
      : fd(_fd), hook(_hook), params(_params) {
    }
  };

  using r_event_info = named::t_prefix<t_event_info>::r_;
  using P_event_info = named::t_prefix<t_event_info>::P_;

  using t_event_infos = std::vector<t_event_info*>;
  using r_event_infos = named::t_prefix<t_event_infos>::r_;

///////////////////////////////////////////////////////////////////////////////

  class t_params {
  public:
    t_n            max;
    t_service_name service_name;

    inline
    t_params(t_n _max, R_service_name _name) : max(_max), service_name(_name) {
    }
  };

  using R_params = named::t_prefix<t_params>::R_;

///////////////////////////////////////////////////////////////////////////////

  class t_impl_;
  using p_impl_ = named::t_prefix<t_impl_>::p_;

  class t_dispatcher {
  public:
    class t_logic {
    public:
      virtual ~t_logic() { }

      virtual t_void may_reorder_events (r_event_infos)  = 0;
      virtual t_void notify_event_remove(r_event_info)   = 0;
      virtual t_quit notify_timeout     (t_microseconds) = 0;
      virtual t_quit notify_error       (t_errno)        = 0;
    };

    using p_logic = named::t_prefix<t_logic>::p_;

    t_dispatcher(       R_params, p_logic, t_bool = false);
    t_dispatcher(t_err, R_params, p_logic, t_bool = false);
    t_dispatcher(t_dispatcher&&);
   ~t_dispatcher();

    t_dispatcher()                                = delete;
    t_dispatcher(const t_dispatcher&)             = delete;
    t_dispatcher& operator=(t_dispatcher&&)       = delete;
    t_dispatcher& operator=(const t_dispatcher&)  = delete;

    operator t_validity() const;
    t_params get_params() const;
    t_void   display   () const;

    t_id         add_event(       t_fd, R_event_params, p_event_hook);
    t_id         add_event(t_err, t_fd, R_event_params, p_event_hook);
    p_event_hook del_event(       t_fd);
    p_event_hook del_event(t_err, t_fd);
    p_event_hook del_event(       t_id);
    p_event_hook del_event(t_err, t_id);

    t_void       clear_events();
    t_void       clear_events(t_err);

    P_event_info get_event(       t_id) const;
    P_event_info get_event(t_err, t_id) const;

    t_bool       fetch_events(r_ids) const;

    t_n event_loop();
    t_n event_loop(t_err);

    t_n event_loop(       t_microseconds);
    t_n event_loop(t_err, t_microseconds);

  private:
    p_impl_ impl_ = nullptr;
  };

///////////////////////////////////////////////////////////////////////////////
}
}
}

#endif
