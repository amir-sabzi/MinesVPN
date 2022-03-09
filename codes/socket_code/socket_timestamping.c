#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <linux/errqueue.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>


#define NIC_QUEUE_SIZE 4096
#define UDP_MAX_LENGTH 1500
#define LOG_ARRAY_SIZE 5000000
long long log_hw_sec [LOG_ARRAY_SIZE][3];
long log_hw_nsec [LOG_ARRAY_SIZE][3];

int64_t log_sw_usec [LOG_ARRAY_SIZE][3];
int log_hw_counter = 0;
int log_sw_counter = 0;
int pause_count = 0;

typedef struct {
  int fd;
  int port;
  int err_no;
  struct sockaddr_in local;
  long last_send;
  long  last_ack;
  struct sockaddr_in remote;
  struct timeval time_before_send;
  struct timeval time_after_ack;
  struct timeval time_after_send;
  int64_t prev_serialnum;
} socket_info;

// they made the function static to limits its scope to this object file (not a big deal anyway)
// this function get a port number, create a socket...
static int setup_udp_receiver(socket_info *inf, int port) {
  inf->port = port;
  inf->fd = socket(AF_INET, SOCK_DGRAM, 17);
  if (inf->fd < 0) {
    inf->err_no = errno;
    fprintf(stderr, "setup_udp_server: socket failed: %s\n",
            strerror(inf->err_no));
    return inf->fd;
  }

 /* ------------ Receiver: IOCTL to active HW-timestamping -----------*/

  struct hwtstamp_config hw_config;
  memset(&hw_config, 0, sizeof(hw_config));

  hw_config.tx_type = HWTSTAMP_TX_ON;

  // Timestamping filter: (CAUTION: should be available as an option for the NIC)
   hw_config.rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;
  
  // Interface Name: (CAUTION: consider changing the rx_filter when you change Interface)
  char* interface_name = "enp66s0f1";
  
  struct ifreq hwtstamp;
  memset(&hwtstamp, 0, sizeof(hwtstamp));
  strcpy(hwtstamp.ifr_name, interface_name);
  hwtstamp.ifr_data = (void *) &hw_config;
  
   // IOCTL call:
   int ioctl_result = ioctl(inf->fd,SIOCSHWTSTAMP,&hwtstamp);
   printf("IOCTL output: %d\n",ioctl_result);
   if(ioctl_result < 0 ){
     inf->err_no = errno;
     fprintf(stderr, "ioctil failed: %s\n",strerror(inf->err_no));
     return ioctl_result;
   }
  /*--------------------------------------------------------------------*/

   /*--------------Receiver: Binding the socket to an interface------------*/
   // we already defined the ifreq struct with the interface name.
   // here we use the same hwtstamp struct to bind to defined interface. 
   int int_bind_result = setsockopt(inf->fd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&hwtstamp, sizeof(hwtstamp));
   if(int_bind_result < 0){
     inf->err_no = errno;
     fprintf(stderr, "setup_udp_server: bind to interface failed: %s\n",strerror(inf->err_no));
     return int_bind_result; 
   }
   /*-----------------------------------------------------------------------*/

  int timestampOn = // SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
       SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_TX_SOFTWARE |
       SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_RX_HARDWARE |
       SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE |
       SOF_TIMESTAMPING_OPT_TSONLY |
       0;
  
  int r = setsockopt(inf->fd, SOL_SOCKET, SO_TIMESTAMPING, &timestampOn,
                     sizeof timestampOn);
  if (r < 0) {
    inf->err_no = errno;
    fprintf(stderr, "setup_udp_server: setsockopt failed: %s\n",
            strerror(inf->err_no));
    return r;
  }
  
  int on = 1;
  r = setsockopt(inf->fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof on);
  if (r < 0) {
    inf->err_no = errno;
    fprintf(stderr, "setup_udp_server: setsockopt2 failed: %s\n",
            strerror(inf->err_no));
    return r;
  }
  
  inf->local = (struct sockaddr_in){.sin_family = AF_INET,
                                    .sin_port = htons((uint16_t)port),
                                    .sin_addr.s_addr = htonl(INADDR_ANY)};
  r = bind(inf->fd, (struct sockaddr *)&inf->local, sizeof inf->local);
  if (r < 0) {
    inf->err_no = errno;
    fprintf(stderr, "setup_udp_server: bind failed: %s\n",
            strerror(inf->err_no));
    return r;
  }

  inf->prev_serialnum = -1;

  return 0;
}

static int setup_udp_sender(socket_info *inf, int port, char *address) {
  inf->port = port;
  inf->fd = socket(AF_INET, SOCK_DGRAM, 17);
  if (inf->fd < 0) {
    inf->err_no = errno;
    fprintf(stderr, "setup_udp_client: socket failed: %s\n",
            strerror(inf->err_no));
    return inf->fd;
  }

 /* ------------ Sender: IOCTL to active HW-timestamping -----------*/

  struct hwtstamp_config hw_config;
  memset(&hw_config, 0, sizeof(hw_config));

  hw_config.tx_type = HWTSTAMP_TX_ON;

  // Timestamping filter: (CAUTION: should be available as an option for the NIC)
   hw_config.rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;
  
  // Interface Name: (CAUTION: consider changing the rx_filter when you change Interface)
  char* interface_name = "enp66s0f1";
  
  struct ifreq hwtstamp;
  memset(&hwtstamp, 0, sizeof(hwtstamp));
  strcpy(hwtstamp.ifr_name, interface_name);
  hwtstamp.ifr_data = (void *) &hw_config;
  
   // IOCTL call:
   int ioctl_result = ioctl(inf->fd,SIOCSHWTSTAMP,&hwtstamp);
   printf("IOCTL output: %d\n",ioctl_result);
   if(ioctl_result < 0 ){
     inf->err_no = errno;
     fprintf(stderr, "ioctil failed: %s\n",strerror(inf->err_no));
     return ioctl_result;
   }
  /*--------------------------------------------------------------------*/  


   /*--------------Sender: Binding the socket to an interface------------*/
   int int_bind_result = setsockopt(inf->fd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&hwtstamp, sizeof(hwtstamp));
   if(int_bind_result < 0){
     inf->err_no = errno;
     fprintf(stderr, "setup_udp_sender: bind to interface failed: %s\n",strerror(inf->err_no));
     return int_bind_result; 
   }
   /*-----------------------------------------------------------------------*/

  



   int timestampOn =  SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
    /* SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_TX_SOFTWARE |
     SOF_TIMESTAMPING_SOFTWARE |
     SOF_TIMESTAMPING_RX_HARDWARE |
     SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE |
     SOF_TIMESTAMPING_OPT_TSONLY |
     0;*/
  int r = setsockopt(inf->fd, SOL_SOCKET, SO_TIMESTAMPING, &timestampOn,
                     sizeof timestampOn);
  if (r < 0) {
    inf->err_no = errno;
    fprintf(stderr, "setup_udp_server: setsockopt failed: %s\n",
            strerror(inf->err_no));
    return r;
  }
  
  inf->remote = (struct sockaddr_in){.sin_family = AF_INET,
                                     .sin_port = htons((uint16_t)port)};
  r = inet_aton(address, &inf->remote.sin_addr);
  if (r == 0) {
    fprintf(stderr, "setup_udp_client: inet_aton failed\n");
    inf->err_no = 0;
    return -1;
  }

  inf->local = (struct sockaddr_in){.sin_family = AF_INET,
                                    .sin_port = htons(0),
                                    .sin_addr.s_addr = htonl(INADDR_ANY)};
  inf->prev_serialnum = -1;

  return 0;
}
// The structure can return up to three timestamps. This is a legacy
// feature. Only one field is non-zero at any time. Most timestamps
// are passed in ts[0]. Hardware timestamps are passed in ts[2].
static void handle_scm_timestamping(struct scm_timestamping *ts, FILE* fp) {
  for (size_t i = 0; i < sizeof ts->ts / sizeof *ts->ts; i++) {
    log_hw_sec[log_hw_counter][i] = (long long)ts -> ts[i].tv_sec;
    log_hw_nsec[log_hw_counter][i] = ts -> ts[i].tv_nsec;
    //fprintf(fp, "%lld.%.9lds", (long long)ts->ts[i].tv_sec, ts->ts[i].tv_nsec);
    //(i==2) ? fprintf(fp, "\n") : fprintf(fp, ",");
    // printf("timestamp: %lld.%.9lds; %ld\n", (long long)ts->ts[i].tv_sec, ts->ts[i].tv_nsec, ts->ts[i].tv_nsec);
     
    
  }
  log_hw_counter ++;
}

static void handle_time(struct msghdr *msg, FILE* fp) {

  for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(msg); cmsg;
       cmsg = CMSG_NXTHDR(msg, cmsg)) {
    //printf("level=%d, type=%d, len=%zu\n", cmsg->cmsg_level, cmsg->cmsg_type, cmsg->cmsg_len);

    if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVERR) {
      struct sock_extended_err *ext =
          (struct sock_extended_err *)CMSG_DATA(cmsg);
     // printf("errno=%d, origin=%d\n", ext->ee_errno, ext->ee_origin);
      continue;
    }

    if (cmsg->cmsg_level != SOL_SOCKET)
      continue;

    switch (cmsg->cmsg_type) {
    case SO_TIMESTAMPNS: {
      struct scm_timestamping *ts = (struct scm_timestamping *)CMSG_DATA(cmsg);
      handle_scm_timestamping(ts, fp);
    } break;
    case SO_TIMESTAMPING: {
      struct scm_timestamping *ts = (struct scm_timestamping *)CMSG_DATA(cmsg);
      handle_scm_timestamping(ts, fp);
    } break;
    default:
      /* Ignore other cmsg options */
      break;
    }
  }
  // printf("End messages\n");
}

static ssize_t udp_receive(socket_info *inf, char *buf, size_t len, FILE* fp) {
  char ctrl[2048];
  struct iovec iov = (struct iovec){.iov_base = buf, .iov_len = len};
  struct msghdr msg = (struct msghdr){.msg_control = ctrl,
                                      .msg_controllen = sizeof ctrl,
                                      .msg_name = &inf->remote,
                                      .msg_namelen = sizeof inf->remote,
                                      .msg_iov = &iov,
                                      .msg_iovlen = 1};
  ssize_t recv_len = recvmsg(inf->fd, &msg, 0);


  if (recv_len < 0) {
    inf->err_no = errno;
    fprintf(stderr, "udp_receive: recvfrom failed: %s\n",
            strerror(inf->err_no));
  }

  handle_time(&msg, fp);

  return recv_len;
}

static ssize_t udp_send(socket_info *inf, char *buf, size_t len) {
  struct iovec iov = (struct iovec){.iov_base = buf, .iov_len = len};
  struct msghdr msg = (struct msghdr){.msg_name = &inf->remote,
                                      .msg_namelen = sizeof inf->remote,
                                      .msg_iov = &iov,
                                      .msg_iovlen = 1};
  gettimeofday(&inf->time_before_send, NULL);
  ssize_t send_len = sendmsg(inf->fd, &msg, 0);
  if (send_len < 0) {
    inf->err_no = errno;
    fprintf(stderr, "udp_send: sendmsg failed: %s\n", strerror(inf->err_no));
  }else{
    gettimeofday(&inf->time_after_send, NULL);
  }
  inf->last_send ++;
  return send_len;
}

static ssize_t meq_receive(socket_info *inf, char *buf, size_t len, FILE* fp) {
  struct iovec iov = (struct iovec){.iov_base = buf, .iov_len = len};
  char ctrl[2048];
  struct msghdr msg = (struct msghdr){.msg_control = ctrl,
                                      .msg_controllen = sizeof ctrl,
                                      .msg_name = &inf->remote,
                                      .msg_namelen = sizeof inf->remote,
                                      .msg_iov = &iov,
                                      .msg_iovlen = 1};
  ssize_t recv_len = recvmsg(inf->fd, &msg, MSG_ERRQUEUE);
  //printf("recv_len: %d\n", (int)recv_len);
  if (recv_len < 0) {
    inf->err_no = errno;
    if (errno != EAGAIN) {
      fprintf(stderr, "meq_receive: recvmsg failed: %s\n",
              strerror(inf->err_no));
    }
    return recv_len;
  }
  handle_time(&msg, fp);
  inf->last_ack ++;
  return recv_len;
}


static void sender_loop(char *host, useconds_t soft_interval, int packet_num, int packet_size,  FILE* fp, char* packet_buffer) {
  socket_info inf;
  inf.last_ack = 0;
  inf.last_send = 0;
  // call to the setup sender with a pointer to socket_info struct to stablish the socket for us.
  int ret = setup_udp_sender(&inf, 8000, host);
  if (ret < 0) {
    return;
  }
  bool b;
  for (int i = 0; i < packet_num; i++) {
    usleep(soft_interval);
    if(inf.last_send - inf.last_ack >= NIC_QUEUE_SIZE){
      pause_count++;
      continue;
    }
    udp_send(&inf, packet_buffer, (size_t)packet_size);
    while (meq_receive(&inf, packet_buffer, sizeof packet_buffer, fp) != -1) {
    }
  gettimeofday(&inf.time_after_ack, NULL);

  log_sw_usec[log_sw_counter][0] = inf.time_before_send.tv_sec * 1000000000L + inf.time_before_send.tv_usec;
  log_sw_usec[log_sw_counter][1] = inf.time_after_send.tv_sec * 1000000000L + inf.time_after_send.tv_usec;
  log_sw_usec[log_sw_counter][2] = inf.time_after_ack.tv_sec * 1000000000L + inf.time_after_ack.tv_usec;
  log_sw_counter++;
  // printf("last_send: %ld\n", inf.last_send);
  // printf("last_ack: %ld\n", inf.last_ack);
  }
}

static void receiver_loop(FILE* fp) {
  socket_info inf;
  int ret = setup_udp_receiver(&inf, 8000);
  if (ret < 0) {
    return;
  }

  for (int i = 0; i < 1000; i++) {
    char packet_buffer[4096];
    udp_receive(&inf, packet_buffer, sizeof packet_buffer, fp);
  }
}

#define USAGE "Usage: %s delay log [-r | -s]\n"

int main(int argc, char *argv[]) {
  char* log_path = argv[2];
  char* receiver_addr = "192.168.1.18";
  // Packet size
  int packet_size = 512;
  
  // Here we create a constant payload for all packets
  char* payload;
  payload = (char*) malloc(packet_size * sizeof(char));
  memset(payload, 'A', packet_size);
  // Number of packets to be sent
  int packet_num = 5000000;

  // Creating files for logging
  FILE* fp;
  fp = fopen(log_path, "w");
  if (fp == NULL){
        printf("Couldn't create/open file\n");
        return 1;
  }
  if (argc == 4) {
    // The value of interval time in microsecond
    useconds_t interval_t = atoi(argv[1]); 
    if(strcmp(argv[3], "-s")==0){
      sender_loop(receiver_addr, interval_t, packet_num, packet_size, fp, payload);
    }else if (strcmp(argv[3], "-r")==0){
      receiver_loop(fp);
    }else{
      fprintf(stderr, USAGE, argv[0]);
    } 
  }else{
   fprintf(stderr, USAGE, argv[0]);
  }
  for(int i=0;i<LOG_ARRAY_SIZE;i++){
    for (int j=0; j<3; j++){
      fprintf(fp, "%lld%.9ld,",log_hw_sec[i][j], log_hw_nsec[i][j]);
    }
    for (int j=0; j<3; j++){
      (j==2) ? fprintf(fp,"%" PRId64 "\n", log_sw_usec[i][j]) : fprintf(fp, "%" PRId64 ",", log_sw_usec[i][j]);
    }
  }
  fclose(fp);
  printf("Number of hw-ts packets: %d\n", log_hw_counter);
  printf("Number of sw-ts packets: %d\n", log_sw_counter);
  printf("Number of pauses: %d\n", pause_count);
  return 0;
}
