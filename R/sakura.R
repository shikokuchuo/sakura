# sakura -----------------------------------------------------------------------

#' sakura: Extension to R Serialization
#'
#' Extends the functionality of R serialization by augmenting the built-in
#' reference hook system. This enhanced implementation allows an integrated
#' single-pass operation that combines R serialization with third-party
#' serialization methods. Facilitates the serialization of even complex R
#' objects, which contain non-system reference objects, such as those accessed
#' via external pointers, to enable their use in parallel and distributed
#' computing.
#'
#' @useDynLib sakura, .registration = TRUE
#'
"_PACKAGE"

#' Serialize
#'
#' An extension of R native serialization using the 'refhook' system for custom
#' serialization and unserialization of non-system reference objects.
#'
#' @param x an object.
#' @param hook \[default NULL\] optionally, a configuration returned by
#'   [serial_config()].
#'
#' @return For serialize: a raw vector. For unserialize: the unserialized object.
#'
#' @examples
#' vec <- serialize(data.frame())
#' vec
#' unserialize(vec)
#'
#' @examplesIf requireNamespace("arrow", quietly = TRUE)
#' obj <- list(arrow::as_arrow_table(iris), arrow::as_arrow_table(mtcars))
#' cfg <- serial_config(
#'   "ArrowTabular",
#'   arrow::write_to_raw,
#'   function(x) arrow::read_ipc_stream(x, as_data_frame = FALSE)
#' )
#' raw <- serialize(obj, cfg)
#' unserialize(raw, cfg)
#'
#' @examplesIf requireNamespace("torch", quietly = TRUE)
#' x <- list(torch::torch_rand(5L), runif(5L))
#' cfg <- serial_config("torch_tensor", torch::torch_serialize, torch::torch_load)
#' unserialize(serialize(x, cfg), cfg)
#'
#' @export
#'
serialize <- function(x, hook = NULL)
  .Call(sakura_r_serialize, x, hook)

#' @examplesIf requireNamespace("arrow", quietly = TRUE) && requireNamespace("torch", quietly = TRUE)
#' cfg <- serial_config(
#'   c("torch_tensor", "ArrowTabular"),
#'   list(torch::torch_serialize, arrow::write_to_raw),
#'   list(torch::torch_load, function(x) arrow::read_ipc_stream(x, as_data_frame = FALSE))
#' )
#' y <- list(torch::torch_rand(5L), runif(5L), arrow::as_arrow_table(iris))
#' unserialize(serialize(y, cfg), cfg)
#'
#' @rdname serialize
#' @export
#'
unserialize <- function(x, hook = NULL)
  .Call(sakura_r_unserialize, x, hook)

#' Create Serialization Configuration
#'
#' Returns a serialization configuration for custom serialization and
#' unserialization of non-system reference objects, using the 'refhook' system
#' of R native serialization. This allows their use across different R sessions.
#'
#' @param class a character string (or vector) of the class of object custom
#'   serialization functions are applied to, e.g. `'ArrowTabular'` or
#'   `c('torch_tensor', 'ArrowTabular')`.
#' @param sfunc a function (or list of functions) that accepts a reference
#'   object inheriting from `class` and returns a raw vector.
#' @param ufunc a function (or list of functions) that accepts a raw vector and
#'   returns a reference object.
#'
#' @return A pairlist comprising the configuration. This may be provided to the
#'   `hook` argument of [serialize()] and [unserialize()].
#'
#' @examples
#' serial_config("test_class", base::serialize, base::unserialize)
#'
#' serial_config(
#'   c("class_one", "class_two"),
#'   list(base::serialize, base::serialize),
#'   list(base::unserialize, base::unserialize)
#' )
#'
#' @export
#'
serial_config <- function(class, sfunc, ufunc) {

  is.character(class) ||
    stop("`class` must be a character string")
  if (is.list(sfunc)) {
    all(as.logical(lapply(sfunc, is.function))) && all(as.logical(lapply(ufunc, is.function)))
  } else {
    is.function(sfunc) && is.function(ufunc)
  } || stop("both `sfunc` and `ufunc` must be functions")
  length(class) == length(sfunc) && length(sfunc) == length(ufunc) ||
    stop("`class`, `sfunc` and `ufunc` must be the same length")

  pairlist(class, sfunc, ufunc)

}
