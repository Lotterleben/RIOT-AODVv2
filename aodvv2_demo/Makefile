# name of your application
APPLICATION = aodvv2_demo

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../riot/RIOT

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += ng_netif_default
USEMODULE += auto_init_ng_netif
USEMODULE += shell
USEMODULE += uart0
USEMODULE += ng_pktdump

# Specify the mandatory networking modules for IPv6 and UDP
USEMODULE += ng_ipv6_default
USEMODULE += ng_udp

CFLAGS += -DRIOT
USEMODULE += aodvv2

include $(RIOTBASE)/Makefile.include
