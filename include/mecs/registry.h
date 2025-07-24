#pragma once
#include "base.h"
#include "defines.h"

MECS_EXTERNCPP()

MecsRegistry* MECS_API mecsRegistryCreate(const MecsRegistryCreateInfo* createInfo);
void MECS_API mecsRegistryFree(MecsRegistry* registry);
MecsComponentID MECS_API mecsRegistryAddRegistration(MecsRegistry* reg, const ComponentInfo* vtable);

MECS_ENDEXTERNCPP()