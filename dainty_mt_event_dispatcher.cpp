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

#include "dainty_container_list.h"
#include "dainty_mt_event_dispatcher.h"

namespace dainty
{
namespace mt
{
namespace event_dispatcher
{
  using named::t_n_;
  using named::P_cstr;
  using os::fdbased::t_epoll;
  using os::t_epoll_event;

  using r_err      = named::t_prefix<t_err>::r_;
  using t_freelist = container::freelist::t_freelist<t_event_info>;

///////////////////////////////////////////////////////////////////////////////

  class t_impl_ {
  public:
    virtual ~t_impl_() { };

    t_impl_(R_params _params) : params(_params), events{_params.max} {
      infos.reserve(get(_params.max));
    }

    const t_params params;
    t_freelist     events;
    t_event_infos  infos;

    inline
    t_bool fetch_events(r_ids ids) const {
      events.each([&ids](t_id id, const t_event_info&) mutable {
                  ids.push_back(id); });
      return !ids.empty();
    }

    inline
    P_event_info get_event(t_id id) const {
      return events.get(id);
    }

    inline
    P_event_info get_event (r_err err, t_id id) const {
      P_event_info info = events.get(id);
      if (info)
        return info;
      err = E_XXX;
      return nullptr;
    }

    t_void display() const {
      // access events
    }

///////////////////////////////////////////////////////////////////////////////

    virtual operator t_validity() const = 0;

    virtual t_id add_event(       t_fd, R_event_params, p_event_hook) = 0;
    virtual t_id add_event(r_err, t_fd, R_event_params, p_event_hook) = 0;

    virtual p_event_hook del_event(       t_id, p_logic) = 0;
    virtual p_event_hook del_event(r_err, t_id, p_logic) = 0;

    virtual t_void       clear_events(       p_logic) = 0;
    virtual t_void       clear_events(r_err, p_logic) = 0;

    virtual t_n   event_loop(       p_logic)                 = 0;
    virtual t_n   event_loop(r_err, p_logic)                 = 0;
    virtual t_n   event_loop(       p_logic, t_microseconds) = 0;
    virtual t_n   event_loop(r_err, p_logic, t_microseconds) = 0;

///////////////////////////////////////////////////////////////////////////////

    t_quit process_events(r_event_infos infos, p_logic logic) {
      if (!infos.empty()) {
         logic->may_reorder_events(infos);
        for (auto info : infos) {
          t_return ret = info->hook->notify_event(info->fd, info->params);
          switch (ret.cmd) {
            case CONTINUE: {
              t_event_hook* next = ret.hook;
              if (next)
                info->hook = next;
            } break;
            case REMOVE_EVENT:
              del_event(info->id, logic);
              break;
            case QUIT_EVENT_LOOP:
              return true;
          }
        }
      }
      return false;
    }
  };

///////////////////////////////////////////////////////////////////////////////

  class t_epoll_impl_ : public t_impl_ {
  public:
    t_epoll_impl_(R_params _params)
      : t_impl_{_params}, epoll_events_{new t_epoll_event[get(_params.max)]},
        epoll_{} {
    }

    t_epoll_impl_(r_err err, R_params _params)
      : t_impl_{_params}, epoll_events_{new t_epoll_event[get(_params.max)]},
        epoll_{err} {
    }

    ~t_epoll_impl_() {
      delete [] epoll_events_;
    }

    virtual operator t_validity() const override {
      return t_impl_::operator t_validity() == VALID &&
             epoll_events_ && epoll_ == VALID ? VALID : INVALID;
    }

    virtual t_id add_event(t_fd fd, R_event_params params,
                           p_event_hook hook) override {
      auto result = events.insert({fd, hook, params});
      if (result) {
        t_epoll::t_event_data data;
        data.u32 = get(result.id);
        result.ptr->id = result.id;
        if (epoll_.add_event(fd, params.type == RD
                                 ? EPOLLIN : EPOLLOUT, data) == VALID)
          return result.id;
        events.erase(result.id);
      }
      return t_id{0};
    }

    virtual t_id add_event(r_err err, t_fd fd, R_event_params params,
                           p_event_hook hook) override {
      auto result = events.insert(err, {fd, hook, params});
      if (result) {
        t_epoll::t_event_data data;
        data.u32 = get(result.id);
        result.ptr->id = result.id;
        if (epoll_.add_event(err, fd, params.type == RD
                                      ? EPOLLIN : EPOLLOUT, data) == VALID)
          return result.id;
        events.erase(result.id);
      }
      return t_id{0};
    }

    virtual p_event_hook del_event(t_id id, p_logic logic) override {
      auto info = events.get(id);
      if (info) {
        epoll_.del_event(info->fd);
        logic->notify_event_remove(*info);
        events.erase(id);
      }
      return nullptr;
    }

    virtual p_event_hook del_event(r_err err, t_id id,
                                   p_logic logic) override {
      auto info = events.get(err, id);
      if (info) {
        epoll_.del_event(err, info->fd);
        logic->notify_event_remove(*info);
        events.erase(id);
      }
      return nullptr;
    }

    virtual t_void clear_events(p_logic logic) override {
      events.each([this, logic](t_id, r_event_info& info) {
        epoll_.del_event(info.fd);
        logic->notify_event_remove(info);
      });
      events.clear();
    }

    virtual t_void clear_events(r_err err, p_logic logic) override {
      events.each([this, logic](t_id, r_event_info& info) { //XXX
        epoll_.del_event(info.fd);
        logic->notify_event_remove(info);
      });
      events.clear();
    }

    virtual t_n event_loop(p_logic logic) override {
      t_n_ cnt = 0;
      t_quit quit = false;
      do {
        t_n_ n = get(epoll_.wait(epoll_events_, params.max));
        if (n > 0) {
          for (t_n_ cnt = 0; cnt < n; ++cnt)
            infos.push_back(events.get(t_id{epoll_events_[cnt].data.u32}));
          quit = process_events(infos, logic);
        } else
          quit = logic->notify_error(6);
        infos.clear();
      } while (!quit);
      return t_n{cnt};
    }

    virtual t_n event_loop(r_err err, p_logic logic) override {
      return t_n{0};
    }

    virtual t_n event_loop(p_logic logic,
                           t_microseconds microseconds) override {
      t_n_ cnt = 0;
      t_quit quit = false;
      do {
        t_n_ n = get(epoll_.wait(epoll_events_, params.max));
        if (n > 0) {
          for (t_n_ cnt = 0; cnt < n; ++cnt)
            infos.push_back(events.get(t_id{epoll_events_[cnt].data.u32}));
          quit = process_events(infos, logic);
        } else if (!n) {
          quit = logic->notify_timeout(microseconds);
        } else
          quit = logic->notify_error(6);
        infos.clear();
        cnt++;
      } while (!quit);
      return t_n{cnt};
    }

    virtual t_n event_loop(r_err err, p_logic logic,
                           t_microseconds microseconds) override {
      return t_n{0};
    }

  private:
    t_epoll_event* epoll_events_;
    t_epoll        epoll_;
  };

///////////////////////////////////////////////////////////////////////////////

  t_n get_supported_services() {
     return t_n{1};
  }

  t_service_name get_supported_service(t_ix) {
    return "epoll_service";
  }

///////////////////////////////////////////////////////////////////////////////

  t_dispatcher::t_dispatcher(R_params params) {
    if (params.service_name == P_cstr("epoll_service"))
      impl_ = new t_epoll_impl_(params);
    else if (params.service_name == P_cstr("select_service")) {
    }
  }

  t_dispatcher::t_dispatcher(t_err err, R_params params) {
    T_ERR_GUARD(err) {
      if (params.service_name == P_cstr("epoll_service"))
        impl_ = new t_epoll_impl_(err, params);
      else if (params.service_name == P_cstr("select_service")) {
      } else
        err = E_XXX;
    }
  }

  t_dispatcher::t_dispatcher(t_dispatcher&& dispatcher)
    : impl_{named::reset(dispatcher.impl_)} {
  }

  t_dispatcher::~t_dispatcher() {
    if (impl_) {
      // what about open fds and hooks?
      delete impl_;
    }
  }

  t_dispatcher::operator t_validity() const {
    return impl_ ? *impl_ : INVALID;
  }

  t_params t_dispatcher::get_params() const {
    if (*this == VALID)
      return impl_->params;
    return t_params{t_n{0}, ""};
  }

  t_void t_dispatcher::display() const {
    if (*this == VALID)
      impl_->display();
  }

  t_id t_dispatcher::add_event(t_fd fd, R_event_params params,
                               p_event_hook hook) {
    if (*this == VALID)
      return impl_->add_event(fd, params, hook);
    return t_id{0};
  }

  t_id t_dispatcher::add_event(t_err err, t_fd fd, R_event_params params,
                               p_event_hook hook) {
    T_ERR_GUARD(err) {
      if (*this == VALID)
        return impl_->add_event(err, fd, params, hook);
      err = E_XXX;
    }
    return t_id{0};
  }

  p_event_hook t_dispatcher::del_event(t_id id) {
    if (*this == VALID)
      return impl_->del_event(id, this);
    return nullptr;
  }

  p_event_hook t_dispatcher::del_event(t_err err, t_id id) {
    T_ERR_GUARD(err) {
      if (*this == VALID)
        return impl_->del_event(err, id, this);
      err = E_XXX;
    }
    return nullptr;
  }

  t_void t_dispatcher::clear_events() {
    if (*this == VALID)
      impl_->clear_events(this);
  }

  t_void t_dispatcher::clear_events(t_err err) {
    T_ERR_GUARD(err) {
      if (*this == VALID)
        impl_->clear_events(err, this);
      else
        err = E_XXX;
    }
  }

  P_event_info t_dispatcher::get_event(t_id id) const {
    if (*this == VALID)
      return impl_->get_event(id);
    return nullptr;
  }

  P_event_info t_dispatcher::get_event(t_err err, t_id id) const {
    T_ERR_GUARD(err) {
      if (*this == VALID)
        return impl_->get_event(err, id);
      err = E_XXX;
    }
    return nullptr;
  }

  t_bool t_dispatcher::fetch_events(r_ids ids) const {
    if (*this == VALID)
      return impl_->fetch_events(ids);
    return false;
  }

  t_n t_dispatcher::event_loop() {
    if (*this == VALID)
      return impl_->event_loop(this);
    return t_n{0};
  }

  t_n t_dispatcher::event_loop(t_err err) {
    T_ERR_GUARD(err) {
      if (*this == VALID)
        return impl_->event_loop(err, this);
      err = E_XXX;
    }
    return t_n{0};
  }

  t_n t_dispatcher::event_loop(t_microseconds microseconds) {
    if (*this == VALID)
      return impl_->event_loop(this, microseconds);
    return t_n{0};
  }

  t_n t_dispatcher::event_loop(t_err err, t_microseconds microseconds) {
    T_ERR_GUARD(err) {
      if (*this == VALID)
        return impl_->event_loop(err, this, microseconds);
      err = E_XXX;
    }
    return t_n{0};
  }

  t_void t_dispatcher::may_reorder_events(r_event_infos infos) {
  }

  t_void t_dispatcher::notify_event_remove(r_event_info info) {
  }

  t_quit t_dispatcher::notify_timeout(t_microseconds) {
    return true;
  }

  t_quit t_dispatcher::notify_error(t_errno) {
    return true;
  }

///////////////////////////////////////////////////////////////////////////////
}
}
}
