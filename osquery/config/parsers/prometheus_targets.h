/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under both the Apache 2.0 license (found in the
 *  LICENSE file in the root directory of this source tree) and the GPLv2 (found
 *  in the COPYING file in the root directory of this source tree).
 *  You may select, at your option, one of the above-listed licenses.
 */
#pragma once

#include <string>
#include <vector>

#include <osquery/config.h>

namespace osquery {

const std::string kConfigParserRootKey("prometheus_targets");

class PrometheusMetricsConfigParserPlugin : public ConfigParserPlugin {
 public:
  std::vector<std::string> keys() const override;
  Status setUp() override;
  Status update(const std::string& source, const ParserConfig& config) override;
};
}
