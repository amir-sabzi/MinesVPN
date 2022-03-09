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
#include <math.h>

/*
 * NIC queue size (determined from ethtool -g)
 */
#define DEFAULT_NICQ_SIZE 4096
#define DEFAULT_IFACE "enp8s0f1"
#define DEFAULT_HOST  "192.168.1.18"
#define DEFAULT_UDP_PORT  8000

#define LOG_ARRAY_SIZE 10000
long long log_array [LOG_ARRAY_SIZE][6];
int log_counter = 0;

/*
 * =========
 * Debugging
 * =========
 */

/*
 * config level for debug prints
 */
#define CONFIG_DPRINT_LVL 0

/*
 * enable all prints
 */
#define LVL_DBG 0
/*
 * enable error prints
 */
#define LVL_ERR 1
/*
 * experiment config, disable most prints to minimize timing variability
 */
#define LVL_EXP   2

#define dprint(LVL, F, A...)  \
  do {  \
    if (CONFIG_DPRINT_LVL <= LVL) { \
      printf("[%s:%d] " F "\n", __func__, __LINE__, A); \
    } \
  } while(0)

/*
 * =====================
 * Timing and statistics
 * =====================
 */

#define CONFIG_STAT 1
#define CONFIG_STAT_DETAIL 1

#define S_TO_MS (1000UL)
#define MS_TO_US  (1000UL)
#define US_TO_NS  (1000UL)
#define S_TO_US ((S_TO_MS) * (MS_TO_US))
#define S_TO_NS ((S_TO_US) * (US_TO_NS))
#define MS_TO_NS  ((MS_TO_US) * (US_TO_NS))

enum {
  SCALE_SEC = 0,
  SCALE_MS,
  SCALE_US,
  SCALE_NS,
  MAX_TIMESCALE
};

char *scale_str[MAX_TIMESCALE] = {
  "s",
  "ms",
  "us",
  "ns"
};

static inline double get_time(int scale)
{
  struct timeval t;
  gettimeofday(&t, NULL);

  switch (scale) {
    case SCALE_SEC:
      return (double) t.tv_sec + (double) t.tv_usec / S_TO_US;
    case SCALE_MS:
      return (double) t.tv_sec * S_TO_MS + (double) t.tv_usec / MS_TO_US;
    case SCALE_US:
      return (double) t.tv_sec * S_TO_US + (double) t.tv_usec;
    case SCALE_NS:
    default:
      return (double) t.tv_sec * S_TO_NS + (double) t.tv_usec * US_TO_NS;
  }
}

typedef struct expstat {
  char name[64];
  int scale;
  long count;
  double start;
  double end;
  double step;
  double min;
  double max;
  double sum;
  double sqr_sum;
  long *dist;
} expstat_t;

int init_expstat(expstat_t *stat, char *name, double start, double end, double step, int scale)
{
  memcpy(stat->name, name, strlen(name));
  stat->start = start;
  stat->end = end;
  stat->step = step;
  stat->count = 0;
  stat->scale = scale;
  stat->min = 999999999;
  stat->max = 0;

#if CONFIG_STAT_DETAIL == 1
  int size = (end - start) / step + 1;
  stat->dist = malloc(size * sizeof(long));
  if (!stat->dist)
    return -ENOMEM;

  memset(stat->dist, 0, size * sizeof(long));
#endif

  return 0;
}

void add_point(expstat_t *stat, double value)
{
  if (value >= stat->max)
    stat->max = value;
  if (value <= stat->min)
    stat->min = value;

#if CONFIG_STAT_DETAIL == 1
  int index = (int) ((value - stat->start) / stat->step);
  if (value >= stat->end)
    index = (int) ((stat->end - stat->start) / stat->step);
  else if (value < stat->start)
    index = 0;

  stat->dist[index]++;
#endif
  stat->count++;
  stat->sum += value;
  stat->sqr_sum += (value * value);
}

void print_distribution(expstat_t *stat)
{
  if (stat->count == 0)
    return;

  double avg = 0.0, stddev = 0.0;

  avg = stat->sum / stat->count;
  stddev = sqrt((stat->sqr_sum - 2 * avg * stat->sum + stat->count * avg * avg)
                / stat->count);
  printf("\n---------------------\n");
  printf("%s (%lf, %lf, %lf)\n", stat->name, stat->start, stat->end, stat->step);

  printf("%s Total time (%s): %lf\n", stat->name, scale_str[stat->scale], stat->sum);
  printf("%s Total count: %ld\n", stat->name, stat->count);
  printf("%s Range [%lf - %lf]\n", stat->name, stat->min, stat->max);
  printf("%s Avg: %lf, Stddev: %lf\n", stat->name, avg, stddev);

#if CONFIG_STAT_DETAIL == 1
  int size = (int) (stat->end - stat->start) / stat->step;
  double bucket = 0;
  for (int i = 0; i < size; i++) {
    if (stat->dist[i] == 0)
      continue;

    bucket = (double) stat->start + i*stat->step;
    printf("%lf\t%ld\n", bucket, stat->dist[i]);
  }
#endif
}

void free_expstat(expstat_t *stat)
{
#if CONFIG_STAT_DETAIL == 1
  if (stat->dist)
    free(stat->dist);
#endif
}

#if CONFIG_STAT == 1

#define DECL_STAT_EXTERN(name)  extern expstat_t *stat_##name
#define DECL_STAT(name) expstat_t *stat_##name = NULL
#define INIT_STAT(name, desc, sta, end, st, scale) \
  stat_##name = malloc(sizeof(expstat_t));  \
  if (!stat_##name) { \
    return -ENOMEM; \
  } \
  if (init_expstat(stat_##name, desc, sta, end, st, scale))  \
    return -ENOMEM; \

#define PRINT_STAT(name)  \
  if (stat_##name != NULL) {  \
    print_distribution(stat_##name);  \
  }

#define DEST_STAT(name) \
  if (stat_##name != NULL) {  \
    free_expstat(stat_##name);  \
    free(stat_##name);  \
  }

#define ADD_TIME_POINT(name)  \
  add_point(stat_##name, SPENT_TIME(name))

#define INIT_TIMER(name)  double t_start_##name = 0.0, t_end_##name = 0.0
#define START_TIMER(name, scale) t_start_##name = get_time(scale)
#define END_TIMER(name, scale) t_end_##name = get_time(scale)
#define START_TIME(name)  (t_start_##name)
#define END_TIME(name)  (t_end_##name)
#define SPENT_TIME(name)  (END_TIME(name) - START_TIME(name))

#else /* CONFIG_STAT == 0 */

#define DECL_STAT_EXTERN(name)
#define DECL_STAT(name)
#define INIT_STAT(name, desc, sta, end, st, scale)
#define PRINT_STAT(name)
#define DEST_STAT(name)
#define ADD_TIME_POINT(name)
#define INIT_TIMER(name)
#define START_TIMER(name, scale)
#define END_TIMER(name, scale)
#define START_TIME(name)
#define END_TIME(name)
#define SPENT_TIME(name)

#endif /* CONFIG_STAT */

DECL_STAT(sendlatency);

/*
 * ===============================
 * Logging per-packet measurements
 * ===============================
 */

void print_log(long long **log_array, int log_counter, FILE *fp)
{
  for (int i = 0; i < LOG_ARRAY_SIZE; i++) {
    for (int j = 0; j < 3; j++) {
      fprintf(fp, "%lld%lld", log_array[i][2*j], log_array[i][2*j+1]);
      (j==2) ? fprintf(fp, "\n")
             : fprintf(fp, ",");
    }
  }
  dprint(LVL_EXP, "%d", log_counter);
}

/*
 * ===============
 * Experiment code
 * ===============
 */

char *default_payload = NULL;

typedef struct {
  int fd;
  int port;
  int err_no;
  int nicq_size;
  int64_t last_snd;
  int64_t last_rcv;
  int64_t prev_serialnum;
  struct sockaddr_in local;
  struct sockaddr_in remote;
  struct timeval time_kernel;
  struct timeval time_user;
} socket_info;

typedef struct {
  int64_t serialnum;

  int64_t user_time_serialnum;
  int64_t user_time;

  int64_t kernel_time_serialnum;
  int64_t kernel_time;

  size_t message_bytes;
} message_header;

/*
 * prepare a default payload content
 */
void prepare_payload(char *buf, size_t len)
{
  if (len < sizeof(message_header)) {
    return;
  }

  memset(buf, 0, len);
  message_header *header = (message_header *) buf;
  char *payload = (char *) (header + 1);
  size_t payload_len = len - sizeof(message_header);

  for (size_t i = 0; i < payload_len; i++) {
    payload[i] = (char) random();
  }
}

void prepare_header(socket_info *inf, char *buf, size_t len)
{
  if (len < sizeof(message_header)) {
    return;
  }

  message_header *header = (message_header *) buf;
  header->serialnum = ++inf->last_snd;
}

/* ==========
 * UDP client
 * ==========
 */

/*
 * init sender socket, setup TX hardware timestamping
 */
static int setup_udp_sender(socket_info *inf, int port, char *address, int nicq_size)
{
  inf->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (inf->fd < 0) {
    inf->err_no = errno;
    dprint(LVL_ERR, "socket failed: %s", strerror(inf->err_no));
    return inf->fd;
  }

  inf->port = port;
  inf->nicq_size = nicq_size;
  inf->err_no = 0;
  inf->last_snd = 0;
  inf->last_rcv = 0;
  inf->prev_serialnum = -1;

  /* ------------ Sender: IOCTL to active HW-timestamping -----------*/

  struct hwtstamp_config hw_config;
  memset(&hw_config, 0, sizeof(hw_config));

  hw_config.tx_type = HWTSTAMP_TX_ON;

  // Timestamping filter: (CAUTION: should be available as an option for the NIC)
  hw_config.rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;
  
  // Interface Name: (CAUTION: consider changing the rx_filter when you change Interface)
  char* interface_name = DEFAULT_IFACE;
  
  struct ifreq hwtstamp;
  memset(&hwtstamp, 0, sizeof(hwtstamp));
  strcpy(hwtstamp.ifr_name, interface_name);
  hwtstamp.ifr_data = (void *) &hw_config;
  
  // IOCTL call:
  int ioctl_result = ioctl(inf->fd,SIOCSHWTSTAMP,&hwtstamp);
  dprint(LVL_DBG, "ioctl output: %d",ioctl_result);
  if(ioctl_result < 0 ){
    inf->err_no = errno;
    dprint(LVL_ERR, "ioctl failed: %s", strerror(inf->err_no));
    return ioctl_result;
  }
  /*--------------------------------------------------------------------*/  


  /*--------------Sender: Binding the socket to an interface------------*/
  int int_bind_result = setsockopt(inf->fd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&hwtstamp, sizeof(hwtstamp));
  if(int_bind_result < 0){
    inf->err_no = errno;
    dprint(LVL_ERR, "bind to interface failed: %s", strerror(inf->err_no));
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
    dprint(LVL_ERR, "setsockopt failed: %s", strerror(inf->err_no));
    return r;
  }
  
  inf->remote = (struct sockaddr_in) { .sin_family = AF_INET,
                                       .sin_port = htons((uint16_t)port)
                                     };
  r = inet_aton(address, &inf->remote.sin_addr);
  if (r == 0) {
    dprint(LVL_ERR, "%s", "inet_aton failed");
    inf->err_no = 0;
    return -1;
  }

  inf->local = (struct sockaddr_in) { .sin_family = AF_INET,
                                      .sin_port = htons(0),
                                      .sin_addr.s_addr = htonl(INADDR_ANY)
                                    };
  return 0;
}

/*
 * The structure can return up to three timestamps. This is a legacy
 * feature. Only one field is non-zero at any time. Most timestamps
 * are passed in ts[0]. Hardware timestamps are passed in ts[2].
 */
static void handle_scm_timestamping(struct scm_timestamping *ts, FILE* fp)
{
  for (size_t i = 0; i < sizeof ts->ts / sizeof *ts->ts; i++) {
    log_array[log_counter][2*i] = (long long) ts->ts[i].tv_sec;
    log_array[log_counter][2*i + 1] = (long long) ts->ts[i].tv_nsec;
#if 0
    fprintf(fp, "%lld.%.9lds", (long long)ts->ts[i].tv_sec, ts->ts[i].tv_nsec);
    (i==2) ? fprintf(fp, "\n") : fprintf(fp, ",");
     printf("timestamp: %lld.%.9lds; %ld\n", (long long)ts->ts[i].tv_sec,
           ts->ts[i].tv_nsec, ts->ts[i].tv_nsec);
#endif
  }
  log_counter ++;
}

static void handle_time(struct msghdr *msg, FILE* fp)
{

  for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(msg); cmsg;
       cmsg = CMSG_NXTHDR(msg, cmsg)) {
    dprint(LVL_DBG, "level=%d, type=%d, len=%zu\n", cmsg->cmsg_level, cmsg->cmsg_type,
           cmsg->cmsg_len);

    if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVERR) {
      struct sock_extended_err *ext =
          (struct sock_extended_err *)CMSG_DATA(cmsg);
      dprint(LVL_DBG, "errno=%d, origin=%d\n", ext->ee_errno, ext->ee_origin);
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
  dprint(LVL_DBG, "%s", "End messages");
}

static ssize_t udp_send(socket_info *inf, char *buf, size_t len)
{
  struct iovec iov = (struct iovec) { .iov_base = buf, .iov_len = len };
  struct msghdr msg = (struct msghdr) { .msg_name = &inf->remote,
                                        .msg_namelen = sizeof inf->remote,
                                        .msg_iov = &iov,
                                        .msg_iovlen = 1
                                      };
  INIT_TIMER(sendlatency);

  gettimeofday(&inf->time_user, NULL);
  START_TIMER(sendlatency, SCALE_MS);

  ssize_t send_len = sendmsg(inf->fd, &msg, 0);

  END_TIMER(sendlatency, SCALE_MS);
  ADD_TIME_POINT(sendlatency);

  if (send_len < 0) {
    inf->err_no = errno;
    dprint(LVL_ERR, "sendmsg failed: %s", strerror(inf->err_no));
  }

  return send_len;
}

static ssize_t meq_receive(socket_info *inf, char *buf, size_t len, FILE* fp)
{
  char ctrl[2048];
  struct iovec iov = (struct iovec) { .iov_base = buf, .iov_len = len };
  struct msghdr msg = (struct msghdr) { .msg_control = ctrl,
                                        .msg_controllen = sizeof ctrl,
                                        .msg_name = &inf->remote,
                                        .msg_namelen = sizeof inf->remote,
                                        .msg_iov = &iov,
                                        .msg_iovlen = 1
                                      };

  ssize_t recv_len = recvmsg(inf->fd, &msg, MSG_ERRQUEUE);
  
  //printf("recv_len: %d\n", (int)recv_len);
  if (recv_len < 0) {
    inf->err_no = errno;
    if (errno != EAGAIN) {
      dprint(LVL_ERR, "recvmsg failed: %s", strerror(inf->err_no));
    }
    return recv_len;
  }

  message_header *header = (message_header *) iov.iov_base;
  int64_t seq = header->serialnum;
  if (seq != inf->last_rcv+1) {
    dprint(LVL_ERR, "packet error last_snd %ld last_rcv %ld seq %ld",
        inf->last_snd, inf->last_rcv, seq);
  }

  inf->last_rcv = seq;
  //printf("last_rcv: %d\n", (int)inf->last_rcv);
  handle_time(&msg, fp);
  return recv_len;
}

static void sender_loop(socket_info *inf, useconds_t soft_interval,
    int packet_num, size_t packet_size, FILE *fp, char *packet_buffer)
{
  int i = 0;
  while (1) {
    usleep(soft_interval);

    /*
     * NIC queue len worth of packets are either queued in the NIC or in flight
     * wait till some packets drained from the NIC or acknowledged
     */
    if (inf->last_snd - inf->last_rcv >= DEFAULT_NICQ_SIZE){
      printf("last send seq: %d\n", (int)inf->last_snd);
      printf("last recv seq: %d\n", (int)inf->last_rcv);

      continue;
    } 
    prepare_header(inf, packet_buffer, packet_size);
    udp_send(inf, packet_buffer, packet_size);

    while (meq_receive(inf, packet_buffer, sizeof packet_buffer, fp) != -1) {
      // no-op
    }

    i++;
    if (i >= packet_num)
      break;
  }
}

/* ==========
 * UDP server
 * ==========
 */

/*
 * init a receiver socket, setup RX hardware timestamp
 */
static int setup_udp_receiver(socket_info *inf, int port)
{
  inf->port = port;
  inf->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
  hw_config.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_SYNC;
  
  // Interface Name: (CAUTION: consider changing the rx_filter when you change Interface)
  char* interface_name = "enp8s0f1";
  
  struct ifreq hwtstamp;
  memset(&hwtstamp, 0, sizeof(hwtstamp));
  strcpy(hwtstamp.ifr_name, interface_name);
  hwtstamp.ifr_data = (void *) &hw_config;
  
  // IOCTL call:
  int ioctl_result = ioctl(inf->fd,SIOCSHWTSTAMP,&hwtstamp);
  dprint(LVL_DBG, "ioctl output: %d",ioctl_result);
  if(ioctl_result < 0 ) {
    inf->err_no = errno;
    dprint(LVL_ERR, "ioctl failed: %s",strerror(inf->err_no));
    return ioctl_result;
  }
  /*--------------------------------------------------------------------*/

  /*--------------Receiver: Binding the socket to an interface------------*/
  // we already defined the ifreq struct with the interface name.
  // here we use the same hwtstamp struct to bind to defined interface. 
  int int_bind_result = setsockopt(inf->fd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&hwtstamp, sizeof(hwtstamp));
  if(int_bind_result < 0) {
    inf->err_no = errno;
    dprint(LVL_ERR, "bind to interface failed: %s", strerror(inf->err_no));
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
    dprint(LVL_ERR, "setsockopt SO_TIMESTAMPING ret: %s", strerror(inf->err_no));
    return r;
  }
  
  int on = 1;
  r = setsockopt(inf->fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof on);
  if (r < 0) {
    inf->err_no = errno;
    dprint(LVL_ERR, "setsockopt SO_REUSEPORT ret: %s", strerror(inf->err_no));
    return r;
  }
  
  inf->local = (struct sockaddr_in) { .sin_family = AF_INET,
                                      .sin_port = htons((uint16_t)port),
                                      .sin_addr.s_addr = htonl(INADDR_ANY)
                                    };
  r = bind(inf->fd, (struct sockaddr *)&inf->local, sizeof inf->local);
  if (r < 0) {
    inf->err_no = errno;
    dprint(LVL_ERR, "bind failed: %s", strerror(inf->err_no));
    return r;
  }

  inf->prev_serialnum = -1;

  return 0;
}

static ssize_t udp_receive(socket_info *inf, char *buf, size_t len, FILE* fp)
{
  char ctrl[2048];
  struct iovec iov = (struct iovec){.iov_base = buf, .iov_len = len};
  struct msghdr msg = (struct msghdr) { .msg_control = ctrl,
                                        .msg_controllen = sizeof ctrl,
                                        .msg_name = &inf->remote,
                                        .msg_namelen = sizeof inf->remote,
                                        .msg_iov = &iov,
                                        .msg_iovlen = 1
                                      };

  ssize_t recv_len = recvmsg(inf->fd, &msg, 0);
  if (recv_len < 0) {
    inf->err_no = errno;
    dprint(LVL_ERR, "recvfrom failed: %s", strerror(inf->err_no));
    return recv_len;
  }

  message_header *header = (message_header *) iov.iov_base;
  int64_t seq = header->serialnum;
  if (seq != inf->last_rcv+1) {
    dprint(LVL_ERR, "packet error last_snd %ld last_rcv %ld seq %ld",
        inf->last_snd, inf->last_rcv, seq);
  }

  inf->last_rcv = seq;

  handle_time(&msg, fp);

  return recv_len;
}

static void receiver_loop(socket_info *inf, int packet_num, FILE *fp)
{
  char packet_buffer[4096];

  for (int i = 0; i < packet_num; i++) {
    udp_receive(inf, packet_buffer, sizeof packet_buffer, fp);
  }
}

#define USAGE "Usage: %s delay log [-r | -s]\n"

int main(int argc, char *argv[])
{
  if (argc < 4) {
    fprintf(stderr, USAGE, argv[0]);
    return EXIT_FAILURE;
  }

  if ((strcmp(argv[3], "-s") != 0) && (strcmp(argv[3], "-r") != 0)) {
    fprintf(stderr, USAGE, argv[0]);
    return EXIT_FAILURE;
  }

  // The value of interval time in microsecond
  useconds_t interval_t = atoi(argv[1]); 
  char *log_path = argv[2];

  // Number of packets to be sent
  int packet_num = 10000;
  size_t packet_size = 256;

  // Creating files for logging
  FILE *fp;
  fp = fopen(log_path, "w");
  if (fp == NULL) {
    dprint(LVL_ERR, "%s", "couldn't create/open file");
    return EXIT_FAILURE;
  }

  INIT_STAT(sendlatency, "sendmsg latency", 0.0, 100.0, 0.01, SCALE_MS);

  // prepare fixed random payload
  default_payload = malloc(packet_size);
  prepare_payload(default_payload, packet_size);

  if (strcmp(argv[3], "-s") == 0) {
    socket_info snd_inf;
    int ret = setup_udp_sender(&snd_inf, DEFAULT_UDP_PORT, DEFAULT_HOST, DEFAULT_NICQ_SIZE);
    if (ret < 0) {
      return EXIT_FAILURE;
    }
    sender_loop(&snd_inf, interval_t, packet_num, packet_size, fp, default_payload);
  } else {
    socket_info rcv_inf;
    int ret = setup_udp_receiver(&rcv_inf, DEFAULT_UDP_PORT);
    if (ret < 0) {
      return EXIT_FAILURE;
    }
    receiver_loop(&rcv_inf, packet_num, fp);
  } 

  print_log((long long **) log_array, log_counter, fp);
  fclose(fp);

  PRINT_STAT(sendlatency);
  DEST_STAT(sendlatency);

  return EXIT_SUCCESS;
}
