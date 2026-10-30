/**
 * @file app.cpp
 * @brief zgent application server implementation.
 */
#include "app.h"

#include <zpp/error.h>
#include <zpp/spdlog.h>

USE_APP

server::server(int argc, char **argv) : z::server(argc, argv) {}
server::~server() {}

z::err_t server::run() {
  spd_inf("zgent server running...");
  return ERR_OK;
}

z::err_t server::stop() {
  spd_inf("zgent server stopping...");
  return z::server::stop();
}
