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

#include "dainty_os_threading.h"
#include "dainty_mt_notify_change.h"

namespace dainty
{
namespace mt
{
namespace notify_change
{
  using namespace dainty::os::threading;
  using namespace dainty::os::fdbased;

///////////////////////////////////////////////////////////////////////////////

  class t_impl_ {
  public:
    using t_logic = t_processor::t_logic;

    t_impl_(t_err err) noexcept
      : lock_(err), eventfd_(err, t_n{0}) {
    }

    operator t_validity() const noexcept {
      return (lock_ == VALID && eventfd_ == VALID) ?  VALID : INVALID;
    }

    t_validity process(t_err err, t_logic& logic, t_n max) noexcept {
      T_ERR_GUARD(err) {
        for (t_n_ n = get(max); !err && n; --n) {
          t_eventfd::t_value value = 0;
          if (eventfd_.read(err, value) == VALID) {
            t_any  any;
            t_user user;
            t_bool changed = false;
            <% auto scope = lock_.make_locked_scope(err);
              if (scope == VALID) {
                changed = changed_;
                if (changed) {
                  user = user_;
                  any  = std::move(any_);
                  changed_ = false;
                }
              }
            %>
            if (!err && changed)
              logic.process(user, std::move(any));
          }
        }
      }
      return !err ? VALID : INVALID;
    }

    t_validity post(t_err& err, t_user user, t_any&& any) noexcept {
      T_ERR_GUARD(err) {
        t_bool changed = false;
        <% auto scope = lock_.make_locked_scope(err);
          if (scope == VALID) {
            if (any != any_) {
              user_    = user;
              any_     = std::move(any);
              changed_ = true;
              changed  = true;
            }
          }
        %>
        if (changed) {
          t_eventfd::t_value value = 1;
          eventfd_.write(err, value);
        }
      }
      return !err ? VALID : INVALID;
    }

    t_fd get_fd() const noexcept {
      return eventfd_.get_fd();
    }

    t_client make_client(t_user user) noexcept {
      // NOTE: future, we have information on clients.
      return {this, user};
    }

  private:
    t_mutex_lock lock_;
    t_eventfd    eventfd_;
    t_any        any_;
    t_user       user_    = t_user{0L};
    t_bool       changed_ = false;
  };

///////////////////////////////////////////////////////////////////////////////

  t_validity t_client::post(t_err err, t_any&& any) noexcept {
    T_ERR_GUARD(err) {
      if (impl_)
        return impl_->post(err, user_, std::move(any));
      err = E_XXX;
    }
    return INVALID;
  }

///////////////////////////////////////////////////////////////////////////////

  t_processor::t_processor(t_err err) noexcept {
    T_ERR_GUARD(err) {
      impl_ = new t_impl_(err);
      if (impl_) {
        if (err) {
          delete impl_;
          impl_ = nullptr;
        }
      } else
        err = E_XXX;
    }
  }

  t_processor::~t_processor() {
    if (impl_) { // NOTE: can check to see if clients exists
      delete impl_;
    }
  }

  t_client t_processor::make_client(t_user user) noexcept {
    if (impl_)
      return impl_->make_client(user);
    return {};
  }

  t_validity t_processor::process(t_err err, t_logic& logic,
                                  t_n max) noexcept {
    T_ERR_GUARD(err) {
      if (impl_)
        return impl_->process(err, logic, max);
      err = E_XXX;
    }
    return INVALID;
  }

  inline
  t_fd t_processor::get_fd() const noexcept {
    if (impl_)
      return impl_->get_fd();
    return BAD_FD;
  }

///////////////////////////////////////////////////////////////////////////////
}
}
}