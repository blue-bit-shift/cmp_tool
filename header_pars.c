#include <stdlib.h>

#include <my_inttypes.h>

#include <cmp_io.h>
#include <cmp_debug.h>
#include <cmp_entity.h>

static void cmp_ent_parse_generic_header(struct cmp_entity *ent)
{
	uint32_t version_id, cmp_ent_size, original_size, cmp_mode_used,
		 model_value_used, model_id, model_counter, max_used_bits_version,
		 lossy_cmp_par_used, start_coarse_time, end_coarse_time;
	uint16_t start_fine_time, end_fine_time;
	enum cmp_data_type data_type;
	int raw_bit;

	version_id = cmp_ent_get_version_id(ent);
	if (version_id & CMP_TOOL_VERSION_ID_BIT) {
		uint16_t major = (version_id & 0x7FFF0000U) >> 16U;
		uint16_t minor = version_id & 0xFFFFU;

		debug_print("Compressed with cmp_tool version: %u.%02u\n", major, minor);
	} else
		debug_print("ICU ASW Version ID: %" PRIu32 "\n", version_id);

	cmp_ent_size = cmp_ent_get_size(ent);
	debug_print("Compression Entity Size: %" PRIu32 " byte\n", cmp_ent_size);

	original_size = cmp_ent_get_original_size(ent);
	debug_print("Original Data Size: %" PRIu32 " byte\n", original_size);

	start_coarse_time = cmp_ent_get_coarse_start_time(ent);
	debug_print("Compression Coarse Start Time: %" PRIu32 "\n", start_coarse_time);

	start_fine_time = cmp_ent_get_fine_start_time(ent);
	debug_print("Compression Fine Start Time: %d\n", start_fine_time);

	end_coarse_time = cmp_ent_get_coarse_end_time(ent);
	debug_print("Compression Coarse End Time: %" PRIu32 "\n", end_coarse_time);

	end_fine_time = cmp_ent_get_fine_end_time(ent);
	debug_print("Compression Fine End Time: %d\n", end_fine_time);

#ifdef HAS_TIME_H
	{
		struct tm epoch_date = PLATO_EPOCH_DATE;
		time_t time = my_timegm(&epoch_date) + start_coarse_time;

		debug_print("Data were compressed on (local time): %s", ctime(&time));
	}
#endif
	debug_print("The compression took %f second\n", end_coarse_time - start_coarse_time
		+ ((end_fine_time - start_fine_time)/256./256.));

	data_type = cmp_ent_get_data_type(ent);
	debug_print("Data Product Type: %d\n", data_type);

	raw_bit = cmp_ent_get_data_type_raw_bit(ent);
	debug_print("RAW bit in the Data Product Type is%s set\n", raw_bit ? "" : " not");

	cmp_mode_used = cmp_ent_get_cmp_mode(ent);
	debug_print("Used Compression Mode: %" PRIu32 "\n", cmp_mode_used);

	model_value_used = cmp_ent_get_model_value(ent);
	debug_print("Used Model Updating Weighing Value: %" PRIu32 "\n", model_value_used);

	model_id = cmp_ent_get_model_id(ent);
	debug_print("Model ID: %" PRIu32 "\n", model_id);

	model_counter = cmp_ent_get_model_counter(ent);
	debug_print("Model Counter: %" PRIu32 "\n", model_counter);

	max_used_bits_version = cmp_ent_get_max_used_bits_version(ent);
	debug_print("Maximum Used Bits Registry Version: %" PRIu32 "\n", max_used_bits_version);

	lossy_cmp_par_used = cmp_ent_get_lossy_cmp_par(ent);
	debug_print("Used Lossy Compression Parameters: %" PRIu32 "\n", lossy_cmp_par_used);
}
static void cmp_ent_parese_imagette_header(struct cmp_entity *ent)
{
	uint32_t spill_used, golomb_par_used;

	spill_used = cmp_ent_get_ima_spill(ent);
	debug_print("Used Spillover Threshold Parameter: %" PRIu32 "\n", spill_used);

	golomb_par_used = cmp_ent_get_ima_golomb_par(ent);
	debug_print("Used Golomb Parameter: %" PRIu32 "\n", golomb_par_used);
}
static void cmp_ent_parese_adaptive_imagette_header(struct cmp_entity *ent)
{
	uint32_t spill_used, golomb_par_used, ap1_spill_used,
		 ap1_golomb_par_used, ap2_spill_used, ap2_golomb_par_used;

	spill_used = cmp_ent_get_ima_spill(ent);
	debug_print("Used Spillover Threshold Parameter: %" PRIu32 "\n", spill_used);

	golomb_par_used = cmp_ent_get_ima_golomb_par(ent);
	debug_print("Used Golomb Parameter: %" PRIu32 "\n", golomb_par_used);

	ap1_spill_used = cmp_ent_get_ima_ap1_spill(ent);
	debug_print("Used Adaptive 1 Spillover Threshold Parameter: %" PRIu32 "\n", ap1_spill_used);

	ap1_golomb_par_used = cmp_ent_get_ima_ap1_golomb_par(ent);
	debug_print("Used Adaptive 1 Golomb Parameter: %" PRIu32 "\n", ap1_golomb_par_used);

	ap2_spill_used = cmp_ent_get_ima_ap2_spill(ent);
	debug_print("Used Adaptive 2 Spillover Threshold Parameter: %" PRIu32 "\n", ap2_spill_used);

	ap2_golomb_par_used = cmp_ent_get_ima_ap2_golomb_par(ent);
	debug_print("Used Adaptive 2 Golomb Parameter: %" PRIu32 "\n", ap2_golomb_par_used);
}

int main(int argc, char *argv[])
{
	ssize_t f_size;
	char *data_file_name = argv[1];
	int verbose_en = 0;
	void *data;

	f_size = read_file8(data_file_name, NULL, 0, verbose_en);
	if (f_size < 0)
		return 1;

	data = malloc(f_size);

	f_size = read_file8(data_file_name, data, f_size, verbose_en);
	if (f_size < 0)
		return 1;

	cmp_ent_parse(data);
	/* cmp_ent_parese_adaptive_imagette_header(data); */
	return 0;
}
