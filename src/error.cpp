#include <cor/error.hpp>

namespace cor {

bool is_address_valid(void *p)
{
    bool res = false;
    int fd[2];
    if (pipe(fd) >= 0) {
        if (write(fd[1], p, 128) > 0)
            res = true;

        close(fd[0]);
        close(fd[1]);
    }
    return res;
}

template class Backtrace<30>;

Backtrace_::symbols_type
Backtrace_::get_symbols(void *const *frames, size_t count)
{
    auto symbols = mk_cmem_handle<char const*>
        (const_cast<char const**>(backtrace_symbols(frames, count)));

    if (!symbols) {
        symbols.reset(static_cast<char const**>(::calloc(1, sizeof(*symbols))));
        *symbols = "?";
        count = 1;
    }
    return std::make_pair(std::move(symbols), count);
}

std::string Backtrace_::name
(size_t, char const* existing_name, void *const * frame)
{
    Dl_info info;
    int status = -1;
    auto p = mk_cmem_handle<char>();
    if (dladdr(frame, &info) && info.dli_sname)
        p.reset(abi::__cxa_demangle(info.dli_sname, NULL, 0, &status));

    return p ? std::string(p.get()) : std::string(existing_name);
}


}
