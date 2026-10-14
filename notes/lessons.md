# Mercury lesson ledger

Lessons 1–8 and 9A–9B were completed before this continuation.

## Roadmap

9C exact level2 parsing; 9D trade parsing; 9E feed recovery; 9F live book integration.
10 recording; 11 deterministic replay; 12 order lifecycle; 13 latency;
14 queue and fill modelling; 15 inventory-aware quoting; 16 risk controls;
17 measurement and benchmarking; 18 end-to-end integration and documentation.

## 9C — exact market-data parsing

Decimal text is accumulated with checked integer arithmetic. Configured decimal
scales define price ticks and quantity units (BTC-USD defaults: cents and satoshis).
Off-increment values are rejected rather than rounded. UTC timestamps retain
nanoseconds. Book batches preserve snapshot versus update semantics, and an
entire message is validated before any batch is returned.

The parser owns its result; it has no network or order-book side effects.
Tests cover precision, overflow, calendar validity, product filtering, malformed
input, side mapping, and zero-quantity deletion.

## 9D — public trade normalization

Coinbase's trade side describes the maker, so Mercury inverts it for its
aggressor-side enum. IDs and snapshot/update kind survive normalization.
Historical snapshots will be excluded from simulated fills. Subscriptions are
serialized as JSON, including heartbeats and market trades.

## 9E — synchronization and recovery

A FeedSession checks the per-connection sequence across all channels, ignores
old envelopes, and clears the book on gaps, disconnects, or stale arrival time.
A new snapshot is mandatory after reset. Historical/duplicate trades cannot
generate fills. The transport uses asynchronous reads internally to enforce
Beast idle deadlines while exposing a simple blocking interface. Each
connection attempt owns a fresh client. DNS resolution remains synchronous.

Sources: [Coinbase schema](https://docs.cdp.coinbase.com/api-reference/advanced-trade-api/advanced-trade-asyncapi.json),
[level2](https://docs.cdp.coinbase.com/api-reference/advanced-trade-api/websocket/level2).

## 9F — live book application

Callbacks separate connection ownership from message processing. mercury_live
subscribes to level2, trades, and heartbeats; reconnects at most twice with
exponential delays; and demands a fresh snapshot after connection reset.
Network connection and read stages use asynchronous deadlines. The application
prints normalized best prices only after the feed is synchronized.

## 10 — recording

Versioned JSONL records retain the raw exchange message, monotonic arrival
time, connection starts, product ID, and integer scales. Stream references
allow the same recorder to write files or test buffers. Writers report I/O
failures; readers validate schema and clock order with line-number diagnostics.
mercury_record refuses to overwrite an existing file.

## 11 — deterministic replay

Arrival order is authoritative. Replay advances a virtual clock without
sleeping and uses the same parser and feed session as the live application.
Reconnect markers reset sequence and book state. Gaps invalidate the book and
are counted. A synthetic recording provides repeatable offline experiments.
Exchange time is metadata, not permission to reorder or look into the future.

## 12 — order lifecycle

OrderManager owns orders with unique IDs and validates state transitions.
Remaining quantity is derived from original minus filled quantity. Pending
new orders cannot fill; pending cancellation orders still can. Cancellation
acknowledgment ends exposure, unless a full fill already ended it. Tests cover
partial fills, cancellation races, rejection, overfills, and clock ordering.

## 13 — virtual latency

A min-priority queue runs actions by (deadline, submission sequence). Equal
deadlines retain FIFO order. Time only advances forward; timestamp additions
are checked for overflow. Configurable new-order and cancellation delays will
control when simulated exposure begins and ends. No wall-clock sleeps are used.

## 14 — queue-aware simulated execution

Each price has one FIFO containing external liquidity and our simulated orders.
A matching trade spends its volume once across that queue. Activation happens
after new-order latency and samples the then-observed book; crossing orders
are rejected as post-only. Fills remain possible until cancellation arrives.

This is a conservative L2 approximation: no cancellation credit, no inferred
trade-through fills, no hidden liquidity, and no market impact. New visible
liquidity is reconciled when another order joins. Public L2 updates do not
independently consume queue positions, avoiding trade/update double counting.
Resynchronization invalidates all simulated resting and pending orders.

## 15 — inventory-aware quote policy

A pure function returns desired quotes from observed book and inventory.
Midpoint and prices stay integral. Each inventory lot shifts both quotes by a
tick, capped by a configurable skew. Prices are clamped to remain post-only;
the risk-increasing side is suppressed at inventory limits. Empty, crossed,
or unrepresentable books produce no quotes. Execution remains separate.

## 16 — accounting and risk

Inventory remains integral. Cash, fees, turnover, and marked equity use long
double in quote-currency units for reporting. Risk reserves all nonterminal
orders, including pending-new and pending-cancel quantities. Buy and sell
exposure are checked separately rather than netted. Limits cover order size,
notional, open count, worst-case inventory, and peak-to-trough equity loss.
Loss and manual stops latch. Unsynchronized feeds cannot submit orders.

## 17 — latency measurement

LatencySamples reports nearest-rank median/p95/p99, extrema, and mean. The
benchmark measures varying book updates and normalized level2 parsing with a
checksum to keep results observable. Use a Release build and compare identical
workloads on the same machine. Timings include clock/sampling overhead; these
are microbenchmarks, not exchange-to-strategy latency or profitability evidence.
