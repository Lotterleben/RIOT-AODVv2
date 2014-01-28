MODULE:= $(shell basename $(CURDIR))

export INCLUDES += -I${RIOTBASE}/core/include/ \
				   -I$(RIOTBASE)/sys/net/include \
				   -I$(RIOTBASE)/drivers/cc110x/ \
				   -I$(RIOTBASE)/cpu/$(BOARD)/include \
				   -I$(OONFBASE)/src-api -I$(RIOTLIBS) \
				   -I./include

include $(RIOTBASE)/Makefile.base
