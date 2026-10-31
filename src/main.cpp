#include "book_replay/book.hpp"
#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using book_replay::Book;
namespace {
std::vector<std::string> fields(const std::string& line) {
  std::vector<std::string> v;
  std::stringstream s(line);
  std::string x;
  while (std::getline(s, x, ',')) v.push_back(x);
  return v;
}
std::int64_t integer(const std::string& s) {
  std::int64_t result{};
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), result);
  if (ec != std::errc{} || ptr != s.data() + s.size()) throw std::runtime_error("invalid integer: " + s);
  return result;
}
void print_level(const char* side, std::optional<book_replay::Level> l) {
  if (l) std::cout << side << ',' << l->price << ',' << l->quantity << ',' << l->orders << '\n';
  else std::cout << side << ",EMPTY\n";
}
}
int main(int argc, char** argv) {
  if (argc != 2) { std::cerr << "usage: book-replay events.csv\n"; return 2; }
  std::ifstream f(argv[1]);
  if (!f) { std::cerr << "cannot open input file\n"; return 2; }
  Book book;
  std::string line;
  std::size_t line_no = 0, events = 0, fills = 0;
  try {
    while (std::getline(f, line)) {
      ++line_no;
      if (line.empty() || line.starts_with('#')) continue;
      const auto p = fields(line);
      if (p.size() != 5) throw std::runtime_error("expected five fields");
      book_replay::Result r;
      if (p[0] == "ADD") {
        const auto side = p[2] == "B" ? book_replay::Side::Buy : book_replay::Side::Sell;
        if (p[2] != "B" && p[2] != "S") throw std::runtime_error("invalid side");
        r = book.add(p[1], side, integer(p[3]), integer(p[4]));
      } else if (p[0] == "CANCEL") {
        r = book.cancel(p[1]);
      } else if (p[0] == "REPLACE") {
        r = book.replace(p[1], integer(p[3]), integer(p[4]));
      } else throw std::runtime_error("unknown action");
      if (!r.ok) throw std::runtime_error(r.message);
      ++events;
      for (const auto& t : r.trades) {
        std::cout << "TRADE," << t.maker_id << ',' << t.taker_id << ',' << t.price << ',' << t.quantity << '\n';
        ++fills;
      }
      std::string why;
      if (!book.check_invariants(&why)) throw std::runtime_error("book invariant failed: " + why);
    }
  } catch (const std::exception& e) {
    std::cerr << "line " << line_no << ": " << e.what() << '\n';
    return 1;
  }
  std::cout << "EVENTS," << events << ",FILLS," << fills << ",RESTING," << book.order_count() << '\n';
  print_level("BID", book.best_bid());
  print_level("ASK", book.best_ask());
}
