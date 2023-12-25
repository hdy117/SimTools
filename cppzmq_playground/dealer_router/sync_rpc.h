#pragma once

#include "sim.h"

#include "message/location.pb.h"
#include "message/basic.pb.h"
#include "message/trajectory.pb.h"
#include "zmq.hpp"

#include <string>

namespace topic {
const std::string LOCATION("LOCATION");
const std::string TRAJECTORY("TRAJECTORY");
} // namespace topic