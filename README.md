# AODVv2 for RIOT OS
*Please note:* The actual implementation of AODVv2 for RIOT has been moved [here](https://github.com/Lotterleben/RIOT). After checking out the ``aodvv2-final`` branch, you can find the code at ``RIOT/sys/net/routing/aodvv2``.   
This implementation is work in progress. It is in no way complete. You can find a description of the subset of the [AODVV2 Draft](http://tools.ietf.org/id/draft-ietf-manet-aodvv2) I implemented so far at ``docs/aodvv2_minimal.txt``. 

## usage
You can find a small demo application using aodvv2 in ``aodvv2/demo_application``.
To use it, run the following commands:
	
	cd aodvv2/demo_application
	../../riot/RIOT/cpu/native/tapsetup.sh create
	make
	sudo ./bin/native/demo_application.elf <tap device>

A small script to run tests on a [desvirt](https://github.com/mehlis/desvirt) grid can be found in the ``vnet_tester`` directory.  


You can find more info about RIOT applications and how to run them at the [RIOT OS Wiki](https://github.com/RIOT-OS/RIOT/wiki/Creating-your-first-RIOT-project).
