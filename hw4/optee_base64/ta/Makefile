CFG_TEE_TA_LOG_LEVEL ?= 4
CFG_TA_OPTEE_CORE_API_COMPAT_1_1=y

# The UUID for the Trusted Application
BINARY = 7843cb18-3601-59c1-8b47-9ef01ea02849


TA_SIGN_KEY = /home/sanhow/optee-signing/subkeys/base_64/base_64.pem
TA_SUBKEY_ARGS = --subkey /home/sanhow/optee-signing/subkeys/base_64/base_64.bin --name base_64
TA_SUBKEY_DEPS = /home/sanhow/optee-signing/subkeys/base_64/base_64.bin

-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk



ifeq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk), )
clean:
	@echo 'Note: $$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk not found, cannot clean TA'
	@echo 'Note: TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR)'
endif
