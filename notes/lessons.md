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
