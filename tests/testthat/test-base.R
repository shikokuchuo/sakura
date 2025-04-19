test_that("sakura without refhooks work", {
  df <- data.frame(a = 1, b = "a")
  expect_type(serialize(df), "raw")
  expect_s3_class(unserialize(serialize(df)), "data.frame")
  expect_identical(unserialize(serialize(df)), df)
  expect_error(unserialize(raw(1)))
})

test_that("sakura mock refhooks work", {
  cfg <- serial_config(
    class = "test_klass",
    sfunc = function(x) base::serialize(x, NULL),
    ufunc = base::unserialize
  )
  obj <- list(a = new.env(), b = new.env(), vector = runif(10L))
  class(obj$b) <- "test_klass"
  vec <- serialize(obj, cfg)
  expect_type(cfg, "pairlist")
  expect_type(vec, "raw")
  expect_true(all.equal(unserialize(vec, cfg), obj))
})
