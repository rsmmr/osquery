/*
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <poll.h>

#include <gtest/gtest.h>

#include <broker/broker.hh>
#include <broker/endpoint.hh>
#include <broker/message_queue.hh>

#include <osquery/flags.h>
#include <osquery/logger.h>

#include "osquery/bro/BrokerManager.h"

DECLARE_string(bro_ip);
DECLARE_uint64(bro_port);
DECLARE_string(bro_groups);
DECLARE_bool(disable_bro);

namespace osquery {

class BrokerManagerTests : public testing::Test {
 public:
  BrokerManagerTests() {
    broker::init();
    BrokerManager::get().reset();
  }

 protected:
  void SetUp() {
    Flag::updateValue("disable_bro", "false");

    //

    ep_ = std::make_unique<broker::endpoint>("brokermanager_test");
  }

  void TearDown() {}

 protected:
  BrokerManager& get() {
    return BrokerManager::get();
  }

 protected:
  std::unique_ptr<broker::endpoint> ep_ = nullptr;
};

TEST_F(BrokerManagerTests, test_failestablishconnection) {
  // NOT preparing receiver

  // Connect the broker endpoint
  EXPECT_TRUE(get().setNodeID("test_failestablishconnection").ok());
  EXPECT_TRUE(get().createEndpoint("test_failestablishconnection").ok());
  auto s_peer = get().peerEndpoint("127.0.0.1", 9999);

  EXPECT_FALSE(s_peer.ok());
}

TEST_F(BrokerManagerTests, test_establishconnection) {
  // Prepare receiver
  ep_->listen(9999, "127.0.0.1");
  broker::message_queue test_queue(get().TOPIC_ANNOUNCE, *ep_);

  // Add groups
  EXPECT_TRUE(get().addGroup("test1").ok());
  EXPECT_TRUE(get().addGroup("test2").ok());

  // Connect the broker endpoint
  EXPECT_TRUE(get().setNodeID("test_establishconnection").ok());
  EXPECT_TRUE(get().createEndpoint("test_establishconnection").ok());
  auto s_peer = get().peerEndpoint("127.0.0.1", 9999);
  EXPECT_TRUE(s_peer.ok());

  // Wait for message
  pollfd pfd{test_queue.fd(), POLLIN, 0};
  poll(&pfd, 1, 2000);
  auto msgs = test_queue.want_pop();

  // Exactly one message expected
  EXPECT_TRUE(msgs.size() == 1);
  auto msg = msgs.front();

  // Checking message format
  EXPECT_TRUE(msg.size() == 4);
  // EVENT Name
  EXPECT_TRUE(broker::is<std::string>(msg[0]));
  EXPECT_TRUE(*broker::get<std::string>(msg[0]) == get().EVENT_HOST_NEW);
  // Node ID
  EXPECT_TRUE(broker::is<std::string>(msg[1]));
  EXPECT_TRUE(*broker::get<std::string>(msg[1]) == "test_establishconnection");
  // Group List
  EXPECT_TRUE(broker::is<broker::vector>(msg[2]));
  broker::vector groups = *broker::get<broker::vector>(msg[2]);
  EXPECT_TRUE(groups.size() == 2);
  EXPECT_TRUE(*broker::get<std::string>(groups.at(0)) == "test1");
  EXPECT_TRUE(*broker::get<std::string>(groups.at(1)) == "test2");
  // Addr List
  EXPECT_TRUE(broker::is<broker::vector>(msg[3]));
}

TEST_F(BrokerManagerTests, test_addandremovegroups) {
  // Prepare receiver
  ep_->listen(9998, "127.0.0.1");
  // broker::message_queue test_queue(get().TOPIC_ANNOUNCE, *ep_);

  // Add group1 prior to connect
  EXPECT_TRUE(get().addGroup("test1").ok());

  // Connect the broker endpoint
  EXPECT_TRUE(get().setNodeID("test_addandremovegroups").ok());
  EXPECT_TRUE(get().createEndpoint("test_addandremovegroups").ok());
  auto s_peer = get().peerEndpoint("127.0.0.1", 9998);
  EXPECT_TRUE(s_peer.ok());
  if (not s_peer.ok()) {
    LOG(ERROR) << s_peer.getMessage();
  }

  // Add group2 after connecting
  EXPECT_TRUE(get().addGroup("test2").ok());

  // Expect subscription to both groups
  EXPECT_NE(get().getNodeID(), "");
  EXPECT_TRUE(get().getGroups().size() == 2);
  EXPECT_TRUE(get().getTopics().size() == 2);
  EXPECT_EQ(get().createEndpoint("should_not_work").getCode(), 1);

  // Receive on both groups
  // Create and send Messages
  broker::message test_msg1 = broker::message{broker::data("message1")};
  broker::message test_msg2 = broker::message{broker::data("message2")};
  EXPECT_TRUE(
      get().sendEvent(get().TOPIC_PRE_GROUPS + "test1", test_msg1).ok());
  EXPECT_TRUE(
      get().sendEvent(get().TOPIC_PRE_GROUPS + "test2", test_msg2).ok());
  // Receive Messages
  pollfd pfd1{
      get().getMessageQueue(get().TOPIC_PRE_GROUPS + "test1")->fd(), POLLIN, 0};
  pollfd pfd2{
      get().getMessageQueue(get().TOPIC_PRE_GROUPS + "test2")->fd(), POLLIN, 0};
  poll(&pfd1, 1, 2000);
  poll(&pfd2, 1, 2000);
  auto msgs1 =
      get().getMessageQueue(get().TOPIC_PRE_GROUPS + "test1")->want_pop();
  auto msgs2 =
      get().getMessageQueue(get().TOPIC_PRE_GROUPS + "test2")->want_pop();
  // Exactly one message expected per group
  EXPECT_TRUE(msgs1.size() == 1);
  EXPECT_TRUE(msgs2.size() == 1);
  auto msg1 = msgs1.front();
  auto msg2 = msgs2.front();
  // Expect messages on both groups
  EXPECT_TRUE(*broker::get<std::string>(msg1[0]) == "message1");
  EXPECT_TRUE(*broker::get<std::string>(msg2[0]) == "message2");

  // remove group2
  EXPECT_TRUE(get().removeGroup("test2").ok());

  // Expect 2
  EXPECT_NE(get().getNodeID(), "");
  EXPECT_TRUE(get().getGroups().size() == 1);
  EXPECT_TRUE(get().getGroups().at(0) == "test1");
  EXPECT_TRUE(get().getTopics().size() == 1);
  EXPECT_TRUE(get().getTopics().at(0) == get().TOPIC_PRE_GROUPS + "test1");
  EXPECT_EQ(get().createEndpoint("should_not_work").getCode(), 1);

  // Receive on both groups (with only subscribed on one)
  // Create and send Messages
  broker::message test_msg3 = broker::message{broker::data("message3")};
  broker::message test_msg4 = broker::message{broker::data("message4")};
  EXPECT_TRUE(
      get().sendEvent(get().TOPIC_PRE_GROUPS + "test1", test_msg3).ok());
  EXPECT_TRUE(
      get().sendEvent(get().TOPIC_PRE_GROUPS + "test2", test_msg4).ok());
  // Receive Messages
  pollfd pfd3{
      get().getMessageQueue(get().TOPIC_PRE_GROUPS + "test1")->fd(), POLLIN, 0};
  poll(&pfd3, 1, 2000);
  auto msgs3 =
      get().getMessageQueue(get().TOPIC_PRE_GROUPS + "test1")->want_pop();
  // Exactly one message expected per group
  EXPECT_TRUE(msgs3.size() == 1);
  auto msg3 = msgs3.front();
  // Expect messages on both groups
  EXPECT_TRUE(*broker::get<std::string>(msg3[0]) == "message3");

  // remove group1
  EXPECT_TRUE(get().removeGroup("test1").ok());

  // Expect subscription to no group
  EXPECT_NE(get().getNodeID(), "");
  EXPECT_TRUE(get().getGroups().size() == 0);
  EXPECT_TRUE(get().getTopics().size() == 0);
  EXPECT_EQ(get().createEndpoint("should_not_work").getCode(), 1);
}

TEST_F(BrokerManagerTests, test_reset) {
  // Prepare receiver
  ep_->listen(9997, "127.0.0.1");
  // broker::message_queue test_queue(get().TOPIC_ANNOUNCE, *ep_);

  // Add group1 prior to connect
  EXPECT_TRUE(get().addGroup("test1").ok());

  // Connect the broker endpoint
  EXPECT_TRUE(get().setNodeID("test_reset").ok());
  EXPECT_TRUE(get().createEndpoint("test_reset").ok());
  auto s_peer = get().peerEndpoint("127.0.0.1", 9997);
  EXPECT_TRUE(s_peer.ok());
  if (not s_peer.ok()) {
    LOG(ERROR) << s_peer.getMessage();
  }

  // Add group2 after connecting
  EXPECT_TRUE(get().addGroup("test2").ok());

  get().reset();

  // Expect reset
  EXPECT_EQ(get().getNodeID(), "");
  EXPECT_TRUE(get().getGroups().size() == 0);
  EXPECT_TRUE(get().getTopics().size() == 0);
  EXPECT_TRUE(get().createEndpoint("should_work").ok());
}

TEST_F(BrokerManagerTests, test_logQueryLogItemToBro) {
  // TODO: test logging for QueryLogItem
}
}