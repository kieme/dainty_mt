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
    t_valid join();
    t_valid join(t_err);


 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

******************************************************************************/

#include "dainty_named.h"
#include "dainty_oops.h"
#include "dainty_mt_threads.h"

namespace dainty
{
namespace mt
{
namespace threads
{
  using namespace dainty::os;
  using namespace dainty::os::threading;

///////////////////////////////////////////////////////////////////////////////

  namespace
  {
    struct t_threaddata {
      t_err        err_;
      p_cstr       name_;
      p_void       logic_;
      t_bool       del_logic_;
      t_bool       ready_;
      t_mutex_lock lock_;
      t_cond_var   cond_;

      t_threaddata(t_err err, p_cstr name, p_void logic,
                   t_bool del_logic) noexcept
        : err_(err), name_(name), logic_(logic), del_logic_(del_logic),
          ready_(false) {
      }
    };

///////////////////////////////////////////////////////////////////////////////

    p_void thread_start_(p_void arg) {
      using p_logic = t_thread::t_logic*;
      t_threaddata* data = reinterpret_cast<t_threaddata*>(arg);

      t_err&  err       = data->err_;
      p_logic logic     = reinterpret_cast<p_logic>(data->logic_);
      t_bool  del_logic = data->del_logic_;

      t_pthread::set_name(err, t_pthread::get_self(), data->name_);
      logic->prepare(err);
      <% auto scope = data->lock_.make_locked_scope(err);
        data->ready_ = logic->prepare(err) == VALID;
      %>

      p_void returnarg = nullptr;
      if (!err) {
        data->cond_.signal(err);
        returnarg = logic->run();
      } else
        data->cond_.signal();

      /* note: signal will invalidate data */

      if (del_logic)
        delete logic;

      return returnarg;
    }

///////////////////////////////////////////////////////////////////////////////

    p_void detachedthread_start_(p_void arg) {
      using p_logic = t_detachedthread::t_logic*;
      t_threaddata* data = reinterpret_cast<t_threaddata*>(arg);

      t_err&  err       = data->err_;
      p_logic logic     = reinterpret_cast<p_logic>(data->logic_);
      t_bool  del_logic = data->del_logic_;

      t_pthread::set_name(err, t_pthread::get_self(), data->name_);
      logic->prepare(err);
      <% auto scope = data->lock_.make_locked_scope(err);
        data->ready_ = logic->prepare(err) == VALID;
      %>

      if (!err) {
        data->cond_.signal(err);
        logic->run();
      } else
        data->cond_.signal();

      /* note: signal will invalidate data */

      if (del_logic)
        delete logic;

      return nullptr;
    }
  }

///////////////////////////////////////////////////////////////////////////////

  t_validity t_thread::t_logic::update(t_err err,
                                       ::pthread_attr_t& attr) noexcept {
    T_ERR_GUARD(err) {
      if ((!::pthread_attr_setstacksize   (&attr, (128*1024)))             &&
          (!::pthread_attr_setguardsize   (&attr, (4 * 1024)))             &&
          (!::pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) &&
          (!::pthread_attr_setschedpolicy (&attr, SCHED_OTHER)))
        return VALID;
      else
        err = E_XXX;
    }
    return INVALID;
  }

  t_validity t_thread::t_logic::prepare(t_err err) noexcept {
    T_ERR_GUARD(err) {
      return VALID;
    }
    return INVALID;
  }

  t_thread::t_thread(t_err   err,
                     p_cstr  name,
                     p_logic logic,
                     t_bool  del_logic) noexcept {
    T_ERR_GUARD(err) {
      t_threaddata data{err, name, logic, del_logic};
      if (get(name) && logic && data.cond_ == VALID && data.lock_ == VALID) {
        pthread_attr_t attr;
        if ((!::pthread_attr_init(&attr)) &&
            (logic->update (err, attr)                         == VALID) &&
            /* must check that it is not made detach */
            (thread_.create(err, thread_start_, &data) == VALID)) {
          <% auto scope = data.lock_.make_locked_scope(err);
             while (!err && !data.ready_)
               data.cond_.wait(err, data.lock_);
          %>
        }
      }
    }
  }

  t_thread::operator t_validity() const noexcept {
    return thread_;
  }

  t_validity t_thread::join() noexcept {
    return thread_.join() == 0 ? VALID : INVALID;
  }

  t_validity t_thread::join(t_err err) noexcept {
    return thread_.join(err);
  }

  t_validity t_thread::join(p_void& arg) noexcept {
    return thread_.join(arg) == 0 ? VALID : INVALID;
  }

  t_validity t_thread::join(t_err err, p_void& arg) noexcept {
    return thread_.join(err);
  }

///////////////////////////////////////////////////////////////////////////////

  t_validity
      t_detachedthread::t_logic::update(t_err err,
                                        ::pthread_attr_t& attr) noexcept {
    T_ERR_GUARD(err) {
      if ((!::pthread_attr_setstacksize   (&attr, (128*1024)))             &&
          (!::pthread_attr_setguardsize   (&attr, (4 * 1024)))             &&
          (!::pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) &&
          (!::pthread_attr_setschedpolicy (&attr, SCHED_OTHER)))
        return VALID;
      else
        err = E_XXX;
    }
    return INVALID;
  }

  t_validity t_detachedthread::t_logic::prepare(t_err err) noexcept {
    T_ERR_GUARD(err) {
      return VALID;
    }
    return INVALID;
  }

  t_detachedthread::t_detachedthread(t_err   err,
                                     p_cstr  name,
                                     p_logic logic,
                                     t_bool  del_logic) noexcept {
    T_ERR_GUARD(err) {
      t_threaddata data{err, name, logic, del_logic};
      if (get(name) && logic && data.cond_ == VALID && data.lock_ == VALID) {
        pthread_attr_t attr;
        if ((!::pthread_attr_init(&attr)) &&
            (!::pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) &&
            (logic->update (err, attr)                         == VALID) &&
            /* must check that it is not made joinable */
            (thread_.create(err, detachedthread_start_, &data) == VALID)) {
          <% auto scope = data.lock_.make_locked_scope(err);
             while (!err && !data.ready_)
               data.cond_.wait(err, data.lock_);
          %>
        }
      }
    }
  }

///////////////////////////////////////////////////////////////////////////////
}
}
}
