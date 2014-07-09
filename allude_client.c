#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

char **read_message(int server_socket)
{
  /* read the first thing */
  
  /* now read the unfirst thing */
}

int connect_to_server(char *hostname, uint16_t port)
{
  int server_socket;
  struct sockaddr_in server_addr;

  if((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    fprintf(stderr, "ARGH - socket doesn't exist or something\n");
    return 1;
  }

  struct hostent *host = gethostbyname(hostname);
  if(host == NULL)
  {
    fprintf(stderr, "Argh, hostname doesn't thing\n");
    return 1;
  }

  memset(&server_addr, 0, sizeof(server_addr)); /* clear struct */
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = *((uint32_t *) host->h_addr_list[0]);
  server_addr.sin_port = htons(port);
  if(connect(server_socket, (struct sockaddr *) &server_addr,
        sizeof(server_addr)) < 0)
  {
    fprintf(stderr, "ARGH - CONNECT DIDN'T.\n");
    return 1;
  }
  return server_socket;
}

int main(int argc, char **argv)
{
  printf("Welcome to the allude [sic] client.\n");
  if(argc != 3)
  {
    fprintf(stderr, "ARGH - wrong number of args. Gimme a server and port...\n");
    return 1;
  }

  int server_socket = connect_to_server(argv[1], atoi(argv[2]));
  return 0;
}
