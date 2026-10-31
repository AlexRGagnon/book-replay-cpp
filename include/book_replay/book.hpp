#pragma once
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace book_replay {
enum class Side { Buy, Sell };
struct Order { std::string id; Side side; std::int64_t price; std::int64_t remaining; };
struct Trade { std::string maker_id, taker_id; std::int64_t price, quantity; };
struct Level { std::int64_t price, quantity; std::size_t orders; };
struct Result { bool ok; std::string message; std::vector<Trade> trades; };

class Book {
public:
  Result add(std::string id, Side side, std::int64_t price, std::int64_t quantity);
  Result cancel(const std::string& id);
  // Replacing with an increased size or a new price loses time priority.
  // A same-price size reduction preserves priority.
  Result replace(const std::string& id, std::int64_t price, std::int64_t remaining);
  std::optional<Level> best_bid() const;
  std::optional<Level> best_ask() const;
  std::size_t order_count() const { return index_.size(); }
  bool check_invariants(std::string* reason = nullptr) const;
private:
  using Queue = std::list<Order>;
  using Bids = std::map<std::int64_t, Queue, std::greater<std::int64_t>>;
  using Asks = std::map<std::int64_t, Queue>;
  struct Location { Side side; std::int64_t price; Queue::iterator iterator; };
  Bids bids_;
  Asks asks_;
  std::unordered_map<std::string, Location> index_;
  void remove(const std::string& id);
};
}
