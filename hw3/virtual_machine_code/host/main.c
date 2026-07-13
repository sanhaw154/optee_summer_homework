#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <tee_client_api.h> //position : /home/sanhow/optee-qemu-v8/optee_client/libteec/include/tee_client_api.h

#include <virtual_machine_code_ta.h>  //lib position : ~/optee-qemu-v8/optee_client/out/libteec

static void operation(TEEC_Session *sess, uint32_t cmd_id, uint32_t instruction , const char *name)
{
	TEEC_Result res;
	TEEC_Operation op;
	uint32_t err_origin;
	

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_VALUE_OUTPUT,
					 TEEC_NONE,
					 TEEC_NONE);

	op.params[0].value.a = instruction;

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
	TEEC_UUID uuid = TA_VIRTUAL_MACHINE_CODE_UUID;
	uint32_t err_origin;
	uint32_t instruction;

	

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);

	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_OpenSession failed with code 0x%x origin 0x%x",
		     res, err_origin);


	printf("Please enter the instruction:\n");

	if (scanf("%u", &instruction) != 1)
    errx(1, "Invalid input");

	if (instruction > 0xFFFF)
    errx(1, "Instruction must be a 16-bit value");

	operation(&sess, TA_VIRTUAL_MACHINE_CODE_CMD_EXECUTE,instruction, "execute");


	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return 0;
}