#pragma once

#include "base.h"
#include "defines.h"

MECS_EXTERNCPP()

MECS_API void mecsIterComponent(MecsIterator* iterator, MecsComponentID component, MecsSize argIndex);
MECS_API void mecsIterComponentFilter(MecsIterator* iterator, MecsComponentID component, MecsIteratorFilter filter, MecsSize argIndex);
MECS_API void mecsIteratorFinalize(MecsIterator* iterator);
MECS_API void mecsIteratorBegin(MecsIterator* iterator);
MECS_API bool mecsIteratorAdvance(MecsIterator* iterator);
MECS_API void* mecsIteratorGetArgument(MecsIterator* iterator, MecsSize argIndex);
MECS_API MecsWorld* mecsIteratorGetWorld(MecsIterator* iterator);
MECS_API MecsEntityID mecsIteratorGetEntity(MecsIterator* iterator);
MECS_API MecsSize mecsUtilIteratorCount(MecsIterator* iterator);

MECS_ENDEXTERNCPP()