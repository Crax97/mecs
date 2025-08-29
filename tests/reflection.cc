
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

    const auto& fooMembers = static_cast<const mecs::RTTIStruct&>(mecs::rttiOf<Foo>()).members;
    const auto& barMembers = static_cast<const mecs::RTTIStruct&>(mecs::rttiOf<Bar>()).members;

    REQUIRE(fooMembers.size() == 2);
    REQUIRE(barMembers.size() == 1);
    REQUIRE(std::string(fooMembers[0].name) == "foo");
    REQUIRE(std::string(fooMembers[1].name) == "bar");
    REQUIRE(std::string(barMembers[0].name) == "name");
    REQUIRE(barMembers[0].memberRtti->typeID == mecs::typeIdOf<std::string>());

    void* foo = &world.entityGetComponent<Foo>(entity);
    void* bar = &world.entityGetComponent<Bar>(entity);

    REQUIRE(*reinterpret_cast<int*>(fooMembers[0].getMemberFn(foo)) == 42);
    REQUIRE(*reinterpret_cast<float*>(fooMembers[1].getMemberFn(foo)) == 3.14F);
    REQUIRE(*reinterpret_cast<std::string*>(barMembers[0].getMemberFn(bar)) == "DuffyDuck");
}
// NOLINTEND