# sakura (development version)

* Length of custom-serialized raw vector is no longer limited to `INT_MAX`.
* `serial_config()` can now be used to specify multiple custom serialization functions for different classes of object (#12).
* Updates the internal mechanism to use inline serialization (@traversc and @shikokuchuo, #8).
* `serial_config()` is simplified by removing the `vec` argument, as this is no longer relevant after the adoption of inline serialization.
* Implements a C level interface with public declarations in `inst/include/sakura.h`.
* Package is re-licensed under the MIT licence.

# sakura 0.1.0

* Initial CRAN release.

# sakura 0.0.1

* Initial release to Github and R-universe.
