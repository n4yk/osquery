/*
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <vector>
#include <string>

#include <osquery/core.h>
#include <osquery/logger.h>
#include <osquery/tables.h>

#include "osquery/events/linux/udev.h"

namespace osquery {

/**
 * @brief Track udev events in Linux
 */
class HardwareEventSubscriber : public EventSubscriber<UdevEventPublisher> {
 public:
  Status init() override;

  Status Callback(const ECRef& ec, const SCRef& sc);
};

REGISTER(HardwareEventSubscriber, "event_subscriber", "hardware_events");

Status HardwareEventSubscriber::init() {
  auto subscription = createSubscriptionContext();
  subscription->action = UDEV_EVENT_ACTION_ALL;

  subscribe(&HardwareEventSubscriber::Callback, subscription);
  return Status(0, "OK");
}

Status HardwareEventSubscriber::Callback(const ECRef& ec, const SCRef& sc) {
  Row r;

  if (ec->devtype.empty()) {
    // Superfluous hardware event.
    return Status(0, "Missing type.");
  } else if (ec->devnode.empty() && ec->driver.empty()) {
    return Status(0, "Missing node and driver.");
  }

  struct udev_device* device = ec->device;
  r["action"] = ec->action_string;
  r["path"] = ec->devnode;
  r["type"] = ec->devtype;
  r["driver"] = ec->driver;

  // UDEV properties.
  r["model"] = UdevEventPublisher::getValue(device, "ID_MODEL_FROM_DATABASE");
  if (r["path"].empty() && r["model"].empty()) {
    // Don't emit mising path/model combos.
    return Status(0, "Missing path and model.");
  }

  r["model_id"] = INTEGER(UdevEventPublisher::getValue(device, "ID_MODEL_ID"));
  r["vendor"] = UdevEventPublisher::getValue(device, "ID_VENDOR_FROM_DATABASE");
  r["vendor_id"] =
      INTEGER(UdevEventPublisher::getValue(device, "ID_VENDOR_ID"));
  r["serial"] =
      INTEGER(UdevEventPublisher::getValue(device, "ID_SERIAL_SHORT"));
  r["revision"] = INTEGER(UdevEventPublisher::getValue(device, "ID_REVISION"));
  add(r);
  return Status(0, "OK");
}
}
