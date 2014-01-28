#export CFLAGS += -DRIOT #-DENABLE_DEBUG

# name of your project
export PROJECT = aodvv2_node

# for easy switching of boards
export BOARD ?= native

# this has to be the absolute path of the RIOT-base dir
export RIOTBASE ?=$(CURDIR)/../riot/RIOT
export RIOTBOARD ?=$(RIOTBASE)/boards
export RIOTLIBS =$(RIOTBASE)/../riot-libs
export OONFBASE =$(CURDIR)/../oonf_api
export AODV_NODE =$(CURDIR)/aodvv2 

ifeq (,$(findstring native,$(BOARD)))
include $(RIOTBASE)/../Makefile.unsupported
else

EXTERNAL_MODULES +=$(OONFBASE)
EXTERNAL_MODULES +=$(RIOTLIBS)
EXTERNAL_MODULES +=$(AODV_NODE)
export EXTERNAL_MODULES

export CFLAGS = -DRIOT -DENABLE_NAME -ggdb

## Modules to include. 

# use auto_init with care, some modules still need to be initialized
# and most get confused if initialized twice
USEMODULE += auto_init

USEMODULE += config
USEMODULE += hwtimer
USEMODULE += nativenet
USEMODULE += posix
USEMODULE += ps
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += uart0
USEMODULE += vtimer

#tlayer
USEMODULE += destiny
USEMODULE += posix
USEMODULE += ltc4150

# oonf
USEMODULE += net_help 
USEMODULE += sixlowpan
USEMODULE += cunit
USEMODULE += compat_misc
USEMODULE += oonf_common
USEMODULE += oonf_rfc5444

#aodv
USEMODULE += aodvv2

export INCLUDES += -I${RIOTBASE}/core/include/ \
				   -I$(RIOTBASE)/sys/net/include \
				   -I$(RIOTBASE)/drivers/cc110x/ \
				   -I$(RIOTBASE)/cpu/$(BOARD)/include \
				   -I$(OONFBASE)/src-api -I$(RIOTLIBS) \
				   #-I$(AODV_NODE)/include

include $(RIOTBASE)/Makefile.include
valgrind: all
endif