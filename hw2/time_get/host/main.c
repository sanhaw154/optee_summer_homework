#include <err.h>
#include <stdio.h>
#include <string.h>

#include <tee_client_api.h>
#include <time_get_ta.h>

static void invoke_time(TEEC_Session *sess, uint32_t cmd_id,
			const char *name)
{
	TEEC_Result res;
	TEEC_Operation op;
	uint32_t err_origin;
	uint64_t seconds;
	uint32_t millis;

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT,
					 TEEC_NONE,
					 TEEC_NONE,
					 TEEC_NONE);

	res = TEEC_InvokeCommand(sess, cmd_id, &op, &err_origin);
	if (res != TEEC_SUCCESS) {
		errx(1, "TEEC_InvokeCommand failed, cmd=%u, res=0x%x, origin=0x%x",
		     cmd_id, res, err_origin);
	}

	seconds = op.params[0].value.a;
	millis = op.params[0].value.b;

	printf("%s: %llu.%03u seconds\n",
	       name, (unsigned long long)seconds, millis);
}

int main(void)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_UUID uuid = TA_TIME_GET_UUID;
	uint32_t err_origin;

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_OpenSession failed with code 0x%x origin 0x%x",
		     res, err_origin);

	invoke_time(&sess, TA_TIME_GET_CMD_UNIX_TIME, "Unix time");
	invoke_time(&sess, TA_TIME_GET_CMD_UPTIME, "TEE uptime");

	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return 0;
}
