#include "include/aodvv2.h"
#include "sender.h"
#include "routing.h" 
#include "utils.h"

void aodv_init(void)
{
    struct netaddr my_ip;
    //netaddr_from_string(&my_ip, MY_IP);

    init_routingtable();
    init_clienttable();
    init_rreqtable();
    // TODO: find out own ip (damnit...) and set properly
    add_client(&my_ip, 0);
    sender_init();

}