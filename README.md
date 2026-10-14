# Mercury

Mercury is a C++23 research prototype for queue-aware market making. Lessons
1–18 now cover live Coinbase public market data, exact normalization, recording,
deterministic replay, delayed simulated orders, FIFO queues, inventory-aware
quotes, accounting, risk, and performance measurement. All execution is simulated.

## Build and test

Requires CMake 3.25+, a C++23 compiler, Boost.System, OpenSSL, and Git. CMake
fetches pinned nlohmann/json v3.12.0 and GoogleTest v1.17.0. On Apple Silicon,
use your existing Homebrew Boost and OpenSSL installations:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/boost;/opt/homebrew/opt/openssl@3"
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

`compile_commands.json` is generated in the build directory for clangd. Tests
use fixed fixtures and require no exchange connection. The GitHub workflow
runs Release and sanitizer builds on Linux. Set `BUILD_TESTING=OFF` to build
applications without GoogleTest.

## Run the complete offline experiment

```sh
./build/mercury_replay tests/fixtures/sample.jsonl
./build/mercury_backtest tests/fixtures/sample.jsonl
./build/mercury_backtest tests/fixtures/sample.jsonl 1000 10
./build/mercury_backtest tests/fixtures/sample.jsonl 1000000 0
```

Backtest arguments are `recording [latency_us] [fee_bps] [order_units]`.
Defaults: 1,000 microseconds for new orders and cancels, zero maker fees, and
100,000 base units per order. A latency of zero is allowed. Fees are integer
basis points in [0, 10000]; order size is in [1, 1000000]. Inventory is capped at
ten order lots. The default notional cap is 1,000 quote-currency units per order,
with two open orders and a 100-unit drawdown stop. Further configuration is in
`PaperConfig` and its component structs.

The synthetic fixture gives three fills and ends flat. Its equity is
0.0000200000 quote-currency units with zero fees. At 10 bps fees, equity becomes
-0.0001800200. With one-second activation latency, the fixture produces no fills.
These values verify arithmetic and causality on a toy dataset.

## Capture and replay live market data

```sh
./build/mercury_feed_smoke
./build/mercury_live 100
./build/mercury_record build/btc-session.jsonl 1000
./build/mercury_replay build/btc-session.jsonl
./build/mercury_backtest build/btc-session.jsonl 1000 10
```

Live applications subscribe to public BTC-USD data without credentials. They
have network deadlines and at most two reconnection attempts. The recorder
refuses to overwrite an existing file. A partial recording remains available
if capture fails. Connection boundaries and sequence gaps reset synchronization;
a fresh snapshot is required before quoting can resume.

## Architecture and units

```text
Coinbase TLS/WebSocket → raw JSON → exact parser → synchronized L2 book
                              ↓ recording
                       JSONL arrival stream
                              ↓ replay clock
inventory quote policy → risk checks → delayed simulated exchange
                                      ↓ FIFO fills
                              cash, inventory, fees, equity
```

BTC-USD defaults use cents for price ticks and satoshis for quantities.
Recording metadata stores the product and decimal scales. Market prices,
quantities, queues, and inventory remain integral; decimal precision errors and
overflow are rejected. Cash and statistics use long double in quote-currency
units. Trade side is converted from Coinbase maker side to Mercury aggressor side.

## Model assumptions

- Price-level quantities are replacements; zero deletes a level. Snapshots
  replace the book. Market trades do not mutate authoritative L2 state.
- Simulated orders join behind displayed liquidity at activation. A shared FIFO
  spends each matching trade's volume once. Unchanged quotes keep priority.
- The model grants no cancellation credit and no inferred fills from trades at
  other prices. It excludes hidden liquidity, market impact, and exact L3 priority.
- The first server envelope anchors exchange time to captured arrival time per
  connection. This is a fixed clock-offset estimate. Delayed trades cannot fill
  newer queue entries; cancelled queues are not reconstructed for late reports.
- Pending and cancelling orders reserve exposure. Cancellation can race with
  fills. Replacements wait for cancellation acknowledgment. Book invalidation
  clears simulated exposure immediately at a data boundary.
- At equal times, scheduled actions run before the incoming message. EOF stops
  simulated orders without inventing liquidation fills. Equity marks residual
  inventory at the final integer midpoint; it is not necessarily realized PnL.
- Historical trade snapshots and recent duplicate IDs are excluded from fills.
  Duplicate-ID memory is bounded to the last 10,000 IDs per connection.

The executable models a paper experiment. Live exchange order placement is
outside this completed lesson outline.

## Benchmark and memory checks

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/boost;/opt/homebrew/opt/openssl@3"
cmake --build build/release -j 4
./build/release/mercury_benchmark 10000

cmake -S . -B build/sanitize -DCMAKE_BUILD_TYPE=Debug \
  -DMERCURY_ENABLE_SANITIZERS=ON \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/boost;/opt/homebrew/opt/openssl@3"
cmake --build build/sanitize -j 4
ctest --test-dir build/sanitize --output-on-failure
```

Benchmark distributions include measurement overhead. Compare the same workload
and compiler settings on the same machine; they measure local operations.

Protocol sources: [Coinbase level2](https://docs.cdp.coinbase.com/api-reference/advanced-trade-api/websocket/level2)
and [official schemas](https://docs.cdp.coinbase.com/api-reference/advanced-trade-api/advanced-trade-asyncapi.json).
