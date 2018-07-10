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

#ifndef _DAINTY_MT_EVENT_H_
#define _DAINTY_MT_EVENT_H_

#include "dainty_named.h"
#include "dainty_oops.h"
#include "dainty_os_fdbased.h"
#include "dainty_mt_err.h"

namespace dainty
{
namespace mt
{
namespace event
{
  using os::fdbased::t_fd;
  using named::t_n;
  using named::t_void;
  using named::p_cstr;
  using named::t_validity;
  using named::VALID;
  using named::INVALID;

///////////////////////////////////////////////////////////////////////////////

  enum  t_cnt_tag_ {};
  using t_cnt_ = named::t_uint64;
  using t_cnt  = named::t_explicit<t_cnt_, t_cnt_tag_>;

  class t_processor_impl_;

  class t_client {
  public:
    t_client(t_client&& client) noexcept;

    t_client& operator=(const t_client&) = delete;
    t_client& operator=(t_client&&)      = delete;

    operator t_validity() const noexcept;

    t_validity post(t_err, t_cnt = t_cnt{1}) noexcept;

  private:
    friend class t_processor;
    friend class t_processor_impl_;
    t_client() = default;
    t_client(t_processor_impl_* impl) noexcept : impl_(impl) { }

    t_processor_impl_* impl_ = nullptr;
  };

///////////////////////////////////////////////////////////////////////////////

  class t_processor {
  public:
    class t_logic {
    public:
      using t_err = oops::t_oops<>;
      using t_cnt = event::t_cnt;

      virtual ~t_logic() { }
      virtual t_void async_process(t_cnt) noexcept = 0;
    };

    using r_logic = t_logic&;

     t_processor(t_err)         noexcept;
     t_processor(t_processor&&) noexcept;
    ~t_processor();

    t_processor(const t_processor&)            = delete;
    t_processor& operator=(t_processor&&)      = delete;
    t_processor& operator=(const t_processor&) = delete;

    operator t_validity () const noexcept;

    t_fd get_fd() const noexcept;

    t_validity process(t_err, r_logic, t_n max = t_n{1}) noexcept;

    t_client make_client() noexcept; // const char* - literal - XXX

  private:
    t_processor_impl_* impl_ = nullptr;
  };

///////////////////////////////////////////////////////////////////////////////

  inline
  t_client::t_client(t_client&& client) noexcept : impl_(client.impl_) {
    client.impl_ = nullptr;
  }

  inline
  t_client::operator t_validity() const noexcept {
    return impl_ ? VALID : INVALID;
  }

///////////////////////////////////////////////////////////////////////////////

  inline
  t_processor::t_processor(t_processor&& processor) noexcept
      : impl_(processor.impl_) {
    processor.impl_ = nullptr;
  }

  inline
  t_processor::operator t_validity() const noexcept {
    return impl_ ? VALID : INVALID;
  }

///////////////////////////////////////////////////////////////////////////////
}
}
}

#endif