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
#' @encoding UTF-8
#' @author Charlie Gao \email{charlie.gao@@shikokuchuo.net}
#'   (\href{https://orcid.org/0000-0002-0750-061X}{ORCID})
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
#' @param hook [default NULL] optionally, a configuration returned by
#'   \code{\link{serial_config}}.
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
#' @export
#'
serialize <- function(x, hook = NULL) .Call(sakura_r_serialize, x, hook)

#' @rdname serialize
#' @export
#'
unserialize <- function(x, hook = NULL) .Call(sakura_r_unserialize, x, hook)

#' Create Serialization Configuration
#'
#' Returns a serialization configuration for custom serialization and
#' unserialization of non-system reference objects, using the 'refhook' system
#' of R native serialization. This allows their use across different R sessions.
#'
#' @param class character string of the class of object custom serialization
#'   functions are applied to, e.g. \sQuote{ArrowTabular} or
#'   \sQuote{torch_tensor}.
#' @param sfunc a function that accepts a reference object inheriting from
#'   \sQuote{class} (or a list of such objects) and returns a raw vector.
#' @param ufunc a function that accepts a raw vector and returns a reference
#'   object (or list of such objects).
#' @param vec [default FALSE] whether or not the serialization functions are
#'   vectorized. If FALSE, they should accept and return reference objects
#'   individually e.g. \code{arrow::write_to_raw} and
#'   \code{arrow::read_ipc_stream}. If TRUE, they should accept and return a
#'   list of reference objects, e.g. \code{torch::torch_serialize} and
#'   \code{torch::torch_load}.
#'
#' @return A pairlist comprising the configuration. This may be provided to the
#'   'hook' argument of \code{\link{serialize}} and \code{\link{unserialize}}.
#'
#' @examples
#' serial_config("test_class", base::serialize, base::unserialize)
#'
#' @export
#'
serial_config <- function(class, sfunc, ufunc, vec = FALSE) {

  is.character(class) ||
    stop("'class' must be a character string")
  is.function(sfunc) && is.function(ufunc) ||
    stop("both 'sfunc' and 'ufunc' must be functions")
  is.logical(vec) ||
    stop("'vec' must be a logical value")

  pairlist(class, sfunc, ufunc, isTRUE(vec))

}
