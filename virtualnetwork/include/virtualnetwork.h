#include "socket_base/socket.h"

/**
 * Substitute for socket_base_sendto().
 * Some of the fields are ignored, but have been left in for easier portability.
 *
 * @param[in] s         The ID of the socket to send through. (will be ignored)
 * @param[in] buf       Buffer to send the data from.
 * @param[in] len       Length of buffer.
 * @param[in] flags     Will be ignored.
 * @param[in] to        IPv6 Address to send data to.
 * @param[in] tolen     Length of address in *to* in byte (always 16).
 *
 * @return Number of sent bytes, -1 on error.
 */
int virtualnetwork_sendto(int s, const void *buf, uint32_t len, int flags,
                              sockaddr6_t *to, socklen_t tolen);

/**
 * @brief   Substitute for ipv6_iface_set_routing_provider().
 *
 * @param   next_hop    function that returns the next hop to reach dest
 */
void virtualnetwork_set_routing_provider(ipv6_addr_t *(*next_hop)(ipv6_addr_t *dest));

/**
 * Substitute for socket_base_recvfrom().
 * Some of the fields are ignored, but have been left in for easier portability.
 *
 * @param[in] s         The ID of the socket to receive from.
 * @param[in] buf       Buffer to store received data in.
 * @param[in] len       Length of buffer.
 * @param[in] flags     Flags for possible later implementations (currently
 *                      unused).
 * @param[in] from      IPv6 Address of the data's sender.
 * @param[in] fromlen   Length of address in *from* in byte (always 16).
 *
 * @return Number of received bytes, -1 on error.
 */
int32_t socket_base_recvfrom(int s, void *buf, uint32_t len, int flags,
                                sockaddr6_t *from, socklen_t *fromlen);
