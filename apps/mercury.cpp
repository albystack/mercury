// gives us access to std::cout. in python, normally write print("hello"). c++,
// console output done with std::cout
#include <iostream>

// this includes the header file we created for mercury's core market types.
// because cmake added the project's include directory, to the include path, we
// can write:
//
// <mercury/market/types.hpp>
// instead of relative paths like ".//include/..."
// keeping includes independent of directory layout makes project cleaner as it
// grows
#include <mercury/market/types.hpp>

// every normal C++executable starts execution inside a function called main.
// main() usually. the int before it means this function returns int
//

int main() {
  // our types live inside the namespace
  // mercury:: market
  // their full names are therefore
  // 1) mercury::market::Price
  // 2) mercury::market::Quantity
  // 3) mercury::market::Timestamp
  // 4) mercury::market::Side
  // To avoid repeating that long namespace throughout this program, we bring
  // those four names into the scope of main somewhat analogous to: from
  // mercury.market import Price in python

  using mercury::market::Price;
  using mercury::market::Quantity;
  using mercury::market::Side;
  using mercury::market::Timestamp;
  using mercury::market::to_string;

  // example market values
  //
  // we represent a price as an integer number of ticks
  //
  // suppose this instrument has a tick size of $0.01.
  //
  // then: 10'001 ticks is $100.01 dollars.
  //
  // we use integers rather than doubles so that price comparisons are exact
  // "constexpr" means this value is known at compile time and cannot change

  constexpr Price example_price{10'001};

  // example quantity
  //
  // quantities will also be represented as integer units
  // we have NOT yet defined what one unit is.
  // eventually, an instrument specification might say:
  // minimum BTC quantity incrmeent = 0.00000001 BTC
  //
  // if so, mercury could represent quantities as integer multiples of that
  // minimum increment again, avoiding floating point arithmetic in critical
  // trading logic

  constexpr Quantity example_quantity{250};

  // example timestamp
  //
  // electronic markets are event-driven
  // events include:
  // order book updates
  // trades
  // cancellations
  // our own order submissions
  // our fills
  //
  // timing is very important. mercury will evenetually use nanosecond
  // resolution timestamps
  //
  // one nano second is 1x10^-6 seconds
  //
  // 1'000'000 ns is 1 ms
  //

  constexpr Timestamp example_timestamp{1'000'000};

  // example side
  //
  // an order book has 2 sides: buy -> buy side, ask -> sell side
  //
  // side is an enum class. that means mercury accepts explicit values such as
  //
  // Side::Ask
  // Side:Bid
  //
  // raher than arbitrary strings
  // strong typing prevents many classes of invalid states
  constexpr Side example_side{Side::Bid};

  // print program info
  //
  // std:cout is C++'s standard output stream
  //
  // the << operator sends data into that stream
  //
  // read:
  //
  // std::cout << "Hello";
  //
  // approximately as: send "Hello" to standard output
  // '\n' means newline

  std::cout << "Mercury Market Making Engine\n";

  // __cplusplus is a special macro provided by compiler. when compiling in
  // c++23 mode, we expect approx: 202320. confirmed this earlier
  std::cout << "C++ standard: " << __cplusplus << '\n';

  // print a blank line
  //
  std::cout << '\n';

  // print our example market values
  //
  std::cout << "Core market types loaded: \n";

  // python: print("price ticks:", example_price)
  // price is now a class than an integer
  // std::cout does not yet know how to print a Price object directly
  // we therefore call example_price.ticks()
  // which asks the Price object for its underlying integer tick count
  //  the "." operator accesses a public memebr of an object
  //
  std::cout << "Price ticks: " << example_price.ticks() << '\n';

  std::cout << "Quantity units: " << example_quantity << '\n';

  std::cout << "Timestamp ns: " << example_timestamp << '\n';

  // printing side
  //
  // std::cout currently does not know how we want a side displayed
  //
  // so for now, we manually convert:
  //
  // Side::Bid -> "Bid"
  // Side::Ask -> "Ask"
  //
  // this expression:
  // condition ? value_if_true : value_if_false
  // is called the ternary operator
  //
  // python equivalent
  //
  // "Bid" if example_side == Side.Bid else "Ask"
  //
  // c++:
  //
  // example_side == Side::Bid ? "Bid" : "Ask"
  // later we will replace this with a proper utility function

  // we will convert side into human readable text using our reusable helper
  // function

  std::cout << "Side: " << to_string(example_side) << '\n';

  std::cout << '\n';
  std::cout << "status: ready\n";

  // exit successufully, returning 0 tells the OS mercury exited successfully
  return 0;
}
