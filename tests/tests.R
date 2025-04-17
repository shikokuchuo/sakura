# minitest - a minimal testing framework v0.0.2 --------------------------------
test_library <- function(package) library(package = package, character.only = TRUE)
test_true <- function(x) invisible(isTRUE(x) || {print(x); stop("the above was returned instead of TRUE")})
test_null <- function(x) invisible(is.null(x) || {print(x); stop("the above was returned instead of NULL")})
test_notnull <- function(x) invisible(!is.null(x) || stop("returns NULL when expected to be not NULL"))
test_zero <- function(x) invisible(x == 0L || {print(x); stop("the above was returned instead of 0L")})
test_type <- function(type, x) invisible(typeof(x) == type || {stop("object of type '", typeof(x), "' was returned instead of '", type, "'")})
test_class <- function(class, x) invisible(inherits(x, class) || {stop("object of class '", paste(class(x), collapse = ", "), "' was returned instead of '", class, "'")})
test_equal <- function(a, b) invisible(a == b || {print(a); print(b); stop("the above expressions were not equal")})
test_identical <- function(a, b) invisible(identical(a, b) || {print(a); print(b); stop("the above expressions were not identical")})
test_print <- function(x) invisible(is.character(capture.output(print(x))) || stop("print output of expression cannot be captured as a character value"))
test_error <- function(x, containing = "") invisible(inherits(x <- tryCatch(x, error = identity), "error") && grepl(containing, x[["message"]], fixed = TRUE) || stop("Expected error message containing: ", containing, "\nActual error message: ", x[["message"]]))
# ------------------------------------------------------------------------------

test_library("sakura")
df <- data.frame(a = 1, b = "a")
test_type("raw", serialize(df))
test_class("data.frame", unserialize(serialize(df)))
test_identical(unserialize(serialize(df)), df)
test_error(unserialize(raw(1)))

if (requireNamespace("torch", quietly = TRUE)) {
  cfg <- serial_config(
    class = "torch_tensor",
    sfunc = torch::torch_serialize,
    ufunc = torch::torch_load
  )
  test_type("pairlist", cfg)
  obj <- list(a = torch::torch_rand(1e3), b = new.env(), vector = runif(1000L))
  vec <- serialize(obj, cfg)
  test_type("raw", vec)
  test_true(all.equal(unserialize(vec, cfg), obj))
}

if (requireNamespace("arrow", quietly = TRUE)) {
  cfga <- serial_config(
    "ArrowTabular",
    arrow::write_to_raw,
    function(x) arrow::read_ipc_stream(x, as_data_frame = FALSE)
  )
  test_type("pairlist", cfga)
  test_type("raw", serialize(obj, cfga))
  x <- list(
    a = arrow::as_arrow_table(iris),
    b = arrow::as_arrow_table(mtcars)
  )
  vec <- serialize(x, cfga)
  test_type("raw", vec)
  test_true(all.equal(unserialize(vec, cfga), x))
}
