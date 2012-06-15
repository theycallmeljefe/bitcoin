#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

#include "serialize.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(serialize_tests)

BOOST_AUTO_TEST_CASE(varints)
{
    // encode

    CDataStream ss(SER_DISK, 0);
    for (int i = 0; i < 100000; i++) {
        ss << VARINT(i);
    }

    for (uint64 i = 0;  i < 100000000000ULL; i += 999999937) {
        ss << VARINT(i);
    }

    // decode
    for (int i = 0; i < 100000; i++) {
        int j;
        ss >> VARINT(j);
        assert(i == j);
    }

    for (uint64 i = 0;  i < 100000000000ULL; i += 999999937) {
        uint64 j;
        ss >> VARINT(j);
        assert(i == j);
    }

}

BOOST_AUTO_TEST_SUITE_END()
