# Mercury

**Mercury** is a C++23 queue-aware market-making engine built to explore limit-order-book mechanics, passive execution, queue position, latency, and market-making infrastructure.

The project is being developed incrementally, with a strong emphasis on correctness, type safety, testing, and systems design.

## Current Status

Mercury currently includes:

- C++23 project infrastructure with CMake
- strict compiler warnings
- `clangd` / `compile_commands.json` support
- strongly typed market primitives
- `Price` represented as integer ticks rather than floating point
- bid/ask side representation
- reusable `Side` string conversion
- GoogleTest unit testing
- mutable L2 limit order book
- bid and ask price-level storage
- L2 insert / replace / delete semantics
- best bid / ask retrieval
- rejection of negative quantities

The development roadmap includes market-event models, market-data recording and replay, latency simulation, order lifecycle modelling, queue-aware execution, inventory-aware quoting, risk controls, and performance benchmarking.

## Testing

Build the project and run the test suite:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## Development

Mercury is under active development, with hopes to finish by beginning of Michaelmas 26. Feedback, discussion, and suggestions for improving the implementation/architecture are always welcome.

