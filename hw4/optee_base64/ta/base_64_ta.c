// SPDX-License-Identifier: BSD-2-Clause

#include <stddef.h>
#include <stdint.h>

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <base_64_ta.h>

enum base64_result {
	BASE64_OK = 0,
	BASE64_BAD_PARAM = -1,
	BASE64_SHORT_BUFFER = -2,
	BASE64_INVALID_DATA = -3
};

static const char base64_table[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

static int base64_value(uint8_t character)
{
	if (character >= 'A' && character <= 'Z')
		return character - 'A';

	if (character >= 'a' && character <= 'z')
		return character - 'a' + 26;

	if (character >= '0' && character <= '9')
		return character - '0' + 52;

	if (character == '+')
		return 62;

	if (character == '/')
		return 63;

	return -1;
}

/*
 * Base64 encode。
 *
 * output_len：
 *   成功時表示實際輸出 bytes。
 *   SHORT_BUFFER 時表示需要的輸出容量。
 *
 * 函式不加入 '\0'，因為 Base64 TA 的輸出由 memref.size
 * 表示長度，不需要依賴 C string terminator。
 */
static int base64_enc(const uint8_t *input,
		      size_t input_len,
		      char *output,
		      size_t output_capacity,
		      size_t *output_len)
{
	size_t required;
	size_t input_offset = 0;
	size_t output_offset = 0;

	if ((!input && input_len != 0) || !output_len)
		return BASE64_BAD_PARAM;

	/*
	 * 避免 input_len + 2 overflow。
	 */
	if (input_len > SIZE_MAX - 2)
		return BASE64_BAD_PARAM;

	if (((input_len + 2) / 3) > SIZE_MAX / 4)
		return BASE64_BAD_PARAM;

	required = 4 * ((input_len + 2) / 3);
	*output_len = required;

	if (required == 0)
		return BASE64_OK;

	if (!output || output_capacity < required)
		return BASE64_SHORT_BUFFER;

	while (input_offset + 2 < input_len) {
		uint32_t value;

		value =
			((uint32_t)input[input_offset] << 16) |
			((uint32_t)input[input_offset + 1] << 8) |
			(uint32_t)input[input_offset + 2];

		output[output_offset++] =
			base64_table[(value >> 18) & 0x3f];

		output[output_offset++] =
			base64_table[(value >> 12) & 0x3f];

		output[output_offset++] =
			base64_table[(value >> 6) & 0x3f];

		output[output_offset++] =
			base64_table[value & 0x3f];

		input_offset += 3;
	}

	if (input_offset < input_len) {
		uint32_t value;

		value = (uint32_t)input[input_offset] << 16;

		output[output_offset++] =
			base64_table[(value >> 18) & 0x3f];

		if (input_offset + 1 < input_len) {
			value |=
				(uint32_t)input[input_offset + 1] << 8;

			output[output_offset++] =
				base64_table[(value >> 12) & 0x3f];

			output[output_offset++] =
				base64_table[(value >> 6) & 0x3f];

			output[output_offset++] = '=';
		} else {
			output[output_offset++] =
				base64_table[(value >> 12) & 0x3f];

			output[output_offset++] = '=';
			output[output_offset++] = '=';
		}
	}

	*output_len = output_offset;
	return BASE64_OK;
}

/*
 * Base64 decode。
 *
 * 此版本：
 * - 不接受空白或換行。
 * - 不接受 URL-safe Base64 的 '-' 和 '_'。
 * - 驗證 '=' padding 位置。
 * - 輸出是 bytes，不加入 '\0'。
 */
static int base64_dec(const char *input,
		      size_t input_len,
		      uint8_t *output,
		      size_t output_capacity,
		      size_t *output_len)
{
	size_t padding = 0;
	size_t required;
	size_t input_offset;
	size_t output_offset = 0;

	if ((!input && input_len != 0) || !output_len)
		return BASE64_BAD_PARAM;

	if (input_len == 0) {
		*output_len = 0;
		return BASE64_OK;
	}

	if ((input_len % 4) != 0)
		return BASE64_INVALID_DATA;

	if (input[input_len - 1] == '=')
		padding++;

	if (input[input_len - 2] == '=')
		padding++;

	required = (input_len / 4) * 3 - padding;
	*output_len = required;

	if (required != 0 &&
	    (!output || output_capacity < required))
		return BASE64_SHORT_BUFFER;

	for (input_offset = 0;
	     input_offset < input_len;
	     input_offset += 4) {
		int a;
		int b;
		int c;
		int d;
		int is_last;
		uint32_t value;

		is_last = input_offset + 4 == input_len;

		a = base64_value((uint8_t)input[input_offset]);
		b = base64_value((uint8_t)input[input_offset + 1]);

		if (a < 0 || b < 0)
			return BASE64_INVALID_DATA;

		if (input[input_offset + 2] == '=') {
			/*
			 * "xx==" 只能出現在最後一組。
			 */
			if (!is_last ||
			    input[input_offset + 3] != '=')
				return BASE64_INVALID_DATA;

			/*
			 * 只有 8 bits 資料時，b 的低 4 bits 必須為 0。
			 */
			if ((b & 0x0f) != 0)
				return BASE64_INVALID_DATA;

			c = 0;
			d = 0;
		} else {
			c = base64_value(
				(uint8_t)input[input_offset + 2]);

			if (c < 0)
				return BASE64_INVALID_DATA;

			if (input[input_offset + 3] == '=') {
				/*
				 * "xxx=" 只能出現在最後一組。
				 */
				if (!is_last)
					return BASE64_INVALID_DATA;

				/*
				 * 只有 16 bits 資料時，
				 * c 的低 2 bits 必須為 0。
				 */
				if ((c & 0x03) != 0)
					return BASE64_INVALID_DATA;

				d = 0;
			} else {
				d = base64_value(
					(uint8_t)input[input_offset + 3]);

				if (d < 0)
					return BASE64_INVALID_DATA;
			}
		}

		value =
			((uint32_t)a << 18) |
			((uint32_t)b << 12) |
			((uint32_t)c << 6) |
			(uint32_t)d;

		if (output_offset < required) {
			output[output_offset++] =
				(uint8_t)(value >> 16);
		}

		if (output_offset < required) {
			output[output_offset++] =
				(uint8_t)(value >> 8);
		}

		if (output_offset < required) {
			output[output_offset++] =
				(uint8_t)value;
		}
	}

	*output_len = output_offset;
	return BASE64_OK;
}

static TEE_Result base64_status_to_tee_result(int status)
{
	switch (status) {
	case BASE64_OK:
		return TEE_SUCCESS;

	case BASE64_BAD_PARAM:
		return TEE_ERROR_BAD_PARAMETERS;

	case BASE64_SHORT_BUFFER:
		return TEE_ERROR_SHORT_BUFFER;

	case BASE64_INVALID_DATA:
		return TEE_ERROR_BAD_FORMAT;

	default:
		return TEE_ERROR_GENERIC;
	}
}

static TEE_Result command_base64_encode(uint32_t param_types,
					TEE_Param params[4])
{
	const uint32_t expected_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);

	const uint8_t *input;
	char *output;
	size_t input_len;
	size_t output_capacity;
	size_t output_len = 0;
	int status;

	if (param_types != expected_types)
		return TEE_ERROR_BAD_PARAMETERS;

	input = params[0].memref.buffer;
	input_len = params[0].memref.size;

	output = params[1].memref.buffer;
	output_capacity = params[1].memref.size;

	if (!input && input_len != 0)
		return TEE_ERROR_BAD_PARAMETERS;

	status = base64_enc(input,
			    input_len,
			    output,
			    output_capacity,
			    &output_len);

	/*
	 * 成功時為實際輸出長度；
	 * SHORT_BUFFER 時為所需容量。
	 */
	params[1].memref.size = output_len;

	return base64_status_to_tee_result(status);
}

static TEE_Result command_base64_decode(uint32_t param_types,
					TEE_Param params[4])
{
	const uint32_t expected_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);

	const char *input;
	uint8_t *output;
	size_t input_len;
	size_t output_capacity;
	size_t output_len = 0;
	int status;

	if (param_types != expected_types)
		return TEE_ERROR_BAD_PARAMETERS;

	input = params[0].memref.buffer;
	input_len = params[0].memref.size;

	output = params[1].memref.buffer;
	output_capacity = params[1].memref.size;

	if (!input && input_len != 0)
		return TEE_ERROR_BAD_PARAMETERS;

	status = base64_dec(input,
			    input_len,
			    output,
			    output_capacity,
			    &output_len);

	params[1].memref.size = output_len;

	return base64_status_to_tee_result(status);
}

TEE_Result TA_CreateEntryPoint(void)
{
	DMSG("TA_CreateEntryPoint called");
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
	DMSG("TA_DestroyEntryPoint called");
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
				    TEE_Param params[4],
				    void **session_context)
{
	const uint32_t expected_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);

	DMSG("TA_OpenSessionEntryPoint called");

	if (param_types != expected_types)
		return TEE_ERROR_BAD_PARAMETERS;

	(void)params;
	*session_context = NULL;

	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *session_context)
{
	(void)session_context;
	DMSG("TA_CloseSessionEntryPoint called");
}

TEE_Result TA_InvokeCommandEntryPoint(void *session_context,
				      uint32_t command_id,
				      uint32_t param_types,
				      TEE_Param params[4])
{
	(void)session_context;

	DMSG("TA_InvokeCommandEntryPoint: command_id=%" PRIu32,
	     command_id);

	switch (command_id) {
	case TA_CMD_BASE64_ENCODE:
		return command_base64_encode(param_types, params);

	case TA_CMD_BASE64_DECODE:
		return command_base64_decode(param_types, params);

	default:
		return TEE_ERROR_NOT_SUPPORTED;
	}
}