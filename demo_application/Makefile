
# name of your project
export PROJECT = demo_application

# If no BOARD is found in the environment, use this default:
export BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
export RIOTBASE =$(CURDIR)/../../riot/RIOT

USEPKG += oonf_api
export CFLAGS += -DRIOT #-DENABLE_DEBUG

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
USEMODULE += auto_init
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += uart0
USEMODULE += posix
USEMODULE += ps 
USEMODULE += vtimer 
ifeq ($(strip $(BOARD)),native)
	USEMODULE += nativenet
else
	#USEMODULE += cc110x
	USEMODULE += cc110x_ng
endif

USEMODULE += sixlowpan
USEMODULE += rpl      #what?!
USEMODULE += destiny

USEMODULE += net_help 
USEMODULE += sixlowpan
#USEMODULE += cunit
#USEMODULE += compat_misc
#USEMODULE += oonf_common
#USEMODULE += oonf_rfc5444

#aodv
USEMODULE += aodvv2

export INCLUDES += -I$(RIOTBASE)/sys/net/include \
				   -I$(RIOTBASE)/sys/net/routing/aodvv2/include

# INCLUDES += -I${RIOTBASE}/core/include/         \
#				   -I$(RIOTBASE)/sys/net/include       \
#				   -I$(RIOTBASE)/drivers/cc110x/       \
#				   -I$(RIOTBASE)/cpu/$(BOARD)/include  \
#				   -I$(OONFBASE)/src-api -I$(RIOTLIBS) \
#				   -I$(AODV_NODE)                      \

include $(RIOTBASE)/Makefile.include
