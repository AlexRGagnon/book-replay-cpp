# book-replay-cpp

A C++20 limit-order-book replay engine for deterministic testing of price/time priority, order replacement, and partial fills. This is a **new portfolio implementation**, not an exchange-certified matching engine or evidence of historical client work.

## Why it exists

Many toy backtests silently treat an entire price level as one order. This library models FIFO priority **within** each price level so order cancellation, replacement, and partial fills can be verified with precise expected outputs.

## Features

- Integer price ticks and quantities (no floating-point price ordering).
- Limit orders match at the *resting* maker's price; unfilled quantity rests in the book.
- Same-price size reduction retains FIFO position. Increasing size or changing price loses priority.
- Duplicate/unknown order IDs and invalid sizes are rejected without mutating the book.
- Invariant checker confirms index/queue consistency and no crossed resting book.
- Deterministic CTest coverage including randomized invariant checks.
- Strict, intentionally minimal comma-delimited replay format; not general CSV with quoted fields.

## Local build and tests (Linux, macOS, Windows with CMake + C++20 compiler)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/book-replay examples/events.csv  # Windows: .\\build\\Release\\book-replay.exe
```

CSV columns: `action,id,side,price_ticks,quantity`. `ADD` requires all fields; `REPLACE` ignores `side` and changes the resting order's *remaining* quantity; `CANCEL` ignores price/qty. No embedded commas/quoted cells; identifiers must be nonempty. Negative prices or quantities are invalid.

Output includes maker/taker fills, event/fill counts, resting order count, best bid and best ask. A line with an invalid event produces a line-numbered error and nonzero exit code.

## Limitations

Not designed for high-frequency exchange production use. No auction sessions, market orders, self-trade prevention, order types beyond limits, persistence, or allocation beyond FIFO. Only in-memory replay is supported. No broker connections. No GitHub Actions or hosted compute.

## Attribution

Independent new implementation for a technical portfolio. Standard library only; no third-party runtime dependencies. MIT licensed.
