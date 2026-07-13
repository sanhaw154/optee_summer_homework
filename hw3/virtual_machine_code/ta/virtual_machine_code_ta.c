// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2016, Linaro Limited
 * All rights reserved.
 */

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <virtual_machine_code_ta.h>

/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	DMSG("has been called");

	return TEE_SUCCESS;
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void)
{
	DMSG("has been called");
}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA. In this function you will normally do the global initialization for the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
				    TEE_Param __unused params[4],
				    void __unused **sess_ctx)

{
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);


    IMSG("TA_OpenSessionEntryPoint has been called");
	IMSG("param_types = 0x%x, expected = 0x%x",
	     param_types, exp_param_types);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
	
    (void)params;
    (void)sess_ctx;

	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void __unused *sess_ctx)
{
	(void)sess_ctx;
}

static TEE_Result execute(uint32_t param_types, TEE_Param params[4])
{
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                               TEE_PARAM_TYPE_VALUE_OUTPUT,
                                               TEE_PARAM_TYPE_NONE,
                                               TEE_PARAM_TYPE_NONE);

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    uint32_t instruction = params[0].value.a & 0xFFFF;
    uint32_t opcode = (instruction >> 12) & 0xF;
    uint32_t variable1 = (instruction >> 6) & 0x3F;
    uint32_t variable2 = instruction & 0x3F;

    switch (opcode) {
    case 0x1:
        params[1].value.a = variable1 + variable2;
        break;
    case 0x5:
        params[1].value.a = variable1 & variable2;
        break;
    default:
        params[1].value.a = 0;
        break;
    }

    return TEE_SUCCESS;
}




/*
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */
TEE_Result TA_InvokeCommandEntryPoint(void __unused *sess_ctx,
				      uint32_t cmd_id, uint32_t param_types,
				      TEE_Param params[4])

{
	switch(cmd_id) {
		case TA_VIRTUAL_MACHINE_CODE_CMD_EXECUTE:
    		return execute(param_types, params);
	}
	
	return TEE_ERROR_BAD_PARAMETERS;
}