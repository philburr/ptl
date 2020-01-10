#include "catch2/catch.hpp"
#include "ptl/sync/event.hpp"

#include <thread>

using namespace ptl::sync;

void thread1(manual_reset_event* ev1, manual_reset_event* ev2) {
    ev1->wait();
    ev2->set();
}
void thread2(manual_reset_event* ev1, manual_reset_event* ev2) {
    ev1->set();
    ev2->wait();
}

TEST_CASE("event")
{
    manual_reset_event ev1;
    manual_reset_event ev2;
    std::thread first(thread1, &ev1, &ev2);
    std::thread second(thread2, &ev1, &ev2);

    first.join();
    second.join();
}