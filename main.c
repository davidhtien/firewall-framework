#include <stdio.h>              /* printf, scanf, NULL */
#include <stdlib.h>             /* malloc, free, rand */
#include <stddef.h>
#include <unistd.h>             /* read */
#include <string.h>             /* strcpy, strcat */
#include <fcntl.h>              /* for open */
#include <errno.h>
#include <sys/types.h>          /* for sockets */
#include <sys/socket.h>         /* for sockets and open */
#include <sys/stat.h>           /* for open */
#include <sys/ioctl.h>          /* for ioctl */
#include <sys/time.h>           /* for struct timeval */
#include <sys/un.h>             /* unix domain sockets */  
#include <net/ethernet.h>       /* the L2 protocols */
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>         /* the L3 protocols */
#include <arpa/inet.h>          /* for htons */
#include <linux/if_packet.h>  

#define PKT_DIR_INCOMING 0
#define PKT_DIR_OUTGOING 1

#define MAX_FRAME 1514
#define HDR_SIZE 14
#define P_IP 0x0800

#define ETH_P_ALL 3

#define IFF_TAP 0x0002
#define IFF_NO_PI 0x1000
#define TUNSETIFF 0x400454CA

#define IFNAME_INT "int"
#define IFNAME_EXT "ext"
#define IP_GATEWAY "10.0.2.2"

typedef struct RegularInterface {
    int sockfd;
    char* eth_hdr;

    int (*fileno)(void *self);
    int (*recv_eth_frame)(void *self, char* buf);
    void (*send_ip_packet)(void *self, char* pkt);
    void (*set_eth_hdr)(void *self, char* eth_hdr);
    void (*send_eth_frame)(void *self, char* frame, int size);

} RegularInterface;

int fileno_RI(void* ri) {
    return ((RegularInterface*)ri)->sockfd;
}

int recv_eth_frame_RI(RegularInterface* ri, char* buf) {
    int n = recvfrom(ri->sockfd, buf, MAX_FRAME, 0, NULL, NULL);
    // printf("RI received:  %d bytes\n", n);
    if (n < 0) {
        printf("%d %s\n", __LINE__, strerror(errno));
        exit(1);
    }
    return n;
}

// not used see big comment at line 314
void send_ip_packet_RI(RegularInterface* ri, char* pkt) {
    char* full = malloc(HDR_SIZE+strlen(pkt));
    strcpy(full, ri->eth_hdr);
    strcat(full, pkt);
    (ri->send_eth_frame)(ri, full, HDR_SIZE+strlen(pkt));
}

void set_eth_hdr_RI(RegularInterface *ri, char* eth_hdr) {
    char* header = malloc(14*sizeof(char));
    memcpy(header, eth_hdr, 14);
    ri->eth_hdr = header;
    // must free header after closing program
}

void send_eth_frame_RI(RegularInterface* ri, char* frame, int size) {
    int n = send(ri->sockfd, frame, size, 0);
    // printf("RI sent:      %d bytes\n", n);
    if (n < 0 ) {
        printf("%d %s\n", __LINE__, strerror(errno));
        exit(1);
    }
}

RegularInterface* init_RI(char* name) {
    RegularInterface* ri = malloc(sizeof(RegularInterface));

    int fd;
    fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd == -1) {
        printf("%s\n", "error creating regular interface");
        exit(1);
    }

    //information concerning sockaddr_ll can be found at http://man7.org/linux/man-pages/man7/packet.7.html
    //most fields are found in /sys/class/net/*name of device*
    struct sockaddr_ll ri_addr;
    memset(&ri_addr, 0, sizeof(struct sockaddr_ll));
    ri_addr.sll_family = AF_PACKET;

    struct ifreq if_idx;
    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, name, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFINDEX, &if_idx) < 0) perror("SIOCGIFINDEX");
    ri_addr.sll_ifindex = if_idx.ifr_ifindex;

    struct ifreq if_mac;
    memset(&if_mac, 0, sizeof(struct ifreq));
    strncpy(if_mac.ifr_name, name, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFHWADDR, &if_mac) < 0) perror("SIOCGIFHWADDR");
    int i = 0;
    while (i < 6) {
        ri_addr.sll_addr[i] = ((unsigned char *)&if_mac.ifr_hwaddr.sa_data)[i];
        i++;
    }
 
    ri_addr.sll_halen = ETH_ALEN;

    int size = sizeof(struct sockaddr_ll);
    int n = bind(fd, (struct sockaddr *) &ri_addr, size);
    if (n < 0) {
        printf("%d %s\n", __LINE__, strerror(errno));
        exit(1);
    }

    ri->sockfd = fd;
    ri->fileno = &fileno_RI;
    ri->recv_eth_frame = &recv_eth_frame_RI;
    ri->send_ip_packet = &send_ip_packet_RI;
    ri->set_eth_hdr = &set_eth_hdr_RI;
    ri->send_eth_frame = &send_eth_frame_RI;

    return ri;
}

typedef struct TAPInterface {
    int sockfd;
    char* eth_hdr;

    int (*fileno)(void *self);
    int (*recv_eth_frame)(void *self, char* buf);
    void (*send_ip_packet)(void *self, char* pkt);
    void (*set_eth_hdr)(void *self, char* eth_hdr);
    void (*send_eth_frame)(void *self, char* frame, int size);

} TAPInterface;

int fileno_TAP(TAPInterface* ti) {
    return ti->sockfd;
}

int recv_eth_frame_TAP(TAPInterface* ti, char* buf) {
    int n = read((ti->fileno)(ti), buf, MAX_FRAME);
    // printf("TI received:  %d bytes\n", n);
    if (n < 0) {
        printf("%d %s\n", __LINE__, strerror(errno));
        exit(1);
    }
    return n;
}

// not used see big comment at line 314
void send_ip_packet_TAP(TAPInterface* ti, char* pkt) {
    char* full = malloc(HDR_SIZE+strlen(pkt));
    strcpy(full, ti->eth_hdr);
    strcat(full, pkt);
    (ti->send_eth_frame)(ti, full, HDR_SIZE+strlen(pkt));
}

void set_eth_hdr_TAP(TAPInterface* ti, char* eth_hdr) {
    char* header = malloc(14*sizeof(char));
    memcpy(header, eth_hdr, 14);
    ti->eth_hdr = header;
    // must free header after closing program
}

void send_eth_frame_TAP(TAPInterface* ti, char* frame, int size) {
    int n = write(ti->sockfd, frame, size);
    // printf("TI sent:      %d bytes\n", n);
    if (n < 0) {
        printf("%d %s\n", __LINE__, strerror(errno));
        exit(1);
    }
}   

TAPInterface* init_TI(char* name) {
    TAPInterface* ti = malloc(sizeof(TAPInterface));

    int fd;
    fd = open("/dev/net/tun", O_RDWR);
    if (fd == -1) {
        printf("%s\n", "could not open tun device");
        exit(1);
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strncpy(ifr.ifr_name, name, IFNAMSIZ);

    ioctl(fd, TUNSETIFF, (void *) &ifr);

    ti->sockfd = fd;
    ti->fileno = &fileno_TAP;
    ti->recv_eth_frame = &recv_eth_frame_TAP;
    ti->send_ip_packet = &send_ip_packet_TAP;
    ti->set_eth_hdr = &set_eth_hdr_TAP;
    ti->send_eth_frame = &send_eth_frame_TAP;

    return ti;
}

// allow the use of a python script to process packet
#include <Python.h> 

long py_process_packet(char* file, char* func, char* pkt, int size)
{
    PyObject *pName, *pModule, *pDict, *pFunc;
    PyObject *pArgs, *pValue;
    int i;

    setenv("PYTHONPATH",".",1);

    Py_Initialize();
    pName = PyString_FromString(file);
    /* Error checking of pName left out */

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, func);
        /* pFunc is a new reference */

        if (pFunc && PyCallable_Check(pFunc)) {
            pArgs = PyTuple_New(1);
            for (i = 0; i < 1; ++i) {
                pValue = PyString_FromStringAndSize(pkt, size);
                if (!pValue) {
                    Py_DECREF(pArgs);
                    Py_DECREF(pModule);
                    fprintf(stderr, "Cannot convert argument\n");
                    return 1;
                }
                /* pValue reference stolen here: */
                PyTuple_SetItem(pArgs, i, pValue);
            }
            pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
            if (pValue != NULL) {
                // printf("Result of call: %s\n", PyInt_AsLong(pValue));
                Py_DECREF(pValue);
            }
            else {
                Py_DECREF(pFunc);
                Py_DECREF(pModule);
                PyErr_Print();
                fprintf(stderr,"Call failed\n");
                return 1;
            }
        }
        else {
            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Cannot find function \"%s\"\n", func);
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
    else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", file);
        return 1;
    }
    Py_Finalize();
    return PyInt_AsLong(pValue);
}

int main(int argc, char *argv[]) {
    int dom_socket;
    dom_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (dom_socket == -1) {
        printf("%s\n", "could not create socket");
    }

    struct sockaddr_un dom_addr;
    memset(&dom_addr, 0, sizeof(struct sockaddr_un));
    dom_addr.sun_family = AF_UNIX;
    strncpy(dom_addr.sun_path, "\0SuperAwesomeFirewall", sizeof("\0SuperAwesomeFirewall"));

    if (bind(dom_socket, (struct sockaddr *) &dom_addr, sizeof(struct sockaddr_un)) == -1) {
        printf("%s\n", "Another instance of the firewall is running!");
        exit(1);
    }

    if (getuid() != 0) {
        printf("%s\n", "You must have root privilege to run this program");
        printf("%s\n", "Try again with 'sudo'");
        exit(1);
    }

    if (argc != 1) {
        printf("%d\n", argc);
        printf("%s\n", "Invalid commandline options!");
        printf("%s\n", "Usage: sudo ./main");
        exit(1);
    }

    /* 
        lines 326-444 create the ethernet headers for the two ethernet interfaces
        these headers are not used in this current program but are available

        original use case was to strip the frame of the ethernet headers before
        processing the packet and then add it back in by calling send_ip_packet
        After writing the code below I decided to skip that step of stripping 
        the ethernet header out and now this code has no use in the current program

        so sad :(

        If you would like to restore this functionality you need to make changes to the
        two send_ip_packet functions
    */
    FILE* fp;
    fp = popen("arping -c 1 -I ext 10.0.2.2", "r");
    if (fp == NULL) {
        printf("%s\n", "failed to retrieve mac address for ext");
    }

    int i = 0;
    char arping[250]; //size of response is ~150 gave it some padding just to be safe
    while (i < 2) {
        fgets(arping, 250, fp);
        i++;
    }

    char* pch;
    pch = strtok(arping, " ");
    i = 0;
    while (i < 4) {
        pch = strtok(NULL, " [");
        i++;
    }

    int len = strlen(pch);
    pch[len-1] = 0;

    char mac_ext[6] = {0};
    char* hex;
    unsigned char ascii;
    hex = strtok(pch, ":");
    i = 0;
    while (hex != NULL) {
        ascii = strtol(hex, NULL, 16);
        // printf("%c", ascii);
        mac_ext[i++] = ascii;
        hex = strtok(NULL, ":");
    }

    // printf("\n");

    fp = popen("ip link show int", "r");
    if (fp == NULL) {
        printf("%s\n", "failed to retrieve mac address for int");
    }

    i = 0;
    char iplink[250];
    while (i < 2) {
        fgets(iplink, 250, fp);
        i++;
    }

    pch = strtok(iplink, " ");
    i = 0;
    while (i < 1) {
        pch = strtok(NULL, " ");
        i++;
    }

    char mac_int[6] = {0};
    hex = strtok(pch, ":");
    i = 0;
    while (hex != NULL) {
        ascii = strtol(hex, NULL, 16);
        // printf("%c", ascii);
        mac_int[i++] = ascii;
        hex = strtok(NULL, ":");
    }

    // printf("\n");

    RegularInterface* ri;
    TAPInterface* ti;

    ri = init_RI(IFNAME_EXT);
    ti = init_TI(IFNAME_INT);

    char ext_hdr[14];
    char int_hdr[14];

    // char* ext_mac = "\x52\x54\x00\x12\x35\x02" //arping -c 1 -I ext 10.0.2.2
    // char* int_mac = "\x18\xba\xdd\xc0\xff\xee" //ip link show int

    i = 0;
    while (i < 6) {
        ext_hdr[i] = mac_ext[i];
        int_hdr[i] = mac_int[i];
        i++;
    }

    while (i < 12) {
        ext_hdr[i] = mac_int[i-6];
        int_hdr[i] = mac_ext[i-6];
        i++;
    }

    unsigned char p_ip[2];
    p_ip[0] = 8;
    p_ip[1] = 0;

    ext_hdr[12] = p_ip[0];
    ext_hdr[13] = p_ip[1];

    int_hdr[12] = p_ip[0];
    int_hdr[13] = p_ip[1];

    ri->set_eth_hdr(ri, ext_hdr);
    ti->set_eth_hdr(ti, int_hdr);

    // printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", ri->eth_hdr[0], 
    //     ri->eth_hdr[1], ri->eth_hdr[2], ri->eth_hdr[3],
    //     ri->eth_hdr[4], ri->eth_hdr[5], ri->eth_hdr[6], 
    //     ri->eth_hdr[7], ri->eth_hdr[8], ri->eth_hdr[9], 
    //     ri->eth_hdr[10], ri->eth_hdr[11], ri->eth_hdr[12],
    //     ri->eth_hdr[13]);
    // printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", ti->eth_hdr[0], 
    //     ti->eth_hdr[1], ti->eth_hdr[2], ti->eth_hdr[3],
    //     ti->eth_hdr[4], ti->eth_hdr[5], ti->eth_hdr[6], 
    //     ti->eth_hdr[7], ti->eth_hdr[8], ti->eth_hdr[9], 
    //     ti->eth_hdr[10], ti->eth_hdr[11], ti->eth_hdr[12],
    //     ti->eth_hdr[13]);

    printf("Initialized!\n");
    printf("Begin intercepting\n");
    int nfds;
    if (ri->sockfd >= ti->sockfd) {
        nfds = ri->sockfd + 1;
    } else {
        nfds = ti->sockfd + 1;
    }

    struct timeval wait;
    wait.tv_sec = 0;
    wait.tv_usec = 1;

    while (1) {
        fd_set read_fd_set;
        FD_ZERO(&read_fd_set);
        FD_SET(ri->sockfd, &read_fd_set);
        FD_SET(ti->sockfd, &read_fd_set);

        if (select(nfds, &read_fd_set, NULL, NULL, &wait) >= 0) {
            if (FD_ISSET(ri->sockfd, &read_fd_set)) {
                char* frame = malloc(MAX_FRAME);
                int size_rcvd;

                size_rcvd = ri->recv_eth_frame(ri, frame);

                // uncomment this line if you want to do packet processing in c
                // long pass = py_process_packet("handle_pkt", "bypass", frame, size_rcvd);
                
                long pass = 1; // comment out to do packet processing in python
                printf("passing packet: %d bytes\n", size_rcvd);

                if (pass == 1) {
                    ti->send_eth_frame(ti, frame, size_rcvd);
                }

                free(frame);
            }

            if (FD_ISSET(ti->sockfd, &read_fd_set)) {
                char* frame = malloc(MAX_FRAME);
                int size_rcvd;

                size_rcvd = ti->recv_eth_frame(ti, frame);

                // uncomment this line if you want to do packet processing in c
                // long pass = py_process_packet("handle_pkt", "bypass", frame, size_rcvd);
                
                long pass = 1; // comment out to do packet processing in python
                printf("passing packet: %d bytes\n", size_rcvd);

                if (pass == 1) {
                    ri->send_eth_frame(ri, frame, size_rcvd);
                }
                
                free(frame);
            }

            FD_CLR(ri->sockfd, &read_fd_set);
            FD_CLR(ti->sockfd, &read_fd_set);
        }
    }

    return 0;
}
