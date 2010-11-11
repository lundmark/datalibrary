/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <gtest/gtest.h>

#include <dl/dl.h>
#include <dl/dl_txt.h>
#include <dl/dl_convert.h>

#include "dl_test_common.h"

#include <float.h>

// TODO: Check these versus standard defines!
static const int8  DL_INT8_MAX  = 0x7F;
static const int16 DL_INT16_MAX = 0x7FFF;
static const int32 DL_INT32_MAX = 0x7FFFFFFFL;
static const int64 DL_INT64_MAX = 0x7FFFFFFFFFFFFFFFLL;
static const int8  DL_INT8_MIN  = (-DL_INT8_MAX  - 1);
static const int16 DL_INT16_MIN = (-DL_INT16_MAX - 1);
static const int32 DL_INT32_MIN = (-DL_INT32_MAX - 1);
static const int64 DL_INT64_MIN = (-DL_INT64_MAX - 1);

static const uint8  DL_UINT8_MAX  = 0xFFU;
static const uint16 DL_UINT16_MAX = 0xFFFFU;
static const uint32 DL_UINT32_MAX = 0xFFFFFFFFUL;
static const uint64 DL_UINT64_MAX = 0xFFFFFFFFFFFFFFFFULL;
static const uint8  DL_UINT8_MIN  = 0x00U;
static const uint16 DL_UINT16_MIN = 0x0000U;
static const uint32 DL_UINT32_MIN = 0x00000000UL;
static const uint64 DL_UINT64_MIN = 0x0000000000000000ULL;


void DoTheRoundAbout(dl_ctx_t _Ctx, dl_typeid_t _TypeHash, void* _pPackMe, void* _pUnpackMe)
{
	unsigned char OutDataInstance[1024];
	char  TxtOut[2048];

	memset(OutDataInstance, 0x0, sizeof(OutDataInstance));
	memset(TxtOut, 0x0,  sizeof(TxtOut));

	// store instance to binary
	dl_error_t err = dl_instance_store(_Ctx, _TypeHash, _pPackMe, OutDataInstance, DL_ARRAY_LENGTH(OutDataInstance));
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
	EXPECT_EQ(sizeof(void*),  dl_instance_ptr_size(OutDataInstance,  DL_ARRAY_LENGTH(OutDataInstance)));
	EXPECT_EQ(DL_ENDIAN_HOST, dl_instance_endian(OutDataInstance,   DL_ARRAY_LENGTH(OutDataInstance)));
	EXPECT_EQ(_TypeHash,      dl_instance_root_type(OutDataInstance, DL_ARRAY_LENGTH(OutDataInstance)));

	// unpack binary to txt
	err = dl_txt_unpack(_Ctx, OutDataInstance, DL_ARRAY_LENGTH(OutDataInstance), TxtOut, DL_ARRAY_LENGTH(TxtOut));
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	// M_LOG_INFO("%s", TxtOut);

	unsigned char OutDataText[1024];
	memset(OutDataText, 0x0, DL_ARRAY_LENGTH(OutDataText));

	// pack txt to binary
	err = dl_txt_pack(_Ctx, TxtOut, OutDataText, DL_ARRAY_LENGTH(OutDataText));
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
	EXPECT_EQ(sizeof(void*),  dl_instance_ptr_size(OutDataText,  DL_ARRAY_LENGTH(OutDataText)));
	EXPECT_EQ(DL_ENDIAN_HOST, dl_instance_endian(OutDataText,   DL_ARRAY_LENGTH(OutDataText)));
	EXPECT_EQ(_TypeHash,      dl_instance_root_type(OutDataText, DL_ARRAY_LENGTH(OutDataText)));

	// load binary
	err = dl_instance_load(_Ctx, _pUnpackMe, OutDataText, DL_ARRAY_LENGTH(OutDataText));
	EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

	dl_endian_t   OtherEndian  = DL_ENDIAN_HOST == DL_ENDIAN_BIG ? DL_ENDIAN_LITTLE : DL_ENDIAN_BIG;
	unsigned int OtherPtrSize = sizeof(void*) == 4 ? 8 : 4;

	// check if we can convert only endianness!
	{
		uint8 SwitchedEndian[DL_ARRAY_LENGTH(OutDataText)];
		memcpy(SwitchedEndian, OutDataText, DL_ARRAY_LENGTH(OutDataText));

		err = dl_convert_inplace(_Ctx, SwitchedEndian, DL_ARRAY_LENGTH(SwitchedEndian), OtherEndian, sizeof(void*));
		EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
		EXPECT_EQ(sizeof(void*), dl_instance_ptr_size(SwitchedEndian, DL_ARRAY_LENGTH(SwitchedEndian)));
		EXPECT_EQ(OtherEndian,   dl_instance_endian(SwitchedEndian,  DL_ARRAY_LENGTH(SwitchedEndian)));

		EXPECT_NE(0, memcmp(OutDataText, SwitchedEndian, DL_ARRAY_LENGTH(OutDataText))); // original data should not be equal !

		err = dl_convert_inplace(_Ctx, SwitchedEndian, DL_ARRAY_LENGTH(SwitchedEndian), DL_ENDIAN_HOST, sizeof(void*));
		EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
		EXPECT_EQ(sizeof(void*),  dl_instance_ptr_size(SwitchedEndian,  DL_ARRAY_LENGTH(SwitchedEndian)));
		EXPECT_EQ(DL_ENDIAN_HOST, dl_instance_endian(SwitchedEndian,    DL_ARRAY_LENGTH(SwitchedEndian)));
		EXPECT_EQ(_TypeHash,      dl_instance_root_type(SwitchedEndian, DL_ARRAY_LENGTH(SwitchedEndian)));

		EXPECT_EQ(0, memcmp(OutDataText, SwitchedEndian, DL_ARRAY_LENGTH(OutDataText))); // original data should be equal !
	}

	// check problems when only ptr-size!
	{
		unsigned char OriginalData[DL_ARRAY_LENGTH(OutDataText)];
		memcpy(OriginalData, OutDataText, DL_ARRAY_LENGTH(OutDataText));

		unsigned int PackedSize = 0;
		err = dl_instance_calc_size(_Ctx, _TypeHash, _pPackMe, &PackedSize);
		EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

		unsigned int ConvertedSize = 0;
		err = dl_convert_calc_size(_Ctx, OriginalData, DL_ARRAY_LENGTH(OriginalData), OtherPtrSize, &ConvertedSize);
		EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);

		if(OtherPtrSize <= sizeof(void*)) EXPECT_LE(ConvertedSize, PackedSize);
		else                              EXPECT_GE(ConvertedSize, PackedSize);

		// check inplace conversion.
		if(OtherPtrSize < sizeof(void*))
		{
			// if this is true, the conversion is doable with inplace-version.
			uint8 ConvertedData[DL_ARRAY_LENGTH(OutDataText)];
			memcpy(ConvertedData, OriginalData, DL_ARRAY_LENGTH(OriginalData));

			err = dl_convert_inplace(_Ctx, ConvertedData, DL_ARRAY_LENGTH(ConvertedData), DL_ENDIAN_HOST, OtherPtrSize);
			EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
			EXPECT_EQ(OtherPtrSize,   dl_instance_ptr_size(ConvertedData,  DL_ARRAY_LENGTH(ConvertedData)));
			EXPECT_EQ(DL_ENDIAN_HOST, dl_instance_endian(ConvertedData,    DL_ARRAY_LENGTH(ConvertedData)));
			EXPECT_EQ(_TypeHash,      dl_instance_root_type(ConvertedData, DL_ARRAY_LENGTH(ConvertedData)));

			EXPECT_NE(0, memcmp(OutDataText, ConvertedData, DL_ARRAY_LENGTH(OutDataText))); // original data should not be equal !

			unsigned char OriginalConverted[DL_ARRAY_LENGTH(OutDataText)];
			memcpy(OriginalConverted, ConvertedData, DL_ARRAY_LENGTH(OriginalData));

			unsigned int ReConvertedSize = 0;
			err = dl_convert_calc_size(_Ctx, ConvertedData, DL_ARRAY_LENGTH(ConvertedData), sizeof(void*), &ReConvertedSize);
			EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
			EXPECT_EQ(PackedSize, ReConvertedSize); // should be same size as original!

			unsigned char ReConvertedData[DL_ARRAY_LENGTH(OutDataText) * 3];
			ReConvertedData[ReConvertedSize + 1] = 'L';
			ReConvertedData[ReConvertedSize + 2] = 'O';
			ReConvertedData[ReConvertedSize + 3] = 'L';
			ReConvertedData[ReConvertedSize + 4] = '!';

			err = dl_convert(_Ctx, ConvertedData, DL_ARRAY_LENGTH(ConvertedData), ReConvertedData, DL_ARRAY_LENGTH(ReConvertedData), DL_ENDIAN_HOST, sizeof(void*));
			EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
			EXPECT_EQ(sizeof(void*),  dl_instance_ptr_size(ReConvertedData,  DL_ARRAY_LENGTH(ReConvertedData)));
			EXPECT_EQ(DL_ENDIAN_HOST, dl_instance_endian(ReConvertedData,    DL_ARRAY_LENGTH(ReConvertedData)));
			EXPECT_EQ(_TypeHash,      dl_instance_root_type(ReConvertedData, DL_ARRAY_LENGTH(ReConvertedData)));

			EXPECT_EQ(0, memcmp(OriginalConverted, ConvertedData, DL_ARRAY_LENGTH(OriginalConverted))); // original data should be equal !

			EXPECT_EQ('L', ReConvertedData[ReConvertedSize + 1]);
			EXPECT_EQ('O', ReConvertedData[ReConvertedSize + 2]);
			EXPECT_EQ('L', ReConvertedData[ReConvertedSize + 3]);
			EXPECT_EQ('!', ReConvertedData[ReConvertedSize + 4]);
		}

		// check both endian and ptrsize convert!
		{
			unsigned char ConvertedData[DL_ARRAY_LENGTH(OutDataText) * 3];
			ConvertedData[ConvertedSize + 1] = 'L';
			ConvertedData[ConvertedSize + 2] = 'O';
			ConvertedData[ConvertedSize + 3] = 'L';
			ConvertedData[ConvertedSize + 4] = '!';

			err = dl_convert(_Ctx, OriginalData, DL_ARRAY_LENGTH(OriginalData), ConvertedData, DL_ARRAY_LENGTH(ConvertedData), OtherEndian, OtherPtrSize);
			EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
			EXPECT_EQ(OtherPtrSize, dl_instance_ptr_size(ConvertedData, DL_ARRAY_LENGTH(ConvertedData)));
			EXPECT_EQ(OtherEndian,  dl_instance_endian(ConvertedData,  DL_ARRAY_LENGTH(ConvertedData)));

			EXPECT_EQ('L', ConvertedData[ConvertedSize + 1]);
			EXPECT_EQ('O', ConvertedData[ConvertedSize + 2]);
			EXPECT_EQ('L', ConvertedData[ConvertedSize + 3]);
			EXPECT_EQ('!', ConvertedData[ConvertedSize + 4]);

			EXPECT_NE(0, memcmp(OutDataText, ConvertedData, DL_ARRAY_LENGTH(OutDataText))); // original data should not be equal !

			unsigned int ReConvertedSize = 0;
			err = dl_convert_calc_size(_Ctx, ConvertedData, DL_ARRAY_LENGTH(ConvertedData), sizeof(void*), &ReConvertedSize);
			EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
			EXPECT_EQ(PackedSize, ReConvertedSize); // should be same size as original!

			uint8 ReConvertedData[DL_ARRAY_LENGTH(OutDataText) * 2];
			ReConvertedData[ReConvertedSize + 1] = 'L';
			ReConvertedData[ReConvertedSize + 2] = 'O';
			ReConvertedData[ReConvertedSize + 3] = 'L';
			ReConvertedData[ReConvertedSize + 4] = '!';

			err = dl_convert(_Ctx, ConvertedData, DL_ARRAY_LENGTH(ConvertedData), ReConvertedData, DL_ARRAY_LENGTH(ReConvertedData), DL_ENDIAN_HOST, sizeof(void*));
			EXPECT_DL_ERR_EQ(DL_ERROR_OK, err);
			EXPECT_EQ(sizeof(void*),  dl_instance_ptr_size(ReConvertedData,  DL_ARRAY_LENGTH(ReConvertedData)));
			EXPECT_EQ(DL_ENDIAN_HOST, dl_instance_endian(ReConvertedData,    DL_ARRAY_LENGTH(ReConvertedData)));
			EXPECT_EQ(_TypeHash,      dl_instance_root_type(ReConvertedData, DL_ARRAY_LENGTH(ReConvertedData)));

			EXPECT_EQ(0, memcmp(OutDataText, ReConvertedData, PackedSize)); // original data should be equal !

			EXPECT_EQ('L', ReConvertedData[ReConvertedSize + 1]);
			EXPECT_EQ('O', ReConvertedData[ReConvertedSize + 2]);
			EXPECT_EQ('L', ReConvertedData[ReConvertedSize + 3]);
			EXPECT_EQ('!', ReConvertedData[ReConvertedSize + 4]);
		}
	}
}

TEST_F(DL, TestPack)
{
	Pods P1Original = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	Pods P1         = { 0 };

	DoTheRoundAbout(Ctx, Pods::TYPE_ID, &P1Original, &P1);

	EXPECT_EQ(P1Original.i8,   P1.i8);
	EXPECT_EQ(P1Original.i16,  P1.i16);
	EXPECT_EQ(P1Original.i32,  P1.i32);
	EXPECT_EQ(P1Original.i64,  P1.i64);
	EXPECT_EQ(P1Original.u8,  P1.u8);
	EXPECT_EQ(P1Original.u16, P1.u16);
	EXPECT_EQ(P1Original.u32, P1.u32);
	EXPECT_EQ(P1Original.u64, P1.u64);
	EXPECT_EQ(P1Original.f32,   P1.f32);
	EXPECT_EQ(P1Original.f64,   P1.f64);
}

TEST_F(DL, MaxPod)
{
	Pods P1Original = { DL_INT8_MAX, DL_INT16_MAX, DL_INT32_MAX, DL_INT64_MAX, DL_UINT8_MAX, DL_UINT16_MAX, DL_UINT32_MAX, DL_UINT64_MAX, FLT_MAX, DBL_MAX };
	Pods P1         = { 0 };

	DoTheRoundAbout(Ctx, Pods::TYPE_ID, &P1Original, &P1);

	EXPECT_EQ(P1Original.i8,   P1.i8);
	EXPECT_EQ(P1Original.i16,  P1.i16);
	EXPECT_EQ(P1Original.i32,  P1.i32);
	EXPECT_EQ(P1Original.i64,  P1.i64);
	EXPECT_EQ(P1Original.u8,  P1.u8);
	EXPECT_EQ(P1Original.u16, P1.u16);
	EXPECT_EQ(P1Original.u32, P1.u32);
	EXPECT_EQ(P1Original.u64, P1.u64);
	// need to disable these tests for floating-point-errors =/
	// EXPECT_FLOAT_EQ(P1Original.f32,   P1.f32);
	// EXPECT_DOUBLE_EQ(P1Original.f64,   P1.f64);
}

TEST_F(DL, MinPod)
{
	Pods P1Original = { DL_INT8_MIN, DL_INT16_MIN, DL_INT32_MIN, DL_INT64_MIN, DL_UINT8_MIN, DL_UINT16_MIN, DL_UINT32_MIN, DL_UINT64_MIN, FLT_MIN, DBL_MIN };
	Pods P1         = { 0 };

	DoTheRoundAbout(Ctx, Pods::TYPE_ID, &P1Original, &P1);

	EXPECT_EQ(P1Original.i8,   P1.i8);
	EXPECT_EQ(P1Original.i16,  P1.i16);
	EXPECT_EQ(P1Original.i32,  P1.i32);
	EXPECT_EQ(P1Original.i64,  P1.i64);
	EXPECT_EQ(P1Original.u8,  P1.u8);
	EXPECT_EQ(P1Original.u16, P1.u16);
	EXPECT_EQ(P1Original.u32, P1.u32);
	EXPECT_EQ(P1Original.u64, P1.u64);
	EXPECT_NEAR(P1Original.f32, P1.f32, 0.0000001f);
	EXPECT_NEAR(P1Original.f64, P1.f64, 0.0000001f);
}

TEST_F(DL, StructInStruct)
{
	MorePods P1Original = { { 1, 2, 3, 4, 5, 6, 7, 8, 0.0f, 0}, { 9, 10, 11, 12, 13, 14, 15, 16, 0.0f, 0} };
	MorePods P1         = { { 0 }, { 0 } };

	DoTheRoundAbout(Ctx, MorePods::TYPE_ID, &P1Original, &P1);

	EXPECT_EQ(P1Original.Pods1.i8,    P1.Pods1.i8);
	EXPECT_EQ(P1Original.Pods1.i16,   P1.Pods1.i16);
	EXPECT_EQ(P1Original.Pods1.i32,   P1.Pods1.i32);
	EXPECT_EQ(P1Original.Pods1.i64,   P1.Pods1.i64);
	EXPECT_EQ(P1Original.Pods1.u8,    P1.Pods1.u8);
	EXPECT_EQ(P1Original.Pods1.u16,   P1.Pods1.u16);
	EXPECT_EQ(P1Original.Pods1.u32,   P1.Pods1.u32);
	EXPECT_EQ(P1Original.Pods1.u64,   P1.Pods1.u64);
	EXPECT_NEAR(P1Original.Pods1.f32, P1.Pods1.f32, 0.0000001f);
	EXPECT_NEAR(P1Original.Pods1.f64, P1.Pods1.f64, 0.0000001f);

	EXPECT_EQ(P1Original.Pods2.i8,    P1.Pods2.i8);
	EXPECT_EQ(P1Original.Pods2.i16,   P1.Pods2.i16);
	EXPECT_EQ(P1Original.Pods2.i32,   P1.Pods2.i32);
	EXPECT_EQ(P1Original.Pods2.i64,   P1.Pods2.i64);
	EXPECT_EQ(P1Original.Pods2.u8,    P1.Pods2.u8);
	EXPECT_EQ(P1Original.Pods2.u16,   P1.Pods2.u16);
	EXPECT_EQ(P1Original.Pods2.u32,   P1.Pods2.u32);
	EXPECT_EQ(P1Original.Pods2.u64,   P1.Pods2.u64);
	EXPECT_NEAR(P1Original.Pods2.f32, P1.Pods2.f32, 0.0000001f);
	EXPECT_NEAR(P1Original.Pods2.f64, P1.Pods2.f64, 0.0000001f);
}

TEST_F(DL, StructInStructInStruct)
{
	Pod2InStructInStruct Orig;
	Pod2InStructInStruct New;

	Orig.p2struct.Pod1.Int1 = 1337;
	Orig.p2struct.Pod1.Int2 = 7331;
	Orig.p2struct.Pod2.Int1 = 1234;
	Orig.p2struct.Pod2.Int2 = 4321;

	DoTheRoundAbout(Ctx, Pod2InStructInStruct::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.p2struct.Pod1.Int1, New.p2struct.Pod1.Int1);
	EXPECT_EQ(Orig.p2struct.Pod1.Int2, New.p2struct.Pod1.Int2);
	EXPECT_EQ(Orig.p2struct.Pod2.Int1, New.p2struct.Pod2.Int1);
	EXPECT_EQ(Orig.p2struct.Pod2.Int2, New.p2struct.Pod2.Int2);
}

TEST_F(DL, InlineArray)
{
	WithInlineArray Orig;
	WithInlineArray New;

	Orig.Array[0] = 1337;
	Orig.Array[1] = 7331;
	Orig.Array[2] = 1234;

	DoTheRoundAbout(Ctx, WithInlineArray::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.Array[0], New.Array[0]);
	EXPECT_EQ(Orig.Array[1], New.Array[1]);
	EXPECT_EQ(Orig.Array[2], New.Array[2]);
}

TEST_F(DL, WithInlineStructArray)
{
	WithInlineStructArray Orig;
	WithInlineStructArray New;

	Orig.Array[0].Int1 = 1337;
	Orig.Array[0].Int2 = 7331;
	Orig.Array[1].Int1 = 1224;
	Orig.Array[1].Int2 = 5678;
	Orig.Array[2].Int1 = 9012;
	Orig.Array[2].Int2 = 3456;

	DoTheRoundAbout(Ctx, WithInlineStructArray::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.Array[0].Int1, New.Array[0].Int1);
	EXPECT_EQ(Orig.Array[0].Int2, New.Array[0].Int2);
	EXPECT_EQ(Orig.Array[1].Int1, New.Array[1].Int1);
	EXPECT_EQ(Orig.Array[1].Int2, New.Array[1].Int2);
	EXPECT_EQ(Orig.Array[2].Int1, New.Array[2].Int1);
	EXPECT_EQ(Orig.Array[2].Int2, New.Array[2].Int2);
}

TEST_F(DL, WithInlineStructStructArray)
{
	WithInlineStructStructArray Orig;
	WithInlineStructStructArray New;

	Orig.Array[0].Array[0].Int1 = 1;
	Orig.Array[0].Array[0].Int2 = 3;
	Orig.Array[0].Array[1].Int1 = 3;
	Orig.Array[0].Array[1].Int2 = 7;
	Orig.Array[0].Array[2].Int1 = 13;
	Orig.Array[0].Array[2].Int2 = 37;
	Orig.Array[1].Array[0].Int1 = 1337;
	Orig.Array[1].Array[0].Int2 = 7331;
	Orig.Array[1].Array[1].Int1 = 1;
	Orig.Array[1].Array[1].Int2 = 337;
	Orig.Array[1].Array[2].Int1 = 133;
	Orig.Array[1].Array[2].Int2 = 7;

	DoTheRoundAbout(Ctx, WithInlineStructStructArray::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.Array[0].Array[0].Int1, New.Array[0].Array[0].Int1);
	EXPECT_EQ(Orig.Array[0].Array[0].Int2, New.Array[0].Array[0].Int2);
	EXPECT_EQ(Orig.Array[0].Array[1].Int1, New.Array[0].Array[1].Int1);
	EXPECT_EQ(Orig.Array[0].Array[1].Int2, New.Array[0].Array[1].Int2);
	EXPECT_EQ(Orig.Array[0].Array[2].Int1, New.Array[0].Array[2].Int1);
	EXPECT_EQ(Orig.Array[0].Array[2].Int2, New.Array[0].Array[2].Int2);
	EXPECT_EQ(Orig.Array[1].Array[0].Int1, New.Array[1].Array[0].Int1);
	EXPECT_EQ(Orig.Array[1].Array[0].Int2, New.Array[1].Array[0].Int2);
	EXPECT_EQ(Orig.Array[1].Array[1].Int1, New.Array[1].Array[1].Int1);
	EXPECT_EQ(Orig.Array[1].Array[1].Int2, New.Array[1].Array[1].Int2);
	EXPECT_EQ(Orig.Array[1].Array[2].Int1, New.Array[1].Array[2].Int1);
	EXPECT_EQ(Orig.Array[1].Array[2].Int2, New.Array[1].Array[2].Int2);
}

TEST_F(DL, PodArray1)
{
	uint32 Data[8] = { 1337, 7331, 13, 37, 133, 7, 1, 337 } ;
	PodArray1 Orig = { { Data, 8 } };
	PodArray1* New;

	uint32 Loaded[1024]; // this is so ugly!

	DoTheRoundAbout(Ctx, PodArray1::TYPE_ID, &Orig, Loaded);

	New = (PodArray1*)&Loaded[0];

	EXPECT_EQ(Orig.Array.nCount, New->Array.nCount);
	EXPECT_EQ(Orig.Array[0], New->Array[0]);
	EXPECT_EQ(Orig.Array[1], New->Array[1]);
	EXPECT_EQ(Orig.Array[2], New->Array[2]);
	EXPECT_EQ(Orig.Array[3], New->Array[3]);
	EXPECT_EQ(Orig.Array[4], New->Array[4]);
	EXPECT_EQ(Orig.Array[5], New->Array[5]);
	EXPECT_EQ(Orig.Array[6], New->Array[6]);
	EXPECT_EQ(Orig.Array[7], New->Array[7]);
}

TEST_F(DL, PodArray2)
{
	uint32 Data1[] = { 1337, 7331,  13, 37, 133 } ;
	uint32 Data2[] = {    7,    1, 337 } ;

	PodArray1 OrigArray[] = { { { Data1, 5 } }, { { Data2, 3 } } } ;

	PodArray2 Orig = { { OrigArray, 2 } };
	PodArray2* New;

	uint32 Loaded[1024]; // this is so ugly!

	DoTheRoundAbout(Ctx, PodArray2::TYPE_ID, &Orig, Loaded);

	New = (PodArray2*)&Loaded[0];

	EXPECT_EQ(Orig.Array.nCount, New->Array.nCount);

	EXPECT_EQ(Orig.Array[0].Array.nCount, New->Array[0].Array.nCount);
	EXPECT_EQ(Orig.Array[1].Array.nCount, New->Array[1].Array.nCount);

	EXPECT_EQ(Orig.Array[0].Array[0], New->Array[0].Array[0]);
	EXPECT_EQ(Orig.Array[0].Array[1], New->Array[0].Array[1]);
	EXPECT_EQ(Orig.Array[0].Array[2], New->Array[0].Array[2]);
	EXPECT_EQ(Orig.Array[0].Array[3], New->Array[0].Array[3]);
	EXPECT_EQ(Orig.Array[0].Array[4], New->Array[0].Array[4]);
	EXPECT_EQ(Orig.Array[1].Array[0], New->Array[1].Array[0]);
	EXPECT_EQ(Orig.Array[1].Array[1], New->Array[1].Array[1]);
	EXPECT_EQ(Orig.Array[1].Array[2], New->Array[1].Array[2]);
}

TEST_F(DL, SimpleString)
{
	Strings Orig = { "cow", "bell" } ;
	Strings* New;

	Strings Loaded[5]; // this is so ugly!

	DoTheRoundAbout(Ctx, Strings::TYPE_ID, &Orig, Loaded);

	New = Loaded;

	EXPECT_STREQ(Orig.Str1, New->Str1);
	EXPECT_STREQ(Orig.Str2, New->Str2);
}

TEST_F(DL, InlineArrayString)
{
	StringInlineArray Orig = { { (char*)"awsum", (char*)"cowbells", (char*)"FTW!" } } ;
	StringInlineArray* New;

	StringInlineArray Loaded[5]; // this is so ugly!

	DoTheRoundAbout(Ctx, StringInlineArray::TYPE_ID, &Orig, Loaded);

	New = Loaded;

	EXPECT_STREQ(Orig.Strings[0], New->Strings[0]);
	EXPECT_STREQ(Orig.Strings[1], New->Strings[1]);
	EXPECT_STREQ(Orig.Strings[2], New->Strings[2]);
}

TEST_F(DL, ArrayString)
{
	char* TheStringArray[] = { (char*)"I like", (char*)"the", (char*)"1337 ", (char*)"cowbells of doom!" };
	StringArray Orig = { { TheStringArray, 4 } };
	StringArray* New;

	StringArray Loaded[10]; // this is so ugly!

	DoTheRoundAbout(Ctx, StringArray::TYPE_ID, &Orig, Loaded);

	New = Loaded;

	EXPECT_STREQ(Orig.Strings[0], New->Strings[0]);
	EXPECT_STREQ(Orig.Strings[1], New->Strings[1]);
	EXPECT_STREQ(Orig.Strings[2], New->Strings[2]);
	EXPECT_STREQ(Orig.Strings[3], New->Strings[3]);
}

TEST_F(DL, BitField)
{
	TestBits Orig;
	TestBits New;

	Orig.Bit1 = 0;
	Orig.Bit2 = 2;
	Orig.Bit3 = 4;
	Orig.make_it_uneven = 17;
	Orig.Bit4 = 1;
	Orig.Bit5 = 0;
	Orig.Bit6 = 5;

	DoTheRoundAbout(Ctx, TestBits::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.Bit1, New.Bit1);
	EXPECT_EQ(Orig.Bit2, New.Bit2);
	EXPECT_EQ(Orig.Bit3, New.Bit3);
	EXPECT_EQ(Orig.make_it_uneven, New.make_it_uneven);
	EXPECT_EQ(Orig.Bit4, New.Bit4);
	EXPECT_EQ(Orig.Bit5, New.Bit5);
	EXPECT_EQ(Orig.Bit6, New.Bit6);
}

TEST_F(DL, MoreBits)
{
	MoreBits Orig;
	MoreBits New;

	Orig.Bit1 = 512;
	Orig.Bit2 = 1;

	DoTheRoundAbout(Ctx, MoreBits::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.Bit1, New.Bit1);
	EXPECT_EQ(Orig.Bit2, New.Bit2);
}

TEST_F(DL, 64BitBitfield)
{
	BitBitfield64 Orig;
	BitBitfield64 New;

	Orig.Package  = 2;
	Orig.FileType = 13;
	Orig.PathHash = 1337;
	Orig.FileHash = 0xDEADBEEF;

	DoTheRoundAbout(Ctx, BitBitfield64::TYPE_ID, &Orig, &New);

	EXPECT_EQ(Orig.Package,  New.Package);
	EXPECT_EQ(Orig.FileType, New.FileType);
	EXPECT_EQ(Orig.PathHash, New.PathHash);
	EXPECT_EQ(Orig.FileHash, New.FileHash);
}

TEST_F(DL, SimplePtr)
{
	Pods Pods = { 1, 2, 3, 4, 5, 6, 7, 8, 8.1f, 8.2 };
	SimplePtr  Orig = { &Pods, &Pods };
	SimplePtr* New;

	SimplePtr Loaded[64]; // this is so ugly!

	DoTheRoundAbout(Ctx, SimplePtr::TYPE_ID, &Orig, Loaded);

	New = Loaded;
	EXPECT_NE(Orig.Ptr1, New->Ptr1);
	EXPECT_EQ(New->Ptr1, New->Ptr2);

	EXPECT_EQ(Orig.Ptr1->i8,  New->Ptr1->i8);
	EXPECT_EQ(Orig.Ptr1->i16, New->Ptr1->i16);
	EXPECT_EQ(Orig.Ptr1->i32, New->Ptr1->i32);
	EXPECT_EQ(Orig.Ptr1->i64, New->Ptr1->i64);
	EXPECT_EQ(Orig.Ptr1->u8,  New->Ptr1->u8);
	EXPECT_EQ(Orig.Ptr1->u16, New->Ptr1->u16);
	EXPECT_EQ(Orig.Ptr1->u32, New->Ptr1->u32);
	EXPECT_EQ(Orig.Ptr1->u64, New->Ptr1->u64);
	EXPECT_EQ(Orig.Ptr1->f32, New->Ptr1->f32);
	EXPECT_EQ(Orig.Ptr1->f64, New->Ptr1->f64);
}

TEST_F(DL, PtrChain)
{
	PtrChain Ptr1 = { 1337,   0x0 };
	PtrChain Ptr2 = { 7331, &Ptr1 };
	PtrChain Ptr3 = { 13,   &Ptr2 };
	PtrChain Ptr4 = { 37,   &Ptr3 };

	PtrChain* New;

	PtrChain Loaded[10]; // this is so ugly!

	DoTheRoundAbout(Ctx, PtrChain::TYPE_ID, &Ptr4, Loaded);
	New = Loaded;

	EXPECT_NE(Ptr4.Next, New->Next);
	EXPECT_NE(Ptr3.Next, New->Next->Next);
	EXPECT_NE(Ptr2.Next, New->Next->Next->Next);
	EXPECT_EQ(Ptr1.Next, New->Next->Next->Next->Next); // should be equal, 0x0

	EXPECT_EQ(Ptr4.Int, New->Int);
	EXPECT_EQ(Ptr3.Int, New->Next->Int);
	EXPECT_EQ(Ptr2.Int, New->Next->Next->Int);
	EXPECT_EQ(Ptr1.Int, New->Next->Next->Next->Int);
}

TEST_F(DL, DoublePtrChain)
{
	// tests both circualar ptrs and reference to root-node!

	DoublePtrChain Ptr1 = { 1337, 0x0,   0x0 };
	DoublePtrChain Ptr2 = { 7331, &Ptr1, 0x0 };
	DoublePtrChain Ptr3 = { 13,   &Ptr2, 0x0 };
	DoublePtrChain Ptr4 = { 37,   &Ptr3, 0x0 };

	Ptr1.Prev = &Ptr2;
	Ptr2.Prev = &Ptr3;
	Ptr3.Prev = &Ptr4;

	DoublePtrChain* New;

	DoublePtrChain Loaded[10]; // this is so ugly!

	DoTheRoundAbout(Ctx, DoublePtrChain::TYPE_ID, &Ptr4, Loaded);
	New = Loaded;

	// Ptr4
	EXPECT_NE(Ptr4.Next, New->Next);
	EXPECT_EQ(Ptr4.Prev, New->Prev); // is a null-ptr
	EXPECT_EQ(Ptr4.Int,  New->Int);

	// Ptr3
	EXPECT_NE(Ptr4.Next->Next, New->Next->Next);
	EXPECT_NE(Ptr4.Next->Prev, New->Next->Prev);
	EXPECT_EQ(Ptr4.Next->Int,  New->Next->Int);
	EXPECT_EQ(New,             New->Next->Prev);

	// Ptr2
	EXPECT_NE(Ptr4.Next->Next->Next, New->Next->Next->Next);
	EXPECT_NE(Ptr4.Next->Next->Prev, New->Next->Next->Prev);
	EXPECT_EQ(Ptr4.Next->Next->Int,  New->Next->Next->Int);
	EXPECT_EQ(New->Next,             New->Next->Next->Prev);

	// Ptr1
	EXPECT_EQ(Ptr4.Next->Next->Next->Next, New->Next->Next->Next->Next); // is null
	EXPECT_NE(Ptr4.Next->Next->Next->Prev, New->Next->Next->Next->Prev);
	EXPECT_EQ(Ptr4.Next->Next->Next->Int,  New->Next->Next->Next->Int);
	EXPECT_EQ(New->Next->Next,             New->Next->Next->Next->Prev);
}

TEST_F(DL, Enum)
{
	EXPECT_EQ(TESTENUM2_VALUE2 + 1, TESTENUM2_VALUE3); // value3 is after value2 but has no value. It sohuld automticallay be one bigger!

	TestingEnum Inst;
	Inst.TheEnum = TESTENUM1_VALUE3;

	TestingEnum Loaded;

	DoTheRoundAbout(Ctx, TestingEnum::TYPE_ID, &Inst, &Loaded);

	EXPECT_EQ(Inst.TheEnum, Loaded.TheEnum);
}

TEST_F(DL, EnumInlineArray)
{
	InlineArrayEnum Inst = { { TESTENUM2_VALUE1, TESTENUM2_VALUE2, TESTENUM2_VALUE3, TESTENUM2_VALUE4 } };
	InlineArrayEnum Loaded;

	DoTheRoundAbout(Ctx, InlineArrayEnum::TYPE_ID, &Inst, &Loaded);

	EXPECT_EQ(Inst.EnumArr[0], Loaded.EnumArr[0]);
	EXPECT_EQ(Inst.EnumArr[1], Loaded.EnumArr[1]);
	EXPECT_EQ(Inst.EnumArr[2], Loaded.EnumArr[2]);
	EXPECT_EQ(Inst.EnumArr[3], Loaded.EnumArr[3]);
}

TEST_F(DL, EnumArray)
{
	TestEnum2 Data[8] = { TESTENUM2_VALUE1, TESTENUM2_VALUE2, TESTENUM2_VALUE3, TESTENUM2_VALUE4, TESTENUM2_VALUE4, TESTENUM2_VALUE3, TESTENUM2_VALUE2, TESTENUM2_VALUE1 } ;
	ArrayEnum Inst = { { Data, 8 } };
	ArrayEnum* New;

	uint32 Loaded[1024]; // this is so ugly!

	DoTheRoundAbout(Ctx, ArrayEnum::TYPE_ID, &Inst, &Loaded);

	New = (ArrayEnum*)&Loaded[0];

	EXPECT_EQ(Inst.EnumArr.nCount, New->EnumArr.nCount);
	EXPECT_EQ(Inst.EnumArr[0],       New->EnumArr[0]);
	EXPECT_EQ(Inst.EnumArr[1],       New->EnumArr[1]);
	EXPECT_EQ(Inst.EnumArr[2],       New->EnumArr[2]);
	EXPECT_EQ(Inst.EnumArr[3],       New->EnumArr[3]);
	EXPECT_EQ(Inst.EnumArr[4],       New->EnumArr[4]);
	EXPECT_EQ(Inst.EnumArr[5],       New->EnumArr[5]);
	EXPECT_EQ(Inst.EnumArr[6],       New->EnumArr[6]);
	EXPECT_EQ(Inst.EnumArr[7],       New->EnumArr[7]);
}

TEST_F(DL, EmptyPodArray)
{
	PodArray1 Inst = { { NULL, 0 } };
	PodArray1 Loaded;

	DoTheRoundAbout(Ctx, PodArray1::TYPE_ID, &Inst, &Loaded);

	EXPECT_EQ(0u,  Loaded.Array.nCount);
	EXPECT_EQ(0x0, Loaded.Array.pData);
}

TEST_F(DL, EmptyStructArray)
{
	StructArray1 Inst = { { NULL, 0 } };
	StructArray1 Loaded;

	DoTheRoundAbout(Ctx, StructArray1::TYPE_ID, &Inst, &Loaded);

	EXPECT_EQ(0u,  Loaded.Array.nCount);
	EXPECT_EQ(0x0, Loaded.Array.pData);
}

TEST_F(DL, EmptyStringArray)
{
	StringArray Inst = { { NULL, 0 } };
	StringArray Loaded;

	DoTheRoundAbout(Ctx, StructArray1::TYPE_ID, &Inst, &Loaded);

	EXPECT_EQ(0u,  Loaded.Strings.nCount);
	EXPECT_EQ(0x0, Loaded.Strings.pData);
}

TEST_F(DL, BugTest1)
{
	// There was some error packing arrays ;)

	BugTest1_InArray Arr[3] = { { 1337, 1338, 18 }, { 7331, 8331, 19 }, { 31337, 73313, 20 } } ;
	BugTest1 Inst;
	Inst.Arr.pData  = Arr;
	Inst.Arr.nCount = DL_ARRAY_LENGTH(Arr);

	BugTest1 Loaded[10];

	DoTheRoundAbout(Ctx, BugTest1::TYPE_ID, &Inst, &Loaded);

	EXPECT_EQ(Arr[0].u64_1, Loaded[0].Arr[0].u64_1);
	EXPECT_EQ(Arr[0].u64_2, Loaded[0].Arr[0].u64_2);
	EXPECT_EQ(Arr[0].u16,   Loaded[0].Arr[0].u16);
	EXPECT_EQ(Arr[1].u64_1, Loaded[0].Arr[1].u64_1);
	EXPECT_EQ(Arr[1].u64_2, Loaded[0].Arr[1].u64_2);
	EXPECT_EQ(Arr[1].u16,   Loaded[0].Arr[1].u16);
	EXPECT_EQ(Arr[2].u64_1, Loaded[0].Arr[2].u64_1);
	EXPECT_EQ(Arr[2].u64_2, Loaded[0].Arr[2].u64_2);
	EXPECT_EQ(Arr[2].u16,   Loaded[0].Arr[2].u16);
}

template<typename T>
const char* ArrayToString(T* _pArr, unsigned int _Count, char* pBuffer, unsigned int nBuffer)
{
	unsigned int Pos = snprintf(pBuffer, nBuffer, "{ %f", _pArr[0]);

	for(unsigned int i = 1; i < _Count && Pos < nBuffer; ++i)
	{
		Pos += snprintf(pBuffer + Pos, nBuffer - Pos, ", %f", _pArr[i]);
	}

	snprintf(pBuffer + Pos, nBuffer - Pos, " }");
	return pBuffer;
}

// TODO: Move this to other file!
#define EXPECT_ARRAY_EQ(_Count, _Expect, _Actual) \
	do \
	{ \
		bool WasEq = true; \
		for(uint i = 0; i < _Count && WasEq; ++i) \
			WasEq = _Expect[i] == _Actual[i]; \
		char ExpectBuf[1024]; \
		char ActualBuf[1024]; \
		char Err[2048]; \
		snprintf(Err, DL_ARRAY_LENGTH(Err), "Arrays diffed!\nExpected:\n%s\nActual:\n%s", \
											ArrayToString(_Expect, _Count, ExpectBuf, DL_ARRAY_LENGTH(ExpectBuf)), \
											ArrayToString(_Actual, _Count, ActualBuf, DL_ARRAY_LENGTH(ActualBuf))); \
		EXPECT_TRUE(WasEq) << Err; \
	} while (false);

TEST_F(DL, BugTest2)
{
	// some error converting from 32-bit-data to 64-bit.

	BugTest2_WithMat Arr[2] = { { 1337, { 1.0f, 0.0f, 0.0f, 0.0f,
										  0.0f, 1.0f, 0.0f, 0.0f,
										  0.0f, 0.0f, 1.0f, 0.0f,
										  0.0f, 0.0f, 1.0f, 0.0f } },

								{ 7331, { 0.0f, 0.0f, 0.0f, 1.0f,
										  0.0f, 0.0f, 1.0f, 0.0f,
										  0.0f, 1.0f, 0.0f, 0.0f,
										  1.0f, 0.0f, 0.0f, 0.0f } } };
	BugTest2 Inst;
	Inst.Instances.pData  = Arr;
	Inst.Instances.nCount = 2;

	BugTest2 Loaded[40];

	DoTheRoundAbout(Ctx, BugTest2::TYPE_ID, &Inst, &Loaded);

 	EXPECT_EQ(Arr[0].iSubModel, Loaded[0].Instances[0].iSubModel);
 	EXPECT_EQ(Arr[1].iSubModel, Loaded[0].Instances[1].iSubModel);
 	EXPECT_ARRAY_EQ(16, Arr[0].Transform, Loaded[0].Instances[0].Transform);
 	EXPECT_ARRAY_EQ(16, Arr[1].Transform, Loaded[0].Instances[1].Transform);
}

TEST(DLMisc, EndianIsCorrect)
{
	// Test that DL_ENDIAN_HOST is set correctly
	union
	{
		uint32 i;
		unsigned char c[4];
	} test;
	test.i = 0x01020304;

	if( DL_ENDIAN_HOST == DL_ENDIAN_LITTLE )
	{
		EXPECT_EQ(4, test.c[0]);
		EXPECT_EQ(3, test.c[1]);
		EXPECT_EQ(2, test.c[2]);
		EXPECT_EQ(1, test.c[3]);
	}
	else
	{
		EXPECT_EQ(1, test.c[0]);
		EXPECT_EQ(2, test.c[1]);
		EXPECT_EQ(3, test.c[2]);
		EXPECT_EQ(4, test.c[3]);
	}
}

TEST(DLUtil, ErrorToString)
{
	for(dl_error_t Err = DL_ERROR_OK; Err < DL_ERROR_INTERNAL_ERROR; Err = dl_error_t(uint(Err) + 1))
		EXPECT_STRNE("Unknown error!", dl_error_to_string(Err));
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
