/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <gtest/gtest.h>

#include <dl/dl.h>
#include <dl/dl_txt.h>

#include "dl_test_common.h"

TEST_F(DL, TextMemberOrder)
{
	// test to pack a txt-instance that is not in order!
	const char* TextData =
	"{"
		"\"root\" : {"
			"\"type\" : \"Pods\","
			"\"data\" : {"
				"\"i16\" : 2,"
				"\"u32\" : 7,"
				"\"u8\"  : 5,"
				"\"i32\" : 3,"
				"\"f64\" : 4.1234,"
				"\"i64\" : 4,"
				"\"u16\" : 6,"
				"\"u64\" : 8,"
				"\"i8\"  : 1,"
				"\"f32\" : 3.14"
			"}"
		"}"
	"}";

	Pods P1;

 	uint8 OutDataText[1024];

	// pack txt to binary
	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(1,      P1.i8);
 	EXPECT_EQ(2,      P1.i16);
	EXPECT_EQ(3,      P1.i32);
	EXPECT_EQ(4,      P1.i64);
	EXPECT_EQ(5u,     P1.u8);
	EXPECT_EQ(6u,     P1.u16);
	EXPECT_EQ(7u,     P1.u32);
	EXPECT_EQ(8u,     P1.u64);
	EXPECT_EQ(3.14f,  P1.f32);
	EXPECT_EQ(4.1234, P1.f64);
}

TEST_F(DL, TextSetMemberTwice)
{
	// test to pack a txt-instance that is not in order!
	const char* TextData = 
		"{"
			"\"root\" : {"
				"\"type\" : \"Pods\","
				"\"data\" : {"
					"\"i16\" : 2,"
					"\"u32\" : 7,"
					"\"u8\"  : 5,"
					"\"i32\" : 3,"
					"\"i32\" : 7,"
					"\"f64\" : 4.1234,"
					"\"i64\" : 4,"
					"\"u16\" : 6,"
					"\"u64\" : 8,"
					"\"i8\"  : 1,"
					"\"f32\" : 3.14"
				"}"
			"}"
		"}";

	uint8 OutDataText[1024];

	// pack txt to binary
	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_TXT_MEMBER_SET_TWICE, err);
}

TEST_F(DL, TextNonExistMember)
{
	// test to pack a txt-instance that is not in order!
	const char* TextData = 
		"{"
			"\"root\" : {"
				"\"type\" : \"Pods2\","
				"\"data\" : { \"Int1\" : 1337, \"Int2\" : 1337, \"IDoNotExist\" : 1337 }"
			"}"
		"}";

	uint8 OutDataText[1024];

	// pack txt to binary
	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_MEMBER_NOT_FOUND, err);
}

TEST_F(DL, TextErrorMissingMember)
{
	// error should be cast if member is not set and has no default value!
	// in this case Int2 is not set!

	const char* TextData = "{ \"root\" : { \"type\" : \"Pods2\", \"data\" : { \"Int1\" : 1337 } } }";

	uint8 OutDataText[1024];
	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_TXT_MEMBER_MISSING, err);
}

TEST_F(DL, TextPodDefaults)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"PodsDefaults\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	PodsDefaults P1;

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(2,     P1.i8);
	EXPECT_EQ(3,     P1.i16);
	EXPECT_EQ(4,     P1.i32);
	EXPECT_EQ(5,     P1.i64);
	EXPECT_EQ(6u,    P1.u8);
	EXPECT_EQ(7u,    P1.u16);
	EXPECT_EQ(8u,    P1.u32);
	EXPECT_EQ(9u,    P1.u64);
	EXPECT_EQ(10.0f, P1.f32);
	EXPECT_EQ(11.0,  P1.f64);
}

TEST_F(DL, TextDefaultStr)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultStr\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	DefaultStr P1[10]; // this is so ugly!
	
	// load binary
	err = dl_instance_load(Ctx, P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_STREQ("cowbells ftw!", P1[0].Str);
}

TEST_F(DL, TextDefaultPtr)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultPtr\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	DefaultPtr P1; // this is so ugly!

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(0x0, P1.Ptr);
}

TEST_F(DL, TextDefaultStruct)
{
	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultStruct\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	DefaultStruct P1; // this is so ugly!

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(13u, P1.Struct.Int1);
	EXPECT_EQ(37u, P1.Struct.Int2);
}

TEST_F(DL, TextDefaultEnum)
{
	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultEnum\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	DefaultEnum P1;

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(TESTENUM1_VALUE3, P1.Enum);
}

TEST_F(DL, TextDefaultInlineArrayPod)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultInlArrayPod\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	DefaultInlArrayPod P1;

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(1u, P1.Arr[0]);
	EXPECT_EQ(3u, P1.Arr[1]);
	EXPECT_EQ(3u, P1.Arr[2]);
	EXPECT_EQ(7u, P1.Arr[3]);
}

TEST_F(DL, TextDefaultInlineArrayEnum)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultInlArrayEnum\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	DefaultInlArrayEnum P1;

	// load binary
	err = dl_instance_load(Ctx, &P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(TESTENUM1_VALUE3, P1.Arr[0]);
	EXPECT_EQ(TESTENUM1_VALUE1, P1.Arr[1]);
	EXPECT_EQ(TESTENUM1_VALUE2, P1.Arr[2]);
	EXPECT_EQ(TESTENUM1_VALUE4, P1.Arr[3]);
}

TEST_F(DL, TextDefaultInlineArrayString)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultInlArrayStr\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	DefaultInlArrayStr P1[10];

	// load binary
	err = dl_instance_load(Ctx, P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_STREQ("cow",   P1[0].Arr[0]);
	EXPECT_STREQ("bells", P1[0].Arr[1]);
	EXPECT_STREQ("are",   P1[0].Arr[2]);
	EXPECT_STREQ("cool",  P1[0].Arr[3]);
}

TEST_F(DL, TextDefaultArrayPod)
{
	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultArrayPod\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	DefaultArrayPod P1[10];

	// load binary
	err = dl_instance_load(Ctx, P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(4u, P1[0].Arr.nCount);
	
	EXPECT_EQ(1u, P1[0].Arr[0]);
	EXPECT_EQ(3u, P1[0].Arr[1]);
	EXPECT_EQ(3u, P1[0].Arr[2]);
	EXPECT_EQ(7u, P1[0].Arr[3]);
}

TEST_F(DL, TextDefaultArrayEnum)
{
	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultArrayEnum\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	DefaultArrayEnum P1[10];

	// load binary
	err = dl_instance_load(Ctx, P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(4u, P1[0].Arr.nCount);

	EXPECT_EQ(TESTENUM1_VALUE3, P1[0].Arr[0]);
	EXPECT_EQ(TESTENUM1_VALUE1, P1[0].Arr[1]);
	EXPECT_EQ(TESTENUM1_VALUE2, P1[0].Arr[2]);
	EXPECT_EQ(TESTENUM1_VALUE4, P1[0].Arr[3]);
}

TEST_F(DL, TextDefaultArrayString)
{
	// default-values should be set correctly!

	const char* TextData = "{ \"root\" : { \"type\" : \"DefaultArrayStr\", \"data\" : {} } }";

	uint8 OutDataText[1024];

	dl_error_t err = dl_txt_pack(Ctx, TextData, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	DefaultArrayStr P1[10];

	// load binary
	err = dl_instance_load(Ctx, P1, OutDataText, 1024);
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	EXPECT_EQ(4u, P1[0].Arr.nCount);

	EXPECT_STREQ("cow",   P1[0].Arr[0]);
	EXPECT_STREQ("bells", P1[0].Arr[1]);
	EXPECT_STREQ("are",   P1[0].Arr[2]);
	EXPECT_STREQ("cool",  P1[0].Arr[3]);
}

