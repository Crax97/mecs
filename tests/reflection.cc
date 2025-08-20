
#include "catch_amalgamated.hpp"
#include "mecs/base.h"
#include "mecs/mecs.h"
#include "mecs/registry.h"
#include "mecs/world.h"

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
        ComponentInfo info = { .name = "const char*", .size = sizeof(const char*), .align = alignof(const char*) };
        Component_ConstCharPtr = mecsRegistryAddRegistration(registry, &info);
    } while (0);
    static MecsComponentID Component_Foo = ~0U;
    {
        ComponentInfo info = { .name = "Foo", .size = sizeof(Foo), .align = alignof(Foo) };
        static std::array members {
            MecsComponentMember {   .componentID = Component_int, .offset = offsetof(Foo, foo), .name = "foo" },
            MecsComponentMember { .componentID = Component_float, .offset = offsetof(Foo, bar), .name = "bar" },
        };
        info.memberCount = members.size();
        info.members = members.data();
        Component_Foo = mecsRegistryAddRegistration(registry, &info);
    }
    static MecsComponentID Component_Bar = ~0U;
    {
        ComponentInfo info = { .name = "Bar", .size = sizeof(Bar), .align = alignof(Bar) };
        static std::array members {
            MecsComponentMember { .componentID = Component_ConstCharPtr, .offset = offsetof(Bar, name), .name = "name" },
        };
        info.memberCount = members.size();
        info.members = members.data();
        Component_Bar = mecsRegistryAddRegistration(registry, &info);
    }

    MecsWorld* world = mecsWorldCreate(registry, nullptr);

    MecsEntityID ent0 = mecsWorldSpawnEntity(world, nullptr);
    MECS_COMPONENT(world, ent0, Foo);
    Foo* foo = (Foo*)(mecsWorldEntityGetComponent(world, ent0, Component_Foo));
    *foo = { 42, 3.14F };

    const ComponentInfo* fooInfo = mecsGetComponentInfoByID(registry, Component_Foo);

    REQUIRE(fooInfo->memberCount == 2);

    int* fooP = (int*)((MecsU8*)(foo) + fooInfo->members[0].offset);
    float* barP = (float*)((MecsU8*)(foo) + fooInfo->members[1].offset);

    mecsWorldFlushEvents(world);

    REQUIRE(*fooP == 42);
    REQUIRE(*barP == 3.14F);

    mecsWorldFree(world);
    mecsRegistryFree(registry);
}
// NOLINTEND