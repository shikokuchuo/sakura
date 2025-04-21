# sakura (development version)

* Optimised internal mechanism using inline serialization (@traversc and @shikokuchuo, #8).
* `serial_config()` improvements:
  + Accepts multiple custom serialization functions for different classes of object (#12).
  + Simplified by removing the `vec` argument, as no longer applicable due to inline serialization.
* Each custom-serialized object is no longer limited to a raw vector of length `INT_MAX`.
* Implements a C level interface by registering C function callables.
* Package is re-licensed under the MIT licence.

# sakura 0.1.0

* Initial CRAN release.

# sakura 0.0.1

* Initial release to Github and R-universe.
