/// @file Result.cpp
/// @brief Result型の実装
#include "common/result/Core/Result.h"
#include "common/result/Core/InternalAccessor.h"
#include <cstdio>
#include <cstdlib>

namespace NS { namespace result { namespace detail {

    [[noreturn]] void OnUnhandledResult(::NS::Result result) noexcept
    {
        std::fprintf(stderr,
                     "[FATAL] Unhandled Result: Module=%d, Description=%d, Raw=0x%08X\n"
                     "        Conversion to ResultSuccess failed - Result was not success.\n",
                     result.GetModule(),
                     result.GetDescription(),
                     result.GetRawValue());

        std::abort();
    }

}}} // namespace NS::result::detail
