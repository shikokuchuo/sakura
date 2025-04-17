
<!-- README.md is generated from README.Rmd. Please edit that file -->

# sakura

<!-- badges: start -->

[![Lifecycle:
experimental](https://img.shields.io/badge/lifecycle-experimental-orange.svg)](https://lifecycle.r-lib.org/articles/stages.html#experimental)
[![CRAN
status](https://www.r-pkg.org/badges/version/sakura)](https://CRAN.R-project.org/package=sakura)
[![R-CMD-check](https://github.com/shikokuchuo/sakura/workflows/R-CMD-check/badge.svg)](https://github.com/shikokuchuo/sakura/actions)
[![Codecov test
coverage](https://codecov.io/gh/shikokuchuo/sakura/graph/badge.svg)](https://app.codecov.io/gh/shikokuchuo/sakura)
<!-- badges: end -->

      ________  
     /\ sa    \
    /  \  ku   \
    \  /    ra /
     \/_______/

### Extension to R Serialization

Extends the functionality of R serialization by augmenting the built-in
reference hook system. This enhanced implementation allows an integrated
single-pass operation that combines R serialization with third-party
serialization methods.

Facilitates the serialization of even complex R objects, which contain
non-system reference objects, such as those accessed via external
pointers, to enable their use in parallel and distributed computing.

This package was a request from a meeting of the [R
Consortium](https://r-consortium.org/) [Marshalling and Serialization
Working Group](https://github.com/RConsortium/marshalling-wg/) held at
useR!2024 in Salzburg, Austria. It is designed to eventually provide a
common framework for marshalling in R.

It extracts the functionality embedded within the
[mirai](https://github.com/r-lib/mirai) async framework for use in other
contexts.

### Installation

Install the current release from CRAN:

``` r
install.packages("sakura")
```

Or the development version using:

``` r
pak::pak("shikokuchuo/sakura")
```

### Overview

Some R objects by their nature cannot be serialized, such as those
accessed via an external pointer.

Using the [`arrow`](https://arrow.apache.org/docs/r/) package as an
example:

``` r
library(arrow, warn.conflicts = FALSE)
obj <- list(as_arrow_table(iris), as_arrow_table(mtcars))

unserialize(serialize(obj, NULL))
#> [[1]]
#> Table
#> Error: Invalid <Table>, external pointer to null
```

In such cases, `sakura::serial_config()` can be used to create custom
serialization configurations, specifying functions that hook into R’s
native serialization mechanism for reference objects (‘refhooks’).

``` r
cfg <- sakura::serial_config(
  "ArrowTabular",
  arrow::write_to_raw,
  function(x) arrow::read_ipc_stream(x, as_data_frame = FALSE)
)
```

This configuration can then be supplied as the ‘hook’ argument for
`sakura::serialize()` and `sakura::unserialize()`.

``` r
sakura::unserialize(sakura::serialize(obj, cfg), cfg)
#> [[1]]
#> Table
#> 150 rows x 5 columns
#> $Sepal.Length <double>
#> $Sepal.Width <double>
#> $Petal.Length <double>
#> $Petal.Width <double>
#> $Species <dictionary<values=string, indices=int8>>
#> 
#> See $metadata for additional Schema metadata
#> 
#> [[2]]
#> Table
#> 32 rows x 11 columns
#> $mpg <double>
#> $cyl <double>
#> $disp <double>
#> $hp <double>
#> $drat <double>
#> $wt <double>
#> $qsec <double>
#> $vs <double>
#> $am <double>
#> $gear <double>
#> $carb <double>
#> 
#> See $metadata for additional Schema metadata
```

This time, the arrow tables are handled seamlessly.

Using `torch` as another example:

``` r
library(torch)
x <- list(torch_rand(5L), runif(5L))

unserialize(serialize(x, NULL))
#> [[1]]
#> torch_tensor
#> Error in (function (self) : external pointer is not valid
```

Base R serialization above fails, but `sakura` serialization succeeds:

``` r
cfg <- sakura::serial_config("torch_tensor", torch::torch_serialize, torch::torch_load)

sakura::unserialize(sakura::serialize(x, cfg), cfg)
#> [[1]]
#> torch_tensor
#>  0.9885
#>  0.5773
#>  0.9892
#>  0.3081
#>  0.3537
#> [ CPUFloatType{5} ]
#> 
#> [[2]]
#> [1] 0.34333701 0.30562588 0.08964682 0.75173197 0.10512006
```

### C Interface

A C level interface is provided. A public header file `sakura.h` is
available in `inst/include` for all packages that declare sakura in
`LinkingTo`. This may be used in the following way:

``` c
#include <sakura.h>

sakura_sfunc sakura_serialize;
sakura_ufunc sakura_unserialize;

// runtime initialization:
sakura_serialize = (sakura_sfunc) R_GetCCallable("sakura", "sakura_serialize");
sakura_unserialize = (sakura_ufunc) R_GetCCallable("sakura", "sakura_unserialize");
```

### Acknowledgements

We would like to thank in particular:

- [R Core](https://www.r-project.org/contributors.html) for providing
  the interface to the R serialization mechanism.
- [Luke Tierney](https://github.com/ltierney/) and [Mike
  Cheng](https://github.com/coolbutuseless) for their meticulous efforts
  in documenting the serialization interface.
- [Daniel Falbel](https://github.com/dfalbel) for discussion around an
  efficient solution to serialization and transmission of torch tensors.

–

Please note that this project is released with a [Contributor Code of
Conduct](https://shikokuchuo.net/sakura/CODE_OF_CONDUCT.html). By
participating in this project you agree to abide by its terms.
