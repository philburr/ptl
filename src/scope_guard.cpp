#include "ptl/scope_guard.hpp"

#include <iostream>

/*static*/ void ptl::detail::scope_guard_impl_base::warn_about_to_crash() noexcept
{
    // Ensure the availability of std::cerr
    std::ios_base::Init ioInit;
    std::cerr << "This program will now terminate because a ScopeGuard callback "
                 "threw an \nexception.\n";
}