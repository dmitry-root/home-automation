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


int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
