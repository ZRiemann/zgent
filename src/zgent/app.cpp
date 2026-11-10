/**
 * @file app.cpp
 * @brief zgent application server implementation.
 */
#include "app.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>

#include <hpx/modules/runtime_local.hpp>
#include <hpx/thread.hpp>
#include <zpp/error.h>
#include <zpp/json.hpp>
#include <zpp/spdlog.h>

namespace {

// This is the application lifecycle boundary: each phase may fail
// independently, but a failed loop or stop must not skip subsequent cleanup.
z::err_t invoke_phase(const char *name,
                      const std::function<z::err_t()> &action) {
  try {
    const auto result = action();
    if (result != z::ERR_OK) {
      spd_err("[zgent:{}] failed, result={}", name, result);
    }
    return result;
  } catch (const std::exception &error) {
    spd_err("[zgent:{}] {}", name, error.what());
  } catch (...) {
    spd_err("[zgent:{}] unknown exception", name);
  }
  return z::ERR_FAIL;
}

} // namespace

USE_APP

server::server(int argc, char **argv) : z::server(argc, argv) {}
server::~server() = default;

z::err_t server::configure() {
  if (run_attempted_) {
    return ERR_EXIST;
  }
  configured_ = false;
  if (argc_ < 3 || argv_ == nullptr) {
    return ERR_PARAM_INVALID;
  }

  z::json conf;
  z::json_view service;
  if (conf.load_file(argv_[1]) != ERR_OK ||
      conf.member(argv_[2], service) != ERR_OK || !service.is_object()) {
    spd_err("[zgent:configure] invalid service configuration: {}", argv_[2]);
    return ERR_PARAM_INVALID;
  }

  nng_init_params params{};
  z::json_view runtime;
  const auto runtime_result = service.member("nng_runtime", runtime);
  if (runtime_result != ERR_NOT_EXIST) {
    if (runtime_result != ERR_OK || !runtime.is_object()) {
      spd_err("[zgent:configure] {}.nng_runtime must be an object", argv_[2]);
      return ERR_PARAM_INVALID;
    }

    /** @brief Map one JSON setting to its checked NNG initialization field. */
    struct runtime_field {
      const char *name;
      std::int16_t nng_init_params::*member;
      int minimum;
    };
    const runtime_field fields[] = {
        {"task_threads", &nng_init_params::num_task_threads, 0},
        {"task_threads_max", &nng_init_params::max_task_threads, -1},
        {"expire_threads", &nng_init_params::num_expire_threads, 0},
        {"expire_threads_max", &nng_init_params::max_expire_threads, -1},
        {"poller_threads", &nng_init_params::num_poller_threads, 0},
        {"poller_threads_max", &nng_init_params::max_poller_threads, -1},
        {"resolver_threads", &nng_init_params::num_resolver_threads, 0},
    };
    for (const auto &field : fields) {
      z::json_view value;
      if (runtime.member(field.name, value) == ERR_NOT_EXIST) {
        continue;
      }
      int number = 0;
      if (value.get(number) != ERR_OK || number < field.minimum ||
          number > std::numeric_limits<std::int16_t>::max()) {
        spd_err("[zgent:configure] {}.nng_runtime.{} must be an integer in "
                "[{}, {}]",
                argv_[2], field.name, field.minimum,
                std::numeric_limits<std::int16_t>::max());
        return ERR_PARAM_INVALID;
      }
      params.*(field.member) = static_cast<std::int16_t>(number);
    }
  }

  nng_params_ = params;
  configured_ = true;
  return ERR_OK;
}

z::err_t server::run() {
  if (!configured_) {
    return ERR_PARAM_INVALID;
  }
  if (run_attempted_) {
    return ERR_EXIST;
  }
  run_attempted_ = true;

  const auto result = invoke_phase("run", [this] {
    nng_runtime_.emplace(nng_params_);
    return hpx_runtime_.run(argc_, argv_, [this] { return run_business(); });
  });
  // HPX has returned; no task may still use NNG or capture this server.
  nng_runtime_.reset();
  spd_inf("[zgent:run] runtimes stopped, result={}", result);
  return result;
}

z::err_t server::run_business() {
  auto result = invoke_phase("loop", [this] {
    spd_inf("[zgent:run] ready, hpx_workers={}", ::hpx::get_os_thread_count());
    loop();
    return ERR_OK;
  });
  const auto stop_result = invoke_phase("stop", [this] { return stop(); });
  if (result == ERR_OK) {
    result = stop_result;
  }
  const auto wait_result =
      invoke_phase("wait_stop", [this] { return wait_stop(); });
  return result == ERR_OK ? wait_result : result;
}

z::err_t server::stop() {
  spd_inf("[zgent:stop] stopping communication producers");
  return z::server::stop();
}

z::err_t server::wait_stop() {
  spd_inf("[zgent:wait_stop] draining application work");
  return z::server::wait_stop();
}

z::err_t server::timer() {
  ::hpx::this_thread::sleep_for(std::chrono::milliseconds{100});
  return ERR_OK;
}

z::err_t server::on_timer() { return ERR_OK; }
