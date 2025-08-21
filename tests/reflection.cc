
#include "catch_amalgamated.hpp"
#include "mecs/base.h"
#include "mecs/mecs.h"
#include "mecs/registry.h"
#include "mecs/world.h"

#include "mecshpp/mecs.hpp"

#include "test_private.hpp"
#include <cstddef>

// NOLINTBEGIN
TEST_CASE("C Reflection Basics")
{
    struct Foo {
        int foo;
        float bar;
    };

    struct Bar {
        const char* name;
    };

    MecsRegistryCreateInfo regInfo {};
    regInfo.memAllocator = kDebugAllocator;
    MecsRegistry* registry = mecsRegistryCreate(&regInfo);
    MECS_REGISTER_COMPONENT(registry, int);
    MECS_REGISTER_COMPONENT(registry, float);
    static MecsComponentID Component_ConstCharPtr = ~0U;
    do {
        ComponentInfo info = {
            .name = "const char*",
            .typeID = __COUNTER__,
            .size = sizeof(const char*),
            .align = alignof(const char*),
        };
        Component_ConstCharPtr = mecsRegistryAddRegistration(registry, &info);
    } while (0);
    static MecsComponentID Component_Foo = ~0U;
    {
        ComponentInfo info = { .name = "Foo", .size = sizeof(Foo), .align = alignof(Foo) };
        static std::array members {
            MecsComponentMember {   .typeID = Component_int, .offset = offsetof(Foo, foo), .name = "foo" },
            MecsComponentMember { .typeID = Component_float, .offset = offsetof(Foo, bar), .name = "bar" },
        };
        info.memberCount = members.size();
        info.members = members.data();
        info.typeID = __COUNTER__;
        Component_Foo = mecsRegistryAddRegistration(registry, &info);
    }
    static MecsComponentID Component_Bar = ~0U;
    {
        ComponentInfo info = { .name = "Bar", .size = sizeof(Bar), .align = alignof(Bar) };
        static std::array members {
            MecsComponentMember { .typeID = Component_ConstCharPtr, .offset = offsetof(Bar, name), .name = "name" },
        };
        info.memberCount = members.size();
        info.members = members.data();
        info.typeID = __COUNTER__;
        Component_Bar = mecsRegistryAddRegistration(registry, &info);
    }

    MecsWorld* world = mecsWorldCreate(registry, nullptr);

    MecsEntityID ent0 = mecsWorldSpawnEntity(world, nullptr);
    MECS_COMPONENT(world, ent0, Foo);
    Foo* foo = (Foo*)(mecsWorldEntityGetComponent(world, ent0, Component_Foo));
    *foo = { 42, 3.14F };

    const ComponentInfo* fooInfo = mecsGetComponentInfoByComponentID(registry, Component_Foo);

    REQUIRE(fooInfo->memberCount == 2);

    int* fooP = (int*)((MecsU8*)(foo) + fooInfo->members[0].offset);
    float* barP = (float*)((MecsU8*)(foo) + fooInfo->members[1].offset);

    mecsWorldFlushEvents(world);

    REQUIRE(*fooP == 42);
    REQUIRE(*barP == 3.14F);

    mecsWorldFree(world);
    mecsRegistryFree(registry);
}

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
    const ComponentInfo& fooInfo = registry.getComponentInfo<Foo>();
    const ComponentInfo& barInfo = registry.getComponentInfo<Bar>();

    void* foo = &world.entityGetComponent<Foo>(entity);
    void* bar = &world.entityGetComponent<Bar>(entity);

    REQUIRE(fooInfo.memberCount == 2);
    REQUIRE(barInfo.memberCount == 1);
    REQUIRE(std::string(fooInfo.members[0].name) == "foo");
    REQUIRE(std::string(fooInfo.members[1].name) == "bar");

    REQUIRE(std::string(barInfo.members[0].name) == "name");
    REQUIRE(barInfo.members[0].typeID == mecs::typeIdOf<std::string>());

    REQUIRE(*reinterpret_cast<int*>(static_cast<char*>(foo) + fooInfo.members[0].offset) == 42);
    REQUIRE(*reinterpret_cast<float*>(static_cast<char*>(foo) + fooInfo.members[1].offset) == 3.14F);
    REQUIRE(*reinterpret_cast<std::string*>(static_cast<char*>(bar) + barInfo.members[0].offset) == "DuffyDuck");
}
// NOLINTEND