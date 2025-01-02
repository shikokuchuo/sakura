
<!-- README.md is generated from README.Rmd. Please edit that file -->

# sakura

<!-- badges: start -->

[![R-universe
status](https://shikokuchuo.r-universe.dev/badges/sakura)](https://shikokuchuo.r-universe.dev/sakura)
[![R-CMD-check](https://github.com/shikokuchuo/sakura/workflows/R-CMD-check/badge.svg)](https://github.com/shikokuchuo/sakura/actions)
<!-- badges: end -->

      ________  
     /\ sa    \
    /  \  ku   \
    \  /    ra /
     \/_______/

### Extension to R Serialization

An extension of R native serialization using the ‘refhook’ system for
custom serialization and unserialization of non-system reference
objects.

Some R objects by their nature cannot be serialized, such as those
accessed via an external pointer.

Using the [`arrow`](https://arrow.apache.org/docs/r/) package as an
example:

``` r
library(arrow, warn.conflicts = FALSE)
x <- list(as_arrow_table(iris), as_arrow_table(mtcars))

unserialize(serialize(x, NULL))
#> [[1]]
#> Table
#> Error: Invalid <Table>, external pointer to null
```

In such cases, `sakura::serial_config()` can be used to create custom
serialization configurations, specifying functions that hook into R’s
native serialization mechanism for reference objects (‘refhooks’).

``` r
cfg <- sakura::serial_config(
  class = "ArrowTabular",
  sfunc = arrow::write_to_raw,
  ufunc = function(x) arrow::read_ipc_stream(x, as_data_frame = FALSE)
)
```

This configuration can then be supplied as the ‘hook’ argument for
`sakura::serialize()` and `sakura::unserialize()`.

``` r
sakura::unserialize(sakura::serialize(x, cfg), cfg)
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

Other types of serialization function are vectorized and in this case,
the configuration should be created specifying `vec = TRUE`. Using
`torch` as an example:

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
cfg <- sakura::serial_config(
  class = "torch_tensor",
  sfunc = torch::torch_serialize,
  ufunc = torch::torch_load,
  vec = TRUE
)

sakura::unserialize(sakura::serialize(x, cfg), cfg)
#> [[1]]
#> torch_tensor
#>  0.1275
#>  0.2434
#>  0.4176
#>  0.4934
#>  0.0567
#> [ CPUFloatType{5} ]
#> 
#> [[2]]
#> [1] 0.99648417 0.03548333 0.05227065 0.28857983 0.04848588
```

### Installation

The current development version is available from R-universe:

``` r
install.packages("sakura", repos = "https://shikokuchuo.r-universe.dev")
```

–

Please note that this project is released with a [Contributor Code of
Conduct](https://shikokuchuo.net/sakura/CODE_OF_CONDUCT.html). By
participating in this project you agree to abide by its terms.
