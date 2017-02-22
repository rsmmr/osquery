/*
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <algorithm>
#include <iostream>
#include <list>

#include <broker/broker.hh>
#include <broker/endpoint.hh>
#include <broker/message_queue.hh>

#include <osquery/database.h>
#include <osquery/status.h>
#include <osquery/system.h>

#include "osquery/bro/QueryManager.h"

namespace osquery {

class BrokerManager {
 private:
  BrokerManager();

 public:
  // Get a singleton instance
  static BrokerManager* getInstance() {
    if (!_instance)
      _instance = new BrokerManager();
    return _instance;
  }

  // Topic Prefix
  const std::string TOPIC_PREFIX = "/bro/osquery/";
  const std::string TOPIC_ALL = TOPIC_PREFIX + "all";
  const std::string TOPIC_ANNOUNCE = TOPIC_PREFIX + "announce";
  const std::string TOPIC_PRE_INDIVIDUALS = TOPIC_PREFIX + "uid/";
  const std::string TOPIC_PRE_GROUPS = TOPIC_PREFIX + "group/";
  const std::string TOPIC_PRE_CUSTOMS = TOPIC_PREFIX + "custom/";

  // Event messages
  const std::string EVENT_HOST_NEW = "osquery::host_new";
  const std::string EVENT_HOST_JOIN = "osquery::host_join";
  const std::string EVENT_HOST_LEAVE = "osquery::host_leave";
  const std::string EVENT_HOST_EXECUTE = "osquery::host_execute";
  const std::string EVENT_HOST_SUBSCRIBE = "osquery::host_subscribe";
  const std::string EVENT_HOST_UNSUBSCRIBE = "osquery::host_unsubscribe";

  Status setNodeID(const std::string& uid);

  std::string getNodeID();

  Status addGroup(const std::string& group);

  Status removeGroup(const std::string& group);

  std::vector<std::string> getGroups();

  Status createEndpoint(const std::string& ep_name);

  broker::endpoint* getEndpoint();

  Status createMessageQueue(const std::string& topic);

  Status deleteMessageQueue(const std::string& topic);

  broker::message_queue* getMessageQueue(const std::string& topic);

  Status getTopics(std::vector<std::string>& topics);

  Status peerEndpoint(const std::string& ip, int port);

  Status logQueryLogItemToBro(const QueryLogItem& qli);

  Status sendEvent(const std::string& topic, const broker::message& msg);

 private:
  // The singleton object
  static BrokerManager* _instance;

  QueryManager* qm = nullptr;

  // The ID identifying the node (private channel)
  std::string nodeID = "";
  // The groups of the node
  std::vector<std::string> groups;
  // The Broker Endpoint
  broker::endpoint* ep = nullptr; // delete afterwards

  //  Key: topic_Name, Value: message_queue
  std::map<std::string, broker::message_queue*> messageQueues;
};
}
