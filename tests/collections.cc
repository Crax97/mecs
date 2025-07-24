#include "collections.h"
#include "mecs/base.h"
#include "private.h"
#include "test_private.hpp"

#include <cstdlib>

TEST_CASE("Bitset")
{
    constexpr MecsSize kNumRandomSlots = 512;
    constexpr MecsSize kMaxBits = 4096;
    MecsAllocator alloc = kDebugAllocator;

    BitSet bitset;
    BitSet bitset2;
    BitSet empty;
    MecsVec<MecsSize> slots;

    REQUIRE(bitset == empty);

    for (MecsSize i = 0; i < kMaxBits; i++) {
        REQUIRE(!bitset.test(i));
    }

    for (MecsSize i = 0; i < kNumRandomSlots; i++) {
        MecsSize slot = static_cast<MecsSize>(rand()) % kMaxBits;
        slots.pushUnique(alloc, slot);
        bitset.set(alloc, slot, true);
        bitset2.set(alloc, slot, true);
    }

    REQUIRE(bitset == bitset2);
    REQUIRE(bitset != empty);

    bitset2.set(alloc, slots[0], false);
    REQUIRE(bitset != bitset2);

    slots.forEach([&](MecsSize slot) {
        REQUIRE(bitset.test(slot));
        bitset.set(alloc, slot, false);
    });

    slots.forEach([&](MecsSize slot) {
        REQUIRE(!bitset.test(slot));
    });

    REQUIRE(bitset == empty); // Even though bitset has allocated some bytes, while empty is 'truly' empty, their bytes are all 0

    slots.destroy(alloc);
    bitset.destroy(alloc);
    bitset2.destroy(alloc);
    empty.destroy(alloc);
    empty.destroy(alloc);
}