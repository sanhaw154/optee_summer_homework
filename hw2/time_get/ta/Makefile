CFG_TEE_TA_LOG_LEVEL ?= 4
CFG_TA_OPTEE_CORE_API_COMPAT_1_1=y

# The UUID for the Trusted Application
BINARY=1ef8a69c-368c-5065-80ed-a7e6e87427cf


TA_SIGN_KEY = /home/sanhow/optee-signing/subkeys/example/example.pem
TA_SUBKEY_ARGS = --subkey /home/sanhow/optee-signing/subkeys/example/example.bin --name time_get
TA_SUBKEY_DEPS = /home/sanhow/optee-signing/subkeys/example/example.bin
# orignal = 8ad93514-9898-4b4e-a15a-70dbf647d208
-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk



ifeq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk), )
clean:
	@echo 'Note: $$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk not found, cannot clean TA'
	@echo 'Note: TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR)'
endif
