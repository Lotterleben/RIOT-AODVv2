#include "sender.h"
#include "routing.h" 

void aodv_init(void){

    init_routingtable();
    sender_init();

}