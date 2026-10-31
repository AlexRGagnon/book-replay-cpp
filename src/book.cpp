#include "book_replay/book.hpp"
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace book_replay {
namespace {
Result fail(std::string message) { return {false, std::move(message), {}}; }
Result success(std::vector<Trade> trades = {}) { return {true, {}, std::move(trades)}; }
std::optional<Level> head(const std::int64_t price, const std::list<Order>& q) {
  if (q.empty()) return std::nullopt;
  std::int64_t total = 0;
  for (const auto& x : q) {
    if (x.remaining > std::numeric_limits<std::int64_t>::max() - total)
      throw std::overflow_error("aggregated level quantity exceeds int64");
    total += x.remaining;
  }
  return Level{price, total, q.size()};
}
}

Result Book::add(std::string id, Side side, std::int64_t price, std::int64_t quantity) {
  if (id.empty() || id.find(',') != std::string::npos) return fail("invalid order id");
  if (price <= 0 || quantity <= 0) return fail("price and quantity must be positive");
  if (index_.contains(id)) return fail("duplicate order id");
  std::vector<Trade> trades;
  std::int64_t rest = quantity;
  if (side == Side::Buy) {
    while (rest > 0 && !asks_.empty() && asks_.begin()->first <= price) {
      auto level = asks_.begin();
      auto& q = level->second;
      auto& maker = q.front();
      const auto fill = std::min(rest, maker.remaining);
      trades.push_back({maker.id, id, maker.price, fill});
      rest -= fill;
      maker.remaining -= fill;
      if (maker.remaining == 0) { index_.erase(maker.id); q.pop_front(); }
      if (q.empty()) asks_.erase(level);
    }
    if (rest > 0) {
      auto& q = bids_[price];
      q.push_back({id, side, price, rest});
      index_.emplace(id, Location{side, price, std::prev(q.end())});
    }
  } else {
    while (rest > 0 && !bids_.empty() && bids_.begin()->first >= price) {
      auto level = bids_.begin();
      auto& q = level->second;
      auto& maker = q.front();
      const auto fill = std::min(rest, maker.remaining);
      trades.push_back({maker.id, id, maker.price, fill});
      rest -= fill;
      maker.remaining -= fill;
      if (maker.remaining == 0) { index_.erase(maker.id); q.pop_front(); }
      if (q.empty()) bids_.erase(level);
    }
    if (rest > 0) {
      auto& q = asks_[price];
      q.push_back({id, side, price, rest});
      index_.emplace(id, Location{side, price, std::prev(q.end())});
    }
  }
  return success(std::move(trades));
}

void Book::remove(const std::string& id) {
  auto loc = index_.at(id);
  if (loc.side == Side::Buy) {
    auto level = bids_.find(loc.price);
    level->second.erase(loc.iterator);
    if (level->second.empty()) bids_.erase(level);
  } else {
    auto level = asks_.find(loc.price);
    level->second.erase(loc.iterator);
    if (level->second.empty()) asks_.erase(level);
  }
  index_.erase(id);
}

Result Book::cancel(const std::string& id) {
  if (!index_.contains(id)) return fail("unknown order id");
  remove(id);
  return success();
}

Result Book::replace(const std::string& id, std::int64_t price, std::int64_t remaining) {
  auto found = index_.find(id);
  if (found == index_.end()) return fail("unknown order id");
  if (price <= 0 || remaining <= 0) return fail("price and quantity must be positive");
  auto loc = found->second;
  if (price == loc.price && remaining <= loc.iterator->remaining) {
    loc.iterator->remaining = remaining;
    return success();
  }
  const auto side = loc.side;
  remove(id);
  return add(id, side, price, remaining);
}

std::optional<Level> Book::best_bid() const {
  return bids_.empty() ? std::nullopt : head(bids_.begin()->first, bids_.begin()->second);
}
std::optional<Level> Book::best_ask() const {
  return asks_.empty() ? std::nullopt : head(asks_.begin()->first, asks_.begin()->second);
}

bool Book::check_invariants(std::string* reason) const {
  auto invalid = [&](const std::string& why) { if (reason) *reason = why; return false; };
  std::unordered_set<std::string> seen;
  auto verify = [&](const auto& levels, Side side) {
    for (const auto& [price, q] : levels) {
      if (q.empty() || price <= 0) return false;
      for (const auto& order : q) {
        if (order.side != side || order.price != price || order.remaining <= 0) return false;
        if (!seen.insert(order.id).second) return false;
        auto p = index_.find(order.id);
        if (p == index_.end() || p->second.side != side || p->second.price != price ||
            std::addressof(*p->second.iterator) != std::addressof(order)) return false;
      }
    }
    return true;
  };
  if (!verify(bids_, Side::Buy)) return invalid("invalid bids/index");
  if (!verify(asks_, Side::Sell)) return invalid("invalid asks/index");
  if (seen.size() != index_.size()) return invalid("index size mismatch");
  if (!bids_.empty() && !asks_.empty() && bids_.begin()->first >= asks_.begin()->first)
    return invalid("crossed resting book");
  return true;
}
}
