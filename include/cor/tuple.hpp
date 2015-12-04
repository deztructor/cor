#ifndef _COR_TUPLE_HPP_
#define _COR_TUPLE_HPP_

#include <cor/util.hpp>
#include <tuple>

namespace cor
{

template <typename I, typename V, typename InfoT>
struct TupleTraits
{
    typedef I id_type;
    typedef V value_type;
    typedef V & ref;
    typedef V const & cref;
    typedef InfoT info_type;

    static constexpr size_t last = cor::enum_index(id_type::Last_);
    static constexpr size_t size = last + 1;
    static_assert(size == std::tuple_size<value_type>::value
                  , "IndexT should end with Last_ == last element Id");

    template <id_type Id>
    struct Index {
        static constexpr size_t value = cor::enum_index(Id);
        typedef typename std::tuple_element<cor::enum_index(Id), value_type> element_type;
        typedef typename element_type::type type;
        typedef type & ref;
        typedef type const & cref;
    };

    template <size_t Index>
    struct Enum {
        static constexpr id_type value = static_cast<id_type>(Index);
        static_assert(value <= id_type::Last_, "Should be <= Last_");
    };

    template <id_type Id>
    static void set(ref from, typename Index<Id>::cref v)
    {
        std::get<Index<Id>::value>(from) = v;
    }

    template <id_type Id>
    static typename Index<Id>::ref get(ref from) {
        return std::get<Index<Id>::value>(from);
    }
    
    template <id_type Id>
    static typename Index<Id>::cref get(cref from) {
        return std::get<Index<Id>::value>(from);
    }

};

template <typename TupleTraitsT, typename TupleTraitsT::id_type Id>
typename TupleTraitsT::template Index<Id>::cref
get(typename TupleTraitsT::cref v)
{
    return TupleTraitsT::template get<Id>(v);
}

template <typename TupleTraitsT, typename TupleTraitsT::id_type Id>
typename TupleTraitsT::template Index<Id>::ref
get(typename TupleTraitsT::ref v)
{
    return TupleTraitsT::template get<Id>(v);
}

template <typename TupleTraitsT>
struct RecordCRef
{
    typedef typename TupleTraitsT::cref cref;
    RecordCRef(cref data) : data_(data) {}
    RecordCRef(RecordCRef const &) = delete;
    RecordCRef(RecordCRef &&v) : data_(v.data_) {}
    cref data_;
};

template <typename TupleTraitsT>
struct PrintableRecord : public RecordCRef<TupleTraitsT>
{
    PrintableRecord(typename TupleTraitsT::cref data)
        : RecordCRef<TupleTraitsT>(data) {}
};

template <typename TupleTraitsT, typename TupleTraitsT::id_type Id>
typename TupleTraitsT::template Index<Id>::cref
value(PrintableRecord<TupleTraitsT> const &v)
{
    return get<TupleTraitsT, Id>(v.data_);
}

template <typename TupleTraitsT, typename TupleTraitsT::id_type Id>
constexpr char const *name()
{
    return TupleTraitsT::info_type::template name<Id>();
}

template <typename TupleTraitsT, typename TupleTraitsT::id_type Id>
constexpr char const *name(PrintableRecord<TupleTraitsT> const &)
{
    return name<TupleTraitsT, Id>();
}

template <typename TupleTraitsT>
PrintableRecord<TupleTraitsT> printable(typename TupleTraitsT::cref v)
{
    return PrintableRecord<TupleTraitsT>(v);
}

template <size_t N>
struct RecordPrintablePair
{
    template <typename StreamT, typename TupleTraitsT>
    static void out(StreamT &d, PrintableRecord<TupleTraitsT> const & v)
    {
        static constexpr auto idx = TupleTraitsT::last - N;
        static constexpr auto id = TupleTraitsT::template Enum<idx>::value;
        d << name<TupleTraitsT, id>(v) << "="
          << value<TupleTraitsT, id>(v) << ", ";
        RecordPrintablePair<N - 1>::out(d, v);
    }
};

template <>
struct RecordPrintablePair<0>
{
    template <typename TupleTraitsT, typename StreamT>
    static void out(StreamT &d, PrintableRecord<TupleTraitsT> const & v)
    {
        static constexpr auto id = TupleTraitsT::template Enum<TupleTraitsT::last>::value;
        d << name<TupleTraitsT, id>(v) << "=" << value<TupleTraitsT, id>(v);
    }
};

template <typename T, typename TupleTraitsT>
std::basic_ostream<T>& operator <<
(std::basic_ostream<T> &dst, PrintableRecord<TupleTraitsT> const &v)
{
    static constexpr auto index = TupleTraitsT::last;
    dst << "("; RecordPrintablePair<index>::out(dst, v); dst << ")";
    return dst;
}

// ----------------------------------------------------------------------------

template < template <typename, typename> class AccessorT
           , typename StorageT, typename ... Args>
struct OutOp;

template <template <typename, typename> class AccessorT
          , typename StorageT
          , typename A0, typename A1
          , typename ... Tail>
struct OutOp<AccessorT, StorageT, A0, A1, Tail...>
{
    static std::tuple<A0, A1, Tail...> get(StorageT && tgt)
    {
        typedef OutOp<AccessorT, StorageT, A0> head_op;
        typedef OutOp<AccessorT, StorageT, A1, Tail...> tail_op;
        auto t0 = head_op::get(std::forward<StorageT>(tgt));
        auto t1 = tail_op::get(std::forward<StorageT>(tgt));
        return std::tuple_cat(std::move(t0), std::move(t1));
    }
};

template <template <typename, typename> class AccessorT
          , typename StorageT
          , typename A0>
struct OutOp<AccessorT, StorageT, A0>
{
    static std::tuple<A0> get(StorageT && tgt)
    {
        return std::make_tuple<A0>(AccessorT<StorageT, A0>::get
                                   (std::forward<StorageT>(tgt)));
    }
};

template < template <typename, typename> class AccessorT
           , typename StorageT
           , typename ... Args>
std::tuple<Args...> get_similar(StorageT && tgt, std::tuple<Args...>)
{
    typedef OutOp<AccessorT, StorageT, Args...> op_type;
    return op_type::get(std::forward<StorageT>(tgt));
}


template <template <typename, typename> class AccessorT
          , typename StorageT
          , typename ... Args>
struct OutOp<AccessorT, StorageT, std::tuple<Args...> >
{
    typedef std::tuple<Args...> data_type;
    static std::tuple<data_type> get(StorageT && tgt)
    {
        return std::make_tuple
            (get_similar<AccessorT>(std::forward<StorageT>(tgt)
                                    , data_type()));
    }
};

template <template <typename, typename> class AccessorT
          , typename StorageT
          , typename ... Args>
struct InOp;

template <template <typename, typename> class AccessorT
          , typename StorageT
          , typename ... Args>
struct InOp
{
    template <typename V>
    static void set_to(StorageT && tgt, V const &v)
    {
        AccessorT<StorageT, V>::set(std::forward<StorageT>(tgt), v);
    }

    template <typename ... Args2>
    static void set_to(StorageT && tgt, std::tuple<Args2...> const &v)
    {
        typedef InOp<AccessorT, StorageT, Args2...> op_type;
        op_type::set(std::forward<StorageT>(tgt), v);
    }

    static void set(StorageT && tgt, std::tuple<Args...> const &src)
    {
        set_(std::forward<StorageT>(tgt), src, cor::selector(src));
    }

private:

    template <size_t N>
    static void set_(StorageT && tgt, std::tuple<Args...> const &src
                     , cor::TupleSelector<N, N> const &selector)
    {
        set_to(std::forward<StorageT>(tgt), selector.get(src));
    }

    template <size_t N, size_t P>
    static void set_(StorageT && tgt, std::tuple<Args...> const &src
                     , cor::TupleSelector<N, P> const &selector)
    {
        set_to(std::forward<StorageT>(tgt), selector.get(src));
        set_(std::forward<StorageT>(tgt), src, selector.next());
    }
};

template < template <typename, typename> class AccessorT
           , typename TupleT
           , typename StorageT>
TupleT get(StorageT && tgt)
{
    return get_similar<AccessorT, StorageT>
        (std::forward<StorageT>(tgt), TupleT());
}

template <template <typename, typename> class AccessorT
          , typename StorageT, typename ... Args>
void set(StorageT && tgt, std::tuple<Args...> const &src)
{
    typedef InOp<AccessorT, StorageT, Args...> op_type;
    op_type::set(std::forward<StorageT>(tgt), src);
}


}

#endif // _COR_TUPLE_HPP_
