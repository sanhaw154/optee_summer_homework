// SPDX-License-Identifier: BSD-2-Clause

#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <tee_client_api.h>
#include <base_64_ta.h>

#define INPUT_MAX 1024

/*
 * 最大輸入為 INPUT_MAX - 1 bytes。
 * Base64 encode 最大輸出：
 *
 *     4 * ceil(input_size / 3)
 *
 * 額外保留一個 byte，方便需要時加入 '\0'。
 */
#define OUTPUT_MAX \
	(((((INPUT_MAX - 1) + 2) / 3) * 4) + 1) //use n+(k-1)/k formula to calculate the output size

static int read_input(char *buffer, size_t capacity, size_t *input_size)
{
	size_t length;

	if (!buffer || capacity == 0 || !input_size)
		return -1;

	printf("Input: ");
	fflush(stdout);

	if (!fgets(buffer, capacity, stdin)) {
		fprintf(stderr, "Failed to read input\n");
		return -1;
	}

	length = strlen(buffer);

	/*
	 * 如果 buffer 最後沒有換行，而且 stdin 還沒到 EOF，
	 * 代表輸入超過 buffer 容量而遭到截斷。
	 */
	if (length > 0 &&
	    buffer[length - 1] != '\n' &&
	    !feof(stdin)) {
		fprintf(stderr,
			"Input too long; maximum is %zu bytes\n",
			capacity - 1);
		return -1;
	}

	/*
	 * 移除 Linux 的 '\n' 或 Windows 的 "\r\n"。
	 */
	buffer[strcspn(buffer, "\r\n")] = '\0';
	*input_size = strlen(buffer);

	if (*input_size == 0) {
		fprintf(stderr, "Input cannot be empty\n");
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_UUID uuid = TA_BASE_64_UUID;
	TEEC_Operation operation = { 0 };

	uint32_t error_origin = 0;
	uint32_t command_id;

	char input[INPUT_MAX];
	uint8_t output[OUTPUT_MAX];

	size_t input_size;
	size_t result_size;

	if (argc != 2) {
		fprintf(stderr,
			"Usage: %s --encode | --decode\n",
			argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "--encode") == 0) {
		command_id = TA_CMD_BASE64_ENCODE;
	} else if (strcmp(argv[1], "--decode") == 0) {
		command_id = TA_CMD_BASE64_DECODE;
	} else {
		fprintf(stderr, "Unknown option: %s\n", argv[1]);
		fprintf(stderr,
			"Usage: %s --encode | --decode\n",
			argv[0]);
		return 1;
	}

	if (read_input(input, sizeof(input), &input_size) != 0)
		return 1;

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS) {
		errx(1,
		     "TEEC_InitializeContext failed: res=0x%x",
		     res);
	}

	/*
	 * TA_OpenSessionEntryPoint() 預期四個 NONE，
	 * 因此 operation 參數傳 NULL。
	 */
	res = TEEC_OpenSession(&ctx,
			       &sess,
			       &uuid,
			       TEEC_LOGIN_PUBLIC,
			       NULL,
			       NULL,
			       &error_origin);

	if (res != TEEC_SUCCESS) {
		TEEC_FinalizeContext(&ctx);

		errx(1,
		     "TEEC_OpenSession failed: "
		     "res=0x%x, origin=0x%x",
		     res,
		     error_origin);
	}

	operation.paramTypes =
		TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
				 TEEC_MEMREF_TEMP_OUTPUT,
				 TEEC_NONE,
				 TEEC_NONE);

	operation.params[0].tmpref.buffer = input;
	operation.params[0].tmpref.size = input_size;

	operation.params[1].tmpref.buffer = output;
	operation.params[1].tmpref.size = sizeof(output);

	res = TEEC_InvokeCommand(&sess,
				 command_id,
				 &operation,
				 &error_origin);

	if (res != TEEC_SUCCESS) {
		/*
		 * SHORT_BUFFER 時，TA 會將需要的容量寫回 size。
		 */
		if (res == TEEC_ERROR_SHORT_BUFFER) {
			fprintf(stderr,
				"Output buffer too small; "
				"required=%zu bytes\n",
				operation.params[1].tmpref.size);
		}

		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);

		errx(1,
		     "TEEC_InvokeCommand failed: "
		     "cmd=%u, res=0x%x, origin=0x%x",
		     command_id,
		     res,
		     error_origin);
	}

	result_size = operation.params[1].tmpref.size;

	if (result_size > sizeof(output)) {
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);

		errx(1,
		     "TA returned invalid output size: %zu",
		     result_size);
	}

	/*
	 * 使用 fwrite()，所以 decode 結果即使包含 0x00，
	 * 仍會依照 result_size 輸出。
	 */
	printf("Output: ");

	if (result_size > 0)
		fwrite(output, 1, result_size, stdout);

	putchar('\n');

	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return 0;
}