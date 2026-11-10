#include "hpx_runtime.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include <hpx/hpx_init.hpp>
#include <zpp/spdlog.h>

namespace z::app {

err_t hpx_runtime::run(int argc, char **argv,
                       const std::function<err_t()> &entry) {
  if (used_) {
    return ERR_EXIST;
  }
  used_ = true;
  if (argc < 1 || argv == nullptr || !entry) {
    return ERR_PARAM_INVALID;
  }

  // HPX silently tolerates a missing INI file. An explicitly selected file is
  // required here so that a typo cannot start zgent with unexpected defaults.
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    constexpr std::string_view prefix{"--hpx:config="};
    std::string_view path;
    if (argument.starts_with(prefix)) {
      path = argument.substr(prefix.size());
    } else if (argument == "--hpx:config") {
      if (++index == argc) {
        spd_err("[hpx:configure] --hpx:config requires a file");
        return ERR_PARAM_INVALID;
      }
      path = argv[index];
    } else {
      continue;
    }
    const std::string filename{path};
    std::error_code error;
    if (path.empty() || !std::filesystem::is_regular_file(filename, error) ||
        !std::ifstream(filename).good()) {
      spd_err("[hpx:configure] cannot read INI file: {}", path);
      return ERR_PARAM_INVALID;
    }
  }

  ::hpx::init_params params;
  // z::server owns SIGINT/SIGTERM handling. HPX's SIGINT handler aborts.
  params.cfg = {"hpx.handle_signals!=0"};
  try {
    const auto result = ::hpx::init(
        [&entry](int, char **) -> int {
          err_t result = ERR_OK;
          try {
            result = entry();
          } catch (const std::exception &error) {
            spd_err("[hpx:entry] {}", error.what());
            result = ERR_FAIL;
          } catch (...) {
            spd_err("[hpx:entry] unknown exception");
            result = ERR_FAIL;
          }

          const int finalize_result = ::hpx::finalize();
          return result == ERR_OK ? finalize_result : result;
        },
        argc, argv, params);
    spd_inf("[hpx:run] returned, result={}", result);
    return result;
  } catch (const std::exception &error) {
    spd_err("[hpx:run] {}", error.what());
  } catch (...) {
    spd_err("[hpx:run] unknown exception");
  }
  return ERR_FAIL;
}

} // namespace z::app
