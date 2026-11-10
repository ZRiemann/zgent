#include "app.h"

#include <zpp/error.h>

int main(int argc, char **argv) {
  z::app::server application(argc, argv);
  const auto result = application.configure();
  return result == z::ERR_OK ? application.run() : result;
}
