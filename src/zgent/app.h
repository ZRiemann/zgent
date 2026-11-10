/**
 * @file app.h
 * @brief zgent application server declaration.
 */
#pragma once

#include "hpx_runtime.h"

#include <optional>

#include <zpp/core/server.h>
#include <zpp/namespace.h>
#include <zpp/nng/nng.h>

NSB_APP

/**
 * @class server
 * @brief Application server composing NNG communication and HPX business work.
 *
 * Construct with `<executable> <conf.json> <service> [HPX options]`.
 * configure() validates NNG settings; run() blocks through the complete
 * runtime lifetime. Call both from the native main thread, once per process.
 */
class server : public z::server {
public:
  /** @brief Initialize the shared server's configuration and logging. */
  server(int argc, char **argv);
  /** @brief Release application resources before the shared logger. */
  ~server() override;

  server(const server &) = delete;
  server &operator=(const server &) = delete;
  server(server &&) = delete;
  server &operator=(server &&) = delete;

  /** @brief Validate the selected service and NNG initialization settings. */
  z::err_t configure() override;
  /** @brief Block through NNG/HPX execution, shutdown, and resource cleanup. */
  z::err_t run() override;
  /** @brief Stop communication producers on the HPX lifecycle task. */
  z::err_t stop() override;
  /** @brief Drain application work and callbacks before HPX finalization. */
  z::err_t wait_stop() override;
  /** @brief Suspend the HPX lifecycle task for one cooperative timer tick. */
  z::err_t timer() override;
  /** @brief Keep the service alive after a successful timer tick. */
  z::err_t on_timer() override;

private:
  z::err_t run_business();

  nng_init_params nng_params_{};
  std::optional<z::nng::nng> nng_runtime_;
  hpx_runtime hpx_runtime_;
  bool configured_{false};
  bool run_attempted_{false};
};
NSE_APP
