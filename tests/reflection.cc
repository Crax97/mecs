
#include "catch_amalgamated.hpp"
#include "mecs/base.h"
#include "mecs/mecs.h"

#include "mecshpp/mecs.hpp"

#include "mecshpp/mecsrtti.hpp"
#include "test_private.hpp"
#include <cstddef>
struct Foo {
    int foo;
    float bar;
};

struct Bar {
    Bar() = default;
    Bar(std::string inName)
        : name(inName)
    {
    }

private:
    MECS_RTTI_FRIEND(Bar)
    std::string name;
};

MECS_RTTI_STRUCT_BEGIN(Foo)
MECS_RTTI_STRUCT_MEMBER(foo)
MECS_RTTI_STRUCT_MEMBER(bar)
MECS_RTTI_STRUCT_END()

MECS_RTTI_STRUCT_BEGIN(Bar)
MECS_RTTI_STRUCT_MEMBER(name)
MECS_RTTI_STRUCT_END()

TEST_CASE("C++ reflection sample")
{

    MecsRegistryCreateInfo regInfo {};
    regInfo.memAllocator = kDebugAllocator;
    mecs::Registry registry(regInfo);
    registry.addRegistration<Foo>();
    registry.addRegistration<Bar>();

    mecs::World world(registry);

    mecs::EntityID entity = world.spawnEntity()
                                .withComponent<Foo>(42, 3.14F)
                                .withComponent<Bar>("DuffyDuck");

    constexpr auto kMembersFoo = mecs::rttiOf<Foo>::kMembers;
    constexpr auto kMembersBar = mecs::rttiOf<Bar>::kMembers;

    static_assert(kMembersFoo.size() == 2);
    static_assert(kMembersBar.size() == 1);
    REQUIRE(std::string(kMembersFoo[0].name) == "foo");
    REQUIRE(std::string(kMembersFoo[1].name) == "bar");
    REQUIRE(std::string(kMembersBar[0].name) == "name");
    static_assert(kMembersBar[0].typeID == mecs::typeIdOf<std::string>());

    void* foo = &world.entityGetComponent<Foo>(entity);
    void* bar = &world.entityGetComponent<Bar>(entity);

    REQUIRE(*reinterpret_cast<int*>(kMembersFoo[0].getMemberFn(foo)) == 42);
    REQUIRE(*reinterpret_cast<float*>(kMembersFoo[1].getMemberFn(foo)) == 3.14F);
    REQUIRE(*reinterpret_cast<std::string*>(kMembersBar[0].getMemberFn(bar)) == "DuffyDuck");
}
// NOLINTEND