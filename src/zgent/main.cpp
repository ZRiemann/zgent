#include "app.h"

#include <memory>

#include <zpp/error.h>

int main(int argc, char **argv) {
  z::err_t err{z::ERR_FAIL};
  auto server_ptr = std::make_unique<z::app::server>(argc, argv);
  do {
    if (z::ERR_OK != (err = server_ptr->configure()))
      break;
    if (z::ERR_OK != (err = server_ptr->run()))
      break;
    server_ptr->loop();
    server_ptr->stop();
    server_ptr->wait_stop();
    err = z::ERR_OK;
  } while (0);
  return err;
}
