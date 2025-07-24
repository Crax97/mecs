#include "mecs/registry.h"
#include "mecs/base.h"

#include "private.h"

MecsRegistry* mecsRegistryCreate(const MecsRegistryCreateInfo* createInfo)
{

    MecsAllocator allocator;
    if (createInfo != nullptr && createInfo->memAllocator.memAlloc != nullptr) {
        allocator = createInfo->memAllocator;
    } else {
        allocator = kDefaultAllocator;
    }

    MecsRegistry* reg = mecsAlloc<MecsRegistry>(allocator, allocator);
    reg->memAllocator = allocator;

    return reg;
}

void updateComponentInfo(ComponentInfo& dest, const ComponentInfo& source)
{
    const char* nameCopy = dest.name;
    dest = source;
    dest.name = nameCopy;
}

MecsComponentID
mecsRegistryAddRegistration(MecsRegistry* reg,
    const ComponentInfo* vtable)
{
    MECS_ASSERT(reg != nullptr && vtable != nullptr);
    MECS_ASSERT(vtable->size > 0 && vtable->align > 0 && vtable->name != nullptr);

    MecsSize elementCount = reg->components.count();
    for (MecsSize i = 0; i < elementCount; i++) {
        ComponentInfo& info = reg->components[i];
        if (mecsStrEqual(info.name, vtable->name)) {
            updateComponentInfo(info, *vtable);
            return static_cast<MecsComponentID>(i);
        }
    }

    ComponentInfo info = *vtable;
    info.name = mecsStrDup(reg->memAllocator, vtable->name);

    MECS_ASSERT(info.name != vtable->name);

    MecsSize size = reg->components.count();
    reg->components.push(reg->memAllocator, info);
    return size;
}

void mecsRegistryFree(MecsRegistry* registry)
{
    if (registry == nullptr) {
        return;
    }

    const MecsSize numComponents = registry->components.count();
    for (MecsSize i = 0; i < numComponents; i++) {
        ComponentInfo& info = registry->components[i];
        mecsFree(registry->memAllocator, info.name);
    }

    registry->components.destroy(registry->memAllocator);
    mecsFree(registry->memAllocator, registry);
}