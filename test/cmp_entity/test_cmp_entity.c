#include <unity.h>

#include <cmp_entity.h>
#include <cmp_io.h>

void test_cmp_ent_get_data_buf(void)
{
	enum cmp_data_type data_type;/*TODO: implement: DATA_TYPE_F_CAM_OFFSET, DATA_TYPE_F_CAM_BACKGROUND */
	struct cmp_entity ent = {0};
	char *adr;
	uint32_t s, hdr_size;

	for (data_type = DATA_TYPE_IMAGETTE;
	     data_type <=DATA_TYPE_F_CAM_IMAGETTE_ADAPTIVE;
	     data_type++) {
		s = cmp_ent_create(&ent, data_type, 0, 0);
		TEST_ASSERT_NOT_EQUAL_INT(0, s);

		adr = cmp_ent_get_data_buf(&ent);
		TEST_ASSERT_NOT_NULL(adr);

		hdr_size = cmp_ent_cal_hdr_size(data_type, 0);
		TEST_ASSERT_EQUAL_INT(hdr_size, adr-(char *)&ent);
	}

	/* RAW mode test */
	for (data_type = DATA_TYPE_IMAGETTE;
	     data_type <=DATA_TYPE_F_CAM_IMAGETTE_ADAPTIVE;
	     data_type++) {
		s = cmp_ent_create(&ent, data_type, 1, 0);
		TEST_ASSERT_NOT_EQUAL_INT(0, s);

		adr = cmp_ent_get_data_buf(&ent);
		TEST_ASSERT_NOT_NULL(adr);

		hdr_size = cmp_ent_cal_hdr_size(data_type, 1);
		TEST_ASSERT_EQUAL_INT(hdr_size, adr-(char *)&ent);
	}

	/* ent = NULL test */
	adr = cmp_ent_get_data_buf(NULL);
	TEST_ASSERT_NULL(adr);

	/* compression data type not supported test */
	s = cmp_ent_set_data_type(&ent, DATA_TYPE_UNKNOWN, 0);
	TEST_ASSERT_FALSE(s);

	adr = cmp_ent_get_data_buf(&ent);
	TEST_ASSERT_NULL(adr);
}

