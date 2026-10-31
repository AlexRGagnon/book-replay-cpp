#include "book_replay/book.hpp"
#include <stdexcept>
#define REQUIRE(condition) do { if (!(condition)) throw std::runtime_error(std::string("REQUIRE failed at line ") + std::to_string(__LINE__) + ": " + #condition); } while (false)
#include <iostream>
#include <random>
#include <string>
#include <vector>
using namespace book_replay;

void test_fifo_and_partial() {
  Book b;
  REQUIRE(b.add("b1",Side::Buy,100,5).ok);
  REQUIRE(b.add("b2",Side::Buy,100,7).ok);
  auto r=b.add("s1",Side::Sell,99,8);
  REQUIRE(r.ok && r.trades.size()==2);
  REQUIRE(r.trades[0].maker_id=="b1" && r.trades[0].quantity==5);
  REQUIRE(r.trades[1].maker_id=="b2" && r.trades[1].quantity==3);
  REQUIRE(b.best_bid()->quantity==4 && b.best_bid()->orders==1);
  REQUIRE(b.check_invariants());
}
void test_replace_priority() {
  Book b;
  b.add("b1",Side::Buy,100,10);
  b.add("b2",Side::Buy,100,10);
  REQUIRE(b.replace("b1",100,7).ok);
  auto r=b.add("s1",Side::Sell,100,2);
  REQUIRE(r.trades[0].maker_id=="b1");
  REQUIRE(b.replace("b1",100,12).ok); // increase loses time priority
  r=b.add("s2",Side::Sell,100,3);
  REQUIRE(r.trades[0].maker_id=="b2");
  REQUIRE(b.check_invariants());
}
void test_crossing_and_validation() {
  Book b;
  REQUIRE(!b.add("bad",Side::Buy,-1,1).ok);
  REQUIRE(b.add("a",Side::Sell,102,4).ok);
  auto r=b.add("b",Side::Buy,103,10);
  REQUIRE(r.trades.size()==1 && r.trades[0].price==102 && r.trades[0].quantity==4);
  REQUIRE(b.best_bid()->quantity==6 && !b.best_ask());
  REQUIRE(!b.add("b",Side::Buy,103,1).ok);
  REQUIRE(!b.cancel("missing").ok);
  REQUIRE(!b.replace("b",103,0).ok);
  REQUIRE(b.cancel("b").ok && !b.best_bid());
  REQUIRE(b.check_invariants());
}
void test_amend_crossing() {
  Book b;
  b.add("b",Side::Buy,100,9);
  b.add("s",Side::Sell,105,3);
  auto r=b.replace("b",106,7);
  REQUIRE(r.ok && r.trades.size()==1 && r.trades[0].quantity==3);
  REQUIRE(b.best_bid()->price==106 && b.best_bid()->quantity==4);
}
void test_randomized_invariants() {
  Book b;
  std::mt19937 g(1729);
  std::vector<std::string> ids;
  for (int i=0;i<2500;++i) {
    const auto id="id"+std::to_string(i);
    const auto p=99+(g()%4), q=1+(g()%15);
    REQUIRE(b.add(id, i%2 ? Side::Buy : Side::Sell,p,q).ok);
    ids.push_back(id);
    if (i%7==0) {
      const auto& old=ids[g()%ids.size()];
      auto r=b.cancel(old);
      if (!r.ok) REQUIRE(r.message=="unknown order id");
    }
    REQUIRE(b.check_invariants());
  }
}
int main() {
  test_fifo_and_partial();
  test_replace_priority();
  test_crossing_and_validation();
  test_amend_crossing();
  test_randomized_invariants();
  std::cout << "5 C++ test groups passed\n";
}
