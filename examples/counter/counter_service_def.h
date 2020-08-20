#pragma once

#include "rpc_server.h"

using namespace rest_rpc;
using namespace rpc_service;

namespace examples {
namespace counter {

struct CounterService {
  int Incr(rpc_conn, int delta) {
    
  }
};

}
}