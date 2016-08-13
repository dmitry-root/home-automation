#include <gtest/gtest.h>

#include "proto/Message.h"

TEST(Proto, MessageSerializeRequest)
{
	using namespace dh::proto;

	const Message message(Message::Type_Request, 0x12, 0x3456);
	EXPECT_EQ( 0x12, message.device_id() );
	EXPECT_EQ( 0x3456, message.address() );

	const std::string line = message;

	const Message result(line);
	EXPECT_EQ( Message::Type_Request, result.type() );
	EXPECT_EQ( 0x12, result.device_id() );
	EXPECT_EQ( 0x3456, result.address() );
}

TEST(Proto, MessageSerializeData)
{
	using namespace dh::proto;

	const Message::Body body = { 0x12, 0x34, 0x56 };

	const Message message(Message::Type_Data, 0x12, 0x3456, body);
	EXPECT_EQ( 0x12, message.device_id() );
	EXPECT_EQ( 0x3456, message.address() );
	EXPECT_EQ( body, message.body() );

	const std::string line = message;

	const Message result(line);
	EXPECT_EQ( Message::Type_Data, result.type() );
	EXPECT_EQ( 0x12, result.device_id() );
	EXPECT_EQ( 0x3456, result.address() );
	EXPECT_EQ( body, result.body() );
}

TEST(Proto, MessageSerializeFormat)
{
	using namespace dh::proto;

	const std::string line = "dev:1f addr:De5E :33f3ECc3";
	const Message::Body body = { 0x33, 0xf3, 0xec, 0xc3 };

	const Message message(line);
	EXPECT_EQ( Message::Type_Data, message.type() );
	EXPECT_EQ( 0x1f, message.device_id() );
	EXPECT_EQ( 0xde5e, message.address() );
	EXPECT_EQ( body, message.body() );
}


TEST(Proto, DeviceInitSerialize)
{
	using namespace dh::proto;

	const DeviceInit dev_init(0x5c, 0xdd);
	const std::string line = dev_init;

	const DeviceInit result(line);
	EXPECT_EQ( 0x5c, result.new_id() );
	EXPECT_EQ( 0xdd, result.old_id() );
}

TEST(Proto, DeviceInitSerializeFormat)
{
	using namespace dh::proto;

	const std::string line = "dev-q:3ee5";
	const DeviceInit dev_init(line);

	EXPECT_EQ( 0xe5, dev_init.new_id() );
	EXPECT_EQ( 0x3e, dev_init.old_id() );
}


TEST(Proto, ServiceCommandSerialize)
{
	using namespace dh::proto;

	ServiceCommand scmd("ping");
	scmd.add_argument("string", "hello");
	scmd.add_argument("int", 256u);

	const std::string line = scmd.convert_to_string();

	ServiceCommand result;
	result.assign_from_string(line);

	EXPECT_EQ( "ping", result.action() );
	EXPECT_EQ( "hello", result.get_argument("string") );
	EXPECT_EQ( 256u, result.get_argument("int", 0u) );
	EXPECT_EQ( 111u, result.get_argument("what", 111u) );
}

TEST(Proto, ServiceCommandSerializeFormat)
{
	using namespace dh::proto;

	const std::string line = "cmd:qwerty key:value empty: int:663d";

	ServiceCommand scmd;
	scmd.assign_from_string(line);

	EXPECT_EQ( "qwerty", scmd.action() );
	EXPECT_EQ( "value", scmd.get_argument("key") );
	EXPECT_EQ( "", scmd.get_argument("empty") );
	EXPECT_EQ( 0x663d, scmd.get_argument("int", 0u) );
}


int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
