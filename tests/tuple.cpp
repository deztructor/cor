#include <cor/tuple.hpp>
#include <tut/tut.hpp>

namespace tut
{

struct tuple_test
{
    virtual ~tuple_test()
    {
    }
};

typedef test_group<tuple_test> tf;
typedef tf::object object;
tf cor_tuple_test("tuple");

enum test_ids {
    tid_access = 1,
    tid_output
};

enum class Field { First_ = 0, A = First_, B, Last_ = B};
typedef std::tuple<int, std::string> TestData;

struct TestRecord : public cor::TupleTraits<Field, TestData, TestRecord> {
    RECORD_FIELD_NAMES(TestRecord, "A", "B");
};

template<> template<>
void object::test<tid_access>()
{
    auto t = std::make_tuple(13, std::string("A&B"));
    ensure_eq("tuple 0", std::get<0>(t), 13);
    ensure_eq("tuple 1", std::get<1>(t), "A&B");
    ensure_eq("tuple nr id 0", TestRecord::get<Field::A>(t), std::get<0>(t));
    ensure_eq("tuple nr id 1", TestRecord::get<Field::B>(t), std::get<1>(t));
    ensure_eq("tuple nr id 0", cor::get<TestRecord, Field::A>(t), std::get<0>(t));
    ensure_eq("tuple nr id 1", cor::get<TestRecord, Field::B>(t), std::get<1>(t));
    TestRecord::set<Field::A>(t, 11);
    ensure_eq("tuple 0.1", std::get<0>(t), 11);
    ensure_eq("tuple 1.1", std::get<1>(t), "A&B");
    // std::tuple-like modification
    TestRecord::get<Field::A>(t) = 2;
    ensure_eq("tuple 0.2", std::get<0>(t), 2);
    ensure_eq("tuple 1.2", std::get<1>(t), "A&B");
    TestRecord::get<Field::B>(t) = "C&D";
    ensure_eq("tuple 0.3", std::get<0>(t), 2);
    ensure_eq("tuple 1.3", std::get<1>(t), "C&D");
}

template<> template<>
void object::test<tid_output>()
{
    std::stringstream ss;
    ss << cor::name<TestRecord, Field::A>() << "/"
       << cor::name<TestRecord, Field::B>() << "/";
    ss << cor::printable<TestRecord>(std::make_tuple(12, "E"));
    ensure_eq("output", ss.str(), "A/B/(A=12, B=E)");
}

}
