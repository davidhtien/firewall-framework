import socket
import struct
import time

def bypass(pkt):
    # The example code here prints out the source/destination IP addresses
    # and then passes the packet
    src_ip = pkt[12+14:16+14] #+14 is size of ethernet header
    dst_ip = pkt[16+14:20+14]
    ipid, = struct.unpack('!H', pkt[4+14:6+14])    # IP identifier (big endian)

    print 'len=%4dB, IPID=%5d  %15s -> %15s' % (len(pkt), ipid,
            socket.inet_ntoa(src_ip), socket.inet_ntoa(dst_ip))

    return 1 #pass the packet


# def handle(pkt):
# do hard packet processing here
# and don't forget to change the parameters to process_packet in main
# should be 
#	process_packet("handle_pkt", "handle", frame, size_rcvd);