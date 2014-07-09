#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

char *read_message(int server_socket)
{
  /* read the first thing */
  int pascal_length;
  if(recv(server_socket, &pascal_length, 4, MSG_WAITALL) < 4)
  {
    fprintf(stderr, "receive interrupted by some unknown phenomenon\n");
    return NULL;
  }
  pascal_length = ntohl(pascal_length); /* ENDIAN FIX */
  /* now read the unfirst thing */
  char *message = malloc(pascal_length + 1); /* don't a-forget the a-null */
  if(recv(server_socket, message, pascal_length, MSG_WAITALL) < pascal_length)
  {
    fprintf(stderr, "OTHER receive didn't work\n");
    free(message);
    return NULL;
  }
  message[pascal_length] = '\0'; /* NULL */
  return message;
}

int connect_to_server(char *hostname, uint16_t port)
{
  int server_socket;
  struct sockaddr_in server_addr;

  if((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    fprintf(stderr, "ARGH - socket doesn't exist or something\n");
    exit(1);
  }

  struct hostent *host = gethostbyname(hostname);
  if(host == NULL)
  {
    fprintf(stderr, "Argh, hostname doesn't thing\n");
    exit(1);
  }

  memset(&server_addr, 0, sizeof(server_addr)); /* clear struct */
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = *((uint32_t *) host->h_addr_list[0]);
  server_addr.sin_port = htons(port);
  if(connect(server_socket, (struct sockaddr *) &server_addr,
        sizeof(server_addr)) < 0)
  {
    fprintf(stderr, "ARGH - CONNECT DIDN'T.\n");
    exit(1);
  }
  return server_socket;
}

int game_init(int user_socket, int server_socket)
{
  printf("I'mma innitting the game now that everything's a-connected");
  char *message;
  if((message = read_message(server_socket)) == NULL)
  {
    fprintf(stderr, "READ MESSAGE FAILED\n");
    return 1;
  }
  if(strcmp(message, "NO") == 0)
  {
    fprintf(stderr, "I'm sorry, Cap'n, there's no more room\n");
    free(message);
    return 1;
  }
  else if(strcmp(message, "YO") == 0)
  {
    printf("We're good to go, there's room!\n");
    free(message);
  }
  else
  {
    fprintf(stderr, "Errr, got something that's not YO or NO (%s)\n", message);
    free(message);
    return 1;
  }
  /* we have yo by this stage */
  return 0;
}

void game_loop(int user_socket, int server_socket)
{
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

  if(game_init(STDIN_FILENO, server_socket))
  {
    fprintf(stderr, "Game didn't init\n");
    return 1;
  }

  game_loop(STDIN_FILENO, server_socket);
  return 0;
}
