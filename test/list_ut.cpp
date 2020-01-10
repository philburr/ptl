#include "catch2/catch.hpp"
#include "ptl/containers/list.hpp"
#include "ptl/containers/slist.hpp"

struct in_list
{
    int data;

    ptl::ListEntry<LIST_BINDING(in_list, entry1)> entry1;
    ptl::ListEntry<LIST_BINDING(in_list, entry2)> entry2;
};
MAKE_LIST_BINDING(in_list, entry1);
MAKE_LIST_BINDING(in_list, entry2);

struct the_list
{
    ptl::List<LIST_BINDING(in_list, entry1)> list1;
    ptl::List<LIST_BINDING(in_list, entry2)> list2;
};

struct in_slist
{
    int data;

    ptl::SListEntry<SLIST_BINDING(in_slist, entry1)> entry1;
    ptl::SListEntry<SLIST_BINDING(in_slist, entry2)> entry2;
};
MAKE_SLIST_BINDING(in_slist, entry1);
MAKE_SLIST_BINDING(in_slist, entry2);

struct the_slist
{
    ptl::SList<SLIST_BINDING(in_slist, entry1)> list1;
    ptl::SList<SLIST_BINDING(in_slist, entry2)> list2;
};


TEST_CASE("list")
{
    the_list list;
    list.list1.push_back(new in_list{3});
    list.list1.push_back(new in_list{4});
    list.list2.push_back(new in_list{5});
    list.list2.push_back(new in_list{6});
    REQUIRE(list.list1.front()->data == 3);
    REQUIRE(list.list1.back()->data == 4);
    REQUIRE(list.list2.front()->data == 5);
    REQUIRE(list.list2.back()->data == 6);

    delete list.list1.front();
    delete list.list1.front();
    delete list.list2.front();
    delete list.list2.front();
    REQUIRE(list.list1.empty());
    REQUIRE(list.list2.empty());
}

TEST_CASE("slist")
{
    the_slist list;
}