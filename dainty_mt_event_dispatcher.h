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

  using named::t_n;
  using named::t_ix;
  using named::string::t_string;
  using named::string::FMT;
  using container::freelist::t_id;
  using os::t_fd;

  using t_event_prio = named::t_uchar;
  enum  t_event_user_tag_ {};
  using t_event_user = named::t_user<t_event_user_tag_>;
  enum  t_service_name_tag_ {};
  using t_service_name = t_string<t_service_name_tag_, 20>;
  using R_service_name = named::t_prefix<t_service_name>::R_;
  enum  t_hook_name_tag_ {};
  using t_hook_name = t_string<t_hook_name_tag_, 20>;
  using R_hook_name = named::t_prefix<t_hook_name>::R_;
  enum t_event_type { RX, WR };
  enum t_cmd        { QUIT_EVENT_LOOP, REMOVE_HOOK, CONTINUE };

///////////////////////////////////////////////////////////////////////////////

  t_n            get_supported_services();
  t_service_name get_supported_service (t_ix);

///////////////////////////////////////////////////////////////////////////////

  class t_event_params {
  public:
    const t_event_type type;
    const t_event_prio prio;
    t_event_user       user;

    t_event_params(t_event_type, t_event_prio, t_event_user);
  };

  using r_event_params = named::t_prefix<t_event_params>::r_;

///////////////////////////////////////////////////////////////////////////////

  class t_event_hook {
  public:
    virtual ~t_event_hook() { }
    virtual t_hook_name get_name() const = 0;
    virtual t_return    notify_event(t_fd, r_event_params) = 0;
  };

  using p_event_hook = named::t_prefix<t_event_hook>::p_;

///////////////////////////////////////////////////////////////////////////////

  class t_event_info {
  public:
    t_fd           fd;
    p_event_hook   hook;
    t_event_params params;

    t_event_info(t_fd _fd, p_event_hook _hook, R_event_params _params)
      : fd(_fd), hook(_hook), params(_params) {
    }
  };

///////////////////////////////////////////////////////////////////////////////

  class t_params {
  public:
    t_n            max;
    t_service_name service_name;

    t_params(t_n _max, R_service_name _name) : max(_max), service_name(_name) {
    }
  };

  using R_params = named::t_prefix<t_params>::R_;

///////////////////////////////////////////////////////////////////////////////

  class t_return {
  public:
    t_cmd        cmd;
    p_event_hook hook;

    t_return(t_cmd cmd = REMOVE_HOOK, p_event_hook _hook = nullptr)
     : cmd(_cmd), hook(_hook) {
    }
  };

///////////////////////////////////////////////////////////////////////////////

  class t_logic {
  public:
    virtual ~t_logic() { }

    using container::list::t_list<>;

    virtual t_void order                 (pevents& unordered);
    virtual t_void notify_events_detected(pevents& ordered) const;
    virtual t_void notify_event_remove   (r_event_info);
    virtual t_cmd  notify_timeout        (t_microsecond);
    virtual t_cmd  notify_error          (t_errno);
  };

  using p_logic = named::t_prefix<t_logic>::p_;

///////////////////////////////////////////////////////////////////////////////

  class t_impl_;
  using p_impl_ = named::t_prefix<t_impl_>::p_;

  class t_dispatcher : private t_logic {
  public:
    t_dispatcher(       R_params, p_logic);
    t_dispatcher(t_err, R_params, p_logic);
    t_dispatcher(t_dispatcher&&);
    t_dispatcher& operator=(t_dispatcher&&);
   ~t_dispatcher();

    t_dispatcher()                                = delete;
    t_dispatcher(const t_dispatcher&)             = delete;
    t_dispatcher& operator=(const t_dispatcher&)  = delete;

    t_params get_params() const;

    void show(const pevents&) const;
    void show() const;

    t_id         add_event(       t_fd, R_event_params, p_event_hook);
    t_id         add_event(t_err, t_fd, R_event_params, p_event_hook);
    p_event_hook del_event(       t_fd);
    p_event_hook del_event(t_err, t_fd);
    p_event_hook del_event(       R_id);
    p_event_hook del_event(t_err, R_id);

    t_void       clear_events();
    t_void       clear_events(t_err);

    t_n          get_events() const;
    t_event_info get_event (t_ix) const;

    t_void event_loop();
    t_void event_loop(t_err);

    t_void event_loop(       t_microseconds);
    t_void event_loop(t_err, t_microseconds);

  private:
    t_cmd process_events();

    virtual t_void order                 (pevents& unordered);
    virtual t_void notify_events_detected(pevents& ordered) const;
    virtual t_void notify_event_remove   (r_event_info);
    virtual t_cmd  notify_timeout        (t_microseconds);
    virtual t_cmd  notify_error          (t_errno);

    p_impl impl_;
  };

///////////////////////////////////////////////////////////////////////////////
}
}
}

#endif
