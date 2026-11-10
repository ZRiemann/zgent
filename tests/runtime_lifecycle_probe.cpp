#include "app.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <zpp/error.h>

static_assert(std::is_base_of_v<z::server, z::app::server>);
static_assert(!std::is_copy_constructible_v<z::app::server>);
static_assert(!std::is_move_constructible_v<z::app::server>);
static_assert(!std::is_copy_constructible_v<z::app::hpx_runtime>);
static_assert(!std::is_move_constructible_v<z::app::hpx_runtime>);

namespace {

/** @brief Exercises application cleanup failures on the real HPX runtime. */
class lifecycle_probe : public z::app::server {
public:
  /** @brief Construct a probe with a process-local failure scenario. */
  lifecycle_probe(int argc, char **argv, const std::string &scenario)
      : server(argc, argv), scenario_(scenario) {}

  /** @brief Finish after one tick or simulate an exceptional business task. */
  z::err_t on_timer() override {
    if (scenario_ == "loop_exception" ||
        scenario_ == "loop_and_cleanup_errors") {
      throw std::runtime_error("business failure");
    }
    return z::ERR_END;
  }

  /** @brief Report stop order and simulate an independent stop failure. */
  z::err_t stop() override {
    std::cout << "probe:stop\n" << std::flush;
    if (scenario_ == "stop_exception") {
      throw std::runtime_error("stop failure");
    }
    if (scenario_ == "stop_error" || scenario_ == "both_errors" ||
        scenario_ == "loop_and_cleanup_errors") {
      return z::ERR_TIMEOUT;
    }
    return server::stop();
  }

  /** @brief Report drain order and simulate an independent drain failure. */
  z::err_t wait_stop() override {
    std::cout << "probe:wait_stop\n" << std::flush;
    if (scenario_ == "wait_exception") {
      throw std::runtime_error("drain failure");
    }
    if (scenario_ == "wait_error" || scenario_ == "both_errors" ||
        scenario_ == "loop_and_cleanup_errors") {
      return z::ERR_AGAIN;
    }
    return server::wait_stop();
  }

private:
  std::string scenario_;
};

} // namespace

int main(int argc, char **argv) {
  if (argc < 4) {
    return 2;
  }
  const std::string scenario{argv[argc - 1]};
  lifecycle_probe application(argc, argv, scenario);
  const auto configured = application.configure();
  if (configured != z::ERR_OK || scenario == "configure") {
    return configured;
  }
  const auto result = application.run();
  std::cout << "probe:result=" << result << '\n' << std::flush;
  if (scenario == "single_run" && application.run() != z::ERR_EXIST) {
    return 3;
  }
  return result;
}
