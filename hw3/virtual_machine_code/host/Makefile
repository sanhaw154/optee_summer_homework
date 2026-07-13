CC ?= $(CROSS_COMPILE)gcc

BINARY = optee_example_virtual_machine_code
OBJS = main.o

OPTEE_CLIENT_DIR ?= $(HOME)/optee-qemu-v8/optee_client

CFLAGS += -Wall
CFLAGS += -I../ta/include
CFLAGS += -I$(OPTEE_CLIENT_DIR)/libteec/include

LDADD += -L$(OPTEE_CLIENT_DIR)/out/libteec -lteec

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) -o $@ $^ $(LDADD)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY)
