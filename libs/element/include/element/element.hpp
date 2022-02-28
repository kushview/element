
#pragma once
#include <element/element.h>

#define EL_DISABLE_COPY(ClassName)         \
    ClassName (const ClassName&) = delete; \
    ClassName& operator= (const ClassName&) = delete;
#define EL_DISABLE_MOVE(ClassName)          \
    ClassName (const ClassName&&) = delete; \
    ClassName& operator= (const ClassName&&) = delete;
