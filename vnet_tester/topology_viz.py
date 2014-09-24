'''
Visualize topologies created by desvirt to make debugging less of a pain
'''
import ast
import aodv_test

'''
Take something like
{(1, 2): ('4716', ('fe80::ff:fe00:60cd', '00-00-00-ff-fe-00-60-cd')), (3, 2): ('4721', ('fe80::ff:fe00:60dc', '00-00-00-ff-fe-00-60-dc')), (1, 3): ('4715', ('fe80::ff:fe00:60ca', '00-00-00-ff-fe-00-60-ca')), (3, 1): ('4722', ('fe80::ff:fe00:60df', '00-00-00-ff-fe-00-60-df')), (3, 3): ('4720', ('fe80::ff:fe00:60d9', '00-00-00-ff-fe-00-60-d9')), (3, 4): ('4723', ('fe80::ff:fe00:60e2', '00-00-00-ff-fe-00-60-e2')), (4, 4): ('4712', ('fe80::ff:fe00:60c1', '00-00-00-ff-fe-00-60-c1')), (1, 4): ('4717', ('fe80::ff:fe00:60d0', '00-00-00-ff-fe-00-60-d0')), (1, 1): ('4714', ('fe80::ff:fe00:60c7', '00-00-00-ff-fe-00-60-c7')), (2, 3): ('4719', ('fe80::ff:fe00:60d6', '00-00-00-ff-fe-00-60-d6')), (2, 1): ('4726', ('fe80::ff:fe00:60eb', '00-00-00-ff-fe-00-60-eb')), (4, 3): ('4725', ('fe80::ff:fe00:60e8', '00-00-00-ff-fe-00-60-e8')), (2, 2): ('4718', ('fe80::ff:fe00:60d3', '00-00-00-ff-fe-00-60-d3')), (4, 2): ('4724', ('fe80::ff:fe00:60e5', '00-00-00-ff-fe-00-60-e5')), (4, 1): ('4713', ('fe80::ff:fe00:60c4', '00-00-00-ff-fe-00-60-c4')), (2, 4): ('4711', ('fe80::ff:fe00:60be', '00-00-00-ff-fe-00-60-be'))}
and stuff it into graphviz for a topology image
'''
def prep_graphviz(info):
    riots_dict = ast.literal_eval(info)
    connections = [] # list all the neighbors withiut (a,b) (b,a) duplicates

    gv_str = "graph G {"

    for (i,j), (pid, (ip,lla)) in riots_dict.iteritems():
        ip_suffix = get_suffix(ip)
        neighbors = aodv_test.collect_neighbor_coordinates((i,j), i+1, j+1)
        for neighbor in neighbors:
            try:
                neighbor_suffix = get_suffix(riots_dict[neighbor][1][0])
                if (not (neighbor_suffix, ip_suffix) in connections):
                    connections.append((ip_suffix, neighbor_suffix))
                    gv_str += "\"%s\" -- \"%s\";\n" %(ip_suffix, neighbor_suffix)
            except:
                pass
                # this occurs because i+1, j+1 creates non-existing indices.
                # i'm a horrible, horrible person.

    gv_str += "}"
    return gv_str

def get_suffix(ip):
    return ":" + ip.split(":")[-1]

if __name__ == "__main__":
    main()