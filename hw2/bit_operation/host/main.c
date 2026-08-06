#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include <tee_client_api.h> //position : /home/sanhow/optee-qemu-v8/optee_client/libteec/include/tee_client_api.h

#include <bit_operation_ta.h>  //lib position : ~/optee-qemu-v8/optee_client/out/libteec

static void discard_input_line(void)
{
	int ch = 0;

	while ((ch = getchar()) != '\n' && ch != EOF)
		;
}

static void operation(TEEC_Session *sess, uint32_t cmd_id, uint32_t value, uint32_t bit_index, const char *name)
{
	TEEC_Result res;
	TEEC_Operation op;
	uint32_t err_origin;
	

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_VALUE_OUTPUT,
					 TEEC_NONE,
					 TEEC_NONE);

	op.params[0].value.a = value;
	op.params[0].value.b = bit_index;

	res = TEEC_InvokeCommand(sess, cmd_id, &op, &err_origin);
	if (res != TEEC_SUCCESS) {
		errx(1, "TEEC_InvokeCommand failed, cmd=%u, res=0x%x, origin=0x%x",
		     cmd_id, res, err_origin);
	}

	uint32_t result = op.params[1].value.a;
	
	printf("after the operation of %s: x = %u\n",
	       name, result);
}

int main(void)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_UUID uuid = TA_BIT_OPERATION_UUID;
	uint32_t err_origin;
	uint32_t choice;
	uint64_t input_x;
	uint32_t x; //能用64bit?
	uint32_t y;

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_OpenSession failed with code 0x%x origin 0x%x",
		     res, err_origin);

	do { //choose condition
		int ret = 0;

		printf("Please select the operation:\n");
		printf("1. Set Bit\n");
		printf("2. Clear Bit\n");
		printf("3. Inverse Bit\n");

		ret = scanf("%" SCNu32, &choice);

		if (ret == EOF) {
			printf("Input ended.\n");
			goto out;
		}

		if (ret != 1) {
			printf("Invalid input: please enter a number.\n");
			discard_input_line();
			continue;
		}

		if (choice < 1 || choice > 3)
			printf("Invalid choice, please try again.\n");

	} while (choice < 1 || choice > 3);

	do {
		int ret = 0;

		printf("Please enter x and y:\n");

		ret = scanf("%" SCNu64 " %" SCNu32, &input_x, &y);

		if (ret == EOF) {
			printf("Input ended.\n");
			goto out;
		}

		/*
		* 這裡必須是 2，因為預期成功解析 input_x 和 y
		* 兩個數值。
		*/
		if (ret != 2) {
			printf("Invalid input: please enter two numbers.\n");
			discard_input_line();
			continue;
		}

		if (input_x > UINT32_MAX)
			printf("Invalid x: range is 0 to 0xFFFFFFFF.\n");

		if (y >= 32)
			printf("Invalid y: range is 0 to 31.\n");

	} while (input_x > UINT32_MAX || y >= 32);

	x = (uint32_t)input_x;

	switch (choice) {
	case 1:
		operation(&sess, TA_BIT_OPERATION_CMD_SET_BIT,x,y, "setBit");
		break;
	case 2:
		operation(&sess, TA_BIT_OPERATION_CMD_CLEAR_BIT,x,y, "clearBit");
		break;
	case 3:
		operation(&sess, TA_BIT_OPERATION_CMD_INVERSE_BIT,x,y, "inverseBit");
		break;
	default:
		errx(1, "Invalid choice");
	}

	out:
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);
	return 0;
}