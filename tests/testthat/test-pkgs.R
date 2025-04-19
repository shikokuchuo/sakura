skip_on_cran()

test_that("sakura torch refhooks work", {
  skip_if_not_installed("torch")
  cfg <- serial_config(
    class = "torch_tensor",
    sfunc = torch::torch_serialize,
    ufunc = torch::torch_load
  )
  obj <- list(a = torch::torch_rand(1e3L), b = new.env(), vector = runif(1000L))
  vec <- serialize(obj, cfg)
  expect_type(cfg, "pairlist")
  expect_type(vec, "raw")
  expect_true(all.equal(unserialize(vec, cfg), obj))
})

test_that("sakura arrow refhooks work", {
  skip_if_not_installed("arrow")
  cfg <- serial_config(
    "ArrowTabular",
    arrow::write_to_raw,
    function(x) arrow::read_ipc_stream(x, as_data_frame = FALSE)
  )
  obj <- list(
    a = arrow::as_arrow_table(iris),
    b = arrow::as_arrow_table(mtcars)
  )
  vec <- serialize(obj, cfg)
  expect_type(cfg, "pairlist")
  expect_type(vec, "raw")
  expect_true(all.equal(unserialize(vec, cfg), obj))
})

test_that("sakura multiple refhooks work", {
  skip_if_not_installed("torch")
  skip_if_not_installed("arrow")
  cfg <- serial_config(
    c("torch_tensor", "ArrowTabular"),
    list(torch::torch_serialize, arrow::write_to_raw),
    list(torch::torch_load, function(x) arrow::read_ipc_stream(x, as_data_frame = FALSE))
  )
  obj <- list(
    a = arrow::as_arrow_table(iris),
    b = torch::torch_rand(1e3L),
    c = new.env(),
    runif(5L)
  )
  vec <- serialize(obj, cfg)
  expect_type(cfg, "pairlist")
  expect_type(vec, "raw")
  expect_true(all.equal(unserialize(vec, cfg), obj))
})
