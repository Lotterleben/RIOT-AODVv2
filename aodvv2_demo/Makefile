
# name of your project
export APPLICATION = aodvv2_demo

# If no BOARD is found in the environment, use this default:
export BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
export RIOTBASE =$(CURDIR)/../../riot/RIOT

USEPKG += oonf_api
# note: DTEST_SETUP output *WILL BREAK* if you enable DEBUG currently.
export CFLAGS += -DRIOT -DTEST_SETUP #-DENABLE_DEBUG

# Uncomment these lines if you want to use platform support from external
# repositories:
#export RIOTCPU ?= $(CURDIR)/../../RIOT/thirdparty_cpu
#export RIOTBOARD ?= $(CURDIR)/../../RIOT/thirdparty_boards

# Uncomment this to enable scheduler statistics for ps:
#CFLAGS += -DSCHEDSTATISTICS

# If you want to use valgrind, you should recompile native with either
# HAVE_VALGRIND_H or HAVE_VALGRIND_VALGRIND_H depending on the location
# of the valgrind header (i.e. <valgrind.h> or <valgrind/valgrind.h>)
# For more information about the valgrind support of RIOT read:
# RIOT/cpu/native/README
CFLAGS += -DHAVE_VALGRIND_VALGRIND_H -g
#CFLAGS += -DHAVE_VALGRIND_H

# Uncomment this to enable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
#CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
export QUIET ?= 1

# Modules to include:
#USEMODULE += auto_init
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += uart0
USEMODULE += posix
USEMODULE += ps
USEMODULE += vtimer
USEMODULE += config

ifeq ($(strip $(BOARD)),native)
	USEMODULE += nativenet
else
	USEMODULE += cc110x
endif
USEMODULE += sixlowpan
USEMODULE += udp

#aodv
USEMODULE += aodvv2

# we need this to use print_routingtable()
export INCLUDES += -I${RIOTBASE}/sys/net/routing/aodvv2/

include $(RIOTBASE)/Makefile.include
