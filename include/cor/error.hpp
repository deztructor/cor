#ifndef _COR_ERROR_HPP_
#define _COR_ERROR_HPP_

/*
 * Exceptions and error processing
 *
 * Copyright (C) 2012 Jolla Ltd.
 * Contact: Denis Zalevskiy <denis.zalevskiy@jollamobile.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <sys/mman.h>

namespace cor
{

template <typename T>
struct ptr_traits
{
    typedef std::unique_ptr<T, void (*)(T*)> unique_ptr;
};

template <typename T>
struct c_ptr_traits
{
    typedef std::unique_ptr<T, void (*)(void*)> unique_ptr;
};

template <typename T>
typename c_ptr_traits<T>::unique_ptr mk_cmem_handle(T *from)
{
    return typename c_ptr_traits<T>::unique_ptr(from, &std::free);
}

template <typename T>
typename c_ptr_traits<T>::unique_ptr mk_cmem_handle()
{
    return mk_cmem_handle<T>(nullptr);
}

bool is_address_valid(void *);


struct Backtrace_ {
    typedef typename c_ptr_traits<char const*>::unique_ptr symbols_ptr;
    typedef std::pair<symbols_ptr, size_t> symbols_type;
    static symbols_type get_symbols(void *const *, size_t);
    static std::string name(size_t index, char const* existing_name, void *const * frame);
};

template <size_t Frames>
class Backtrace : private Backtrace_
{
public:
    Backtrace()
        : count_(::backtrace(&frames[0], frames.size()))
        , symbols(mk_cmem_handle<char const*>())
    { }

    Backtrace(Backtrace const &src)
        : count_(0)
        , symbols(mk_cmem_handle<char const*>())
    {
        count_ = src.count_;
        std::copy(&src.frames[0], &src.frames[count_], frames.begin());
        if (src.symbols) {
            symbols.reset((const char**)calloc(src.count_, sizeof(*symbols)));
            auto to = src.symbols.get(), from = symbols.get();
            std::copy(from, from + count_, to);
        }
    }

    ~Backtrace() noexcept {}

    const char **begin() const
    {
        update_symbols();
        return symbols.get();
    }

    const char **end() const
    {
        update_symbols();
        return symbols.get() + count_;
    }

    char const* at(size_t index) const
    {
        update_symbols();
        return (index < count_ && symbols) ? symbols.get()[index] : "???";
    }

    bool dladdr(size_t index, Dl_info &info) const
    {
        return (index < count_) && (::dladdr(frames[index], &info) != 0);
    }

    std::string name(size_t index) const
    {
        return (index < count_)
            ? Backtrace_::name(index, at(index), &frames[index])
            : "?";
    }

    size_t size() const
    {
        return count_;
    }

private:

    void update_symbols() const
    {
        if (!symbols)
            std::tie(symbols, count_) = Backtrace_::get_symbols(&frames[0], count_);
    }

    std::array<void*, Frames> frames;
    mutable size_t count_;

    mutable Backtrace_::symbols_ptr symbols;
};

template <typename T, size_t N>
std::basic_ostream<T>& operator <<
(std::basic_ostream<T> &dst, Backtrace<N> const &src)
{
    for (size_t i = 0; i < src.size(); ++i)
        dst << i << ": " << src.name(i) << std::endl;
    return dst;
}

static inline std::string mk_error_message(std::string const &info)
{
    return info;
}

template <typename ... Args>
std::string mk_error_message(std::string const &info, Args &&... args)
{
    auto len = info.size();
    if (!len)
        return info;

    std::string res(len * 2, 0);
    int rc;
    while (res.size() < 1024 * 64) {
        rc = snprintf(&res[0], res.size(), info.c_str(), args...);
        if (rc >= 0 && (unsigned)rc < res.size()) {
            res.resize(rc);
            break;
        }
        res.resize(res.size() * 2);
    }
    return res;
}

class Error : public std::runtime_error
{
public:
    template <typename ... Args>
    Error(std::string const &info, Args &&... args)
        : std::runtime_error
          (mk_error_message(info, std::forward<Args>(args)...))
    {
    }

    virtual void print_trace() const
    {
        std::cerr << trace;
    }

    Backtrace<30> trace;
};

extern template class Backtrace<30>;

class CError : public Error
{
public:
    CError(long rc, std::string const &info)
        : Error(info), rc(rc) {}
    long rc;

    virtual void print_trace() const
    {
        fprintf(stderr, "rc: %lx\n", rc);
        Error::print_trace();
    }
};

template <typename FnT, typename ... Args>
void error_trace_msg(std::string const &msg, FnT const &fn, Args ...args)
{
    try {
        fn(std::forward<Args>(args)...);
    } catch(Error const &e) {
        std::cerr << msg << "cor::Error: " << e.what() << std::endl;
        e.print_trace();
        throw e;
    } catch(std::exception const &e) {
        std::cerr << msg << "std::exception: " << e.what() << std::endl;
        throw e;
    }
}

template <typename FnT, typename ... Args>
void error_trace_msg_nothrow(std::string const &msg, FnT const &fn, Args ...args)
{
    try {
        fn(std::forward<Args>(args)...);
    } catch(Error const &e) {
        std::cerr << msg << "cor::Error: " << e.what() << std::endl;
        e.print_trace();
    } catch(std::exception const &e) {
        std::cerr << msg << "std::exception: " << e.what() << std::endl;
    }
}

template <typename FnT, typename ... Args>
void error_tracer(FnT const &fn, Args ...args)
{
    error_trace_msg("", fn, std::forward<Args>(args)...);
}

template <typename FnT, typename ... Args>
void error_trace_nothrow(FnT const &fn, Args ...args)
{
    error_trace_msg_nothrow("", fn, std::forward<Args>(args)...);
}



} // namespace cor

#endif // _COR_ERROR_HPP_
