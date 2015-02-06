

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

int main(int argc, char **argv) {
   int fd;
   int ret;
   long flags;
   struct sockaddr_l2 addr;

   addr.l2_family = AF_BLUETOOTH;
   str2ba("00:01:02:0A:0B:0C", &addr.l2_bdaddr);
   addr.l2_psm = htobs(1);

   fd = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
   flags = fcntl(fd, F_GETFL);
   fcntl(fd, F_SETFL, flags | O_NONBLOCK);

   connect(fd, (struct sockaddr *)&addr, sizeof(addr));

   sleep(1);
   shutdown(fd, SHUT_RDWR);
   sleep(1);
   close(fd);
   return 0;
}
