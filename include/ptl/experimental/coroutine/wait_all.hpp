#include "sync_wait.hpp"
#include "detail/is_awaiter.hpp"
#include "detail/wait_all_counter.hpp"
#include "detail/wait_all_awaiter.hpp"
#include "detail/wait_all_task.hpp"

namespace ptl::experimental::coroutine {

template<typename... AWAITABLES,
         std::enable_if_t<std::conjunction_v<detail::is_awaitable<std::remove_reference_t<AWAITABLES>>...>, int> = 0>
auto wait_all(AWAITABLES&&... awaitables)
{
    return detail::wait_all_awaitable<std::tuple<detail::wait_all_task<typename awaitable_traits<std::remove_reference_t<AWAITABLES>>::await_result_t>...>>(
        std::make_tuple(detail::make_wait_all_task(std::forward<AWAITABLES>(awaitables))...));
    
}

}