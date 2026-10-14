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
