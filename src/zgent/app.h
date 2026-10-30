/**
 * @file app.h
 * @brief zgent application server declaration.
 */
#pragma once

#include <zpp/core/server.h>
#include <zpp/namespace.h>

NSB_APP

/**
 * @class server
 * @brief zgent application server, driven by the project main entry.
 *
 * Constructed by the main entry with the uri-style arguments
 * `<executable> <uni-conf.json> <conf-server-item>`.
 */
class server : public z::server {
public:
  server(int argc, char **argv);
  ~server() override;

  z::err_t run() override;
  z::err_t stop() override;
};
NSE_APP
