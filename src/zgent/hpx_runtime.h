#pragma once

#include <functional>

#include <zpp/error.h>

namespace z::app {

/**
 * @brief Application-local owner of one blocking HPX runtime invocation.
 *
 * The caller and its captured objects must outlive run(). No global hpx_main
 * or application-owned worker thread is required.
 */
class hpx_runtime {
public:
  /** @brief Construct an unused runtime owner. */
  hpx_runtime() = default;

  hpx_runtime(const hpx_runtime &) = delete;
  hpx_runtime &operator=(const hpx_runtime &) = delete;
  hpx_runtime(hpx_runtime &&) = delete;
  hpx_runtime &operator=(hpx_runtime &&) = delete;

  /**
   * @brief Execute entry as an HPX task, finalize HPX, and return its result.
   * @param argc Process argument count, including HPX options.
   * @param argv Process arguments, valid until this call returns.
   * @param entry Application lifecycle including its own cleanup.
   * @return The first application error, or the HPX initialization/finalization
   * result. A second invocation returns ERR_EXIST.
   * @note Call from the native main thread; concurrent calls are unsupported.
   */
  err_t run(int argc, char **argv, const std::function<err_t()> &entry);

private:
  bool used_{false};
};

} // namespace z::app
