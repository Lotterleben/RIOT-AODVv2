valgrind: CFLAGS += -g
valgrind: CFLAGS += -DHAVE_VALGRIND_VALGRIND_H

# name of your project
export PROJECT = aodvv2
#
# for easy switching of boards
export BOARD = native

# this has to be the absolute path of the RIOT-base dir
export RIOTBASE =$(CURDIR)/../../riot/RIOT
export RIOTBOARD =$(RIOTBASE)/../boards
export RIOTLIBS =$(RIOTBASE)/../riot-libs
export OONFBASE=$(CURDIR)/../../oonf_api

ifeq (,$(findstring native,$(BOARD)))
include $(RIOTBASE)/../Makefile.unsupported
else

EXTERNAL_MODULES +=$(OONFBASE)
EXTERNAL_MODULES +=$(RIOTLIBS)
export EXTERNAL_MODULES

## Modules to include. 

# use auto_init with care, some modules still need to be initialized
# and most get confused if initialized twice
USEMODULE += auto_init

USEMODULE += config
USEMODULE += hwtimer
USEMODULE += ltc4150 # depends on hwtimer
USEMODULE += nativenet
USEMODULE += posix
USEMODULE += ps
USEMODULE += rtc
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += transceiver
USEMODULE += uart0
USEMODULE += vtimer

# oonf
USEMODULE += net_help 
USEMODULE += sixlowpan
USEMODULE += cunit
USEMODULE += compat_misc
USEMODULE += oonf_common
USEMODULE += oonf_rfc5444

#export INCLUDES +=	-I$(RIOTBOARD)/$(BOARD)/include -I$(RIOTBASE)/core/include \
#					-I$(RIOTBASE)/sys/include -I$(RIOTBASE)/drivers/include \
#					-I$(RIOTBASE)/cpu/$(BOARD)/include -I$(RIOTBASE)/sys/lib \
#					-I$(RIOTBASE)/sys/net/destiny/include -I.. \
#					-I$(OONFBASE)/src-api -I$(RIOTLIBS)

export INCLUDES +=	-I$(RIOTBASE)/sys/include -I$(RIOTBASE)/drivers/include \
					-I$(RIOTBASE)/cpu/$(BOARD)/include \
					-I$(RIOTBASE)/sys/net/include \
					-I$(OONFBASE)/src-api -I$(RIOTLIBS) \

include $(RIOTBASE)/Makefile.include
valgrind: all
endif
