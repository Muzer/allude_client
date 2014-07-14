#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define CMD_ERROR -1
#define CMD_YO 0
#define CMD_NO 1
#define CMD_MT 2
#define CMD_MC 3
#define NUM_CMDS 4

const char *cmds[] = {"YO", "NO", "MT", "MC"};

struct team
{
  uint32_t id;
  uint8_t type;
  uint32_t name_length;
  uint8_t *name;
};

struct character
{
  uint32_t id;
  uint32_t teamid;
  uint8_t type;
  uint32_t name_length;
  uint8_t *name;
};

/* this function calls malloc(3). You must call free(3) on its output. */
/* DOES NOT RETURN NULL-TERMINATED STRING - use memcmp(3) because it's evil.*/
uint8_t *read_message(int server_socket, uint32_t *length)
{
  /* read the first thing */
  uint32_t pascal_length;
  if(recv(server_socket, &pascal_length, 4, MSG_WAITALL) < 4)
  {
    fprintf(stderr, "receive interrupted by some unknown phenomenon\n");
    return NULL;
  }
  pascal_length = ntohl(pascal_length); /* ENDIAN FIX */
  /* now read the unfirst thing */
  uint8_t *message = malloc(pascal_length);
  if(recv(server_socket, message, pascal_length, MSG_WAITALL) < pascal_length)
  {
    fprintf(stderr, "OTHER receive didn't work\n");
    free(message);
    message = NULL;
    return NULL;
  }
  *length = pascal_length;
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

int get_cmd(uint8_t *message, int length)
{
  if(length<2)
    return -1;
  int i;
  for(i=0; i<NUM_CMDS; i++)
  {
    if(memcmp(message, cmds[i], 2) == 0)
      return i;
  }
  return -1;
}

struct team *get_teams(uint8_t *message, uint32_t length, uint32_t *num_teams)
{
  if(get_cmd(message, length) != CMD_MT)
  {
    fprintf(stderr, "Unexpected command in bagging area.\n");
    free(message);
    message = NULL;
    return NULL;
  }
  message += 2;
  // get length
  *num_teams = ntohl(*((uint32_t *)message));
  struct team *teams = malloc(sizeof(struct team) * *num_teams);
  uint32_t i;
  message += 4;
  for(i=0; i<*num_teams; i++)
  {
    teams[i].id = ntohl(*((uint32_t *)message));
    message += 4;
    teams[i].type = *message;
    message += 1;
    teams[i].name_length = ntohl(*((uint32_t *)message));
    message += 4;
    teams[i].name = malloc(teams[i].name_length);
    memcpy(teams[i].name, message, teams[i].name_length);
    message += teams[i].name_length;
  }
  return teams;
}

void free_teams(struct team *teams, uint32_t num_teams)
{
  uint32_t i;
  for(i=0; i<num_teams; i++)
  {
    free(teams[i].name);
    teams[i].name = NULL;
  }
  free(teams);
}

void print_teams(struct team *teams, uint32_t num_teams)
{
  uint32_t i;
  printf("--- TEAMS ---\n");
  for(i=0; i<num_teams; i++)
  {
    printf("%i) %.*s - %s\n", teams[i].id, teams[i].name_length, teams[i].name,
        teams[i].type == 0 ? "LEA" : "CREW");
  }
}

struct character *get_chars(uint8_t *message, uint32_t length, uint32_t
    *num_chars)
{
  if(get_cmd(message, length) != CMD_MC)
  {
    fprintf(stderr, "Unexpected command in bagging area.\n");
    free(message);
    message = NULL;
    return NULL;
  }
  message += 2;
  // get length
  *num_chars = ntohl(*((uint32_t *)message));
  struct character *characters = malloc(sizeof(struct character) * *num_chars);
  uint32_t i;
  message += 4;
  for(i=0; i<*num_chars; i++)
  {
    characters[i].id = ntohl(*((uint32_t *)message));
    message += 4;
    characters[i].teamid = ntohl(*((uint32_t *)message));
    message += 4;
    characters[i].type = *message;
    message += 1;
    characters[i].name_length = ntohl(*((uint32_t *)message));
    message += 4;
    characters[i].name = malloc(characters[i].name_length);
    memcpy(characters[i].name, message, characters[i].name_length);
    message += characters[i].name_length;
  }
  return characters;
}

void free_chars(struct character *characters, uint32_t num_chars)
{
  uint32_t i;
  for(i=0; i<num_chars; i++)
  {
    free(characters[i].name);
    characters[i].name = NULL;
  }
  free(characters);
}

void print_chars(struct character *characters, uint32_t num_chars)
{
  uint32_t i;
  printf("--- CHARACTERS ---\n");
  for(i=0; i<num_chars; i++)
  {
    printf("%i) %.*s - %s team %i\n", characters[i].id,
        characters[i].name_length, characters[i].name, characters[i].type == 0
        ? "LEA" : "CREW", characters[i].teamid);
  }
}

int game_init(int user_socket, int server_socket)
{
  printf("I'mma innitting the game now that everything's a-connected\n");
  uint8_t *message = NULL;
  uint32_t length = 0;
  if((message = read_message(server_socket, &length)) == NULL)
  {
    fprintf(stderr, "READ MESSAGE FAILED\n");
    return 1;
  }
  switch(get_cmd(message, length))
  {
    case CMD_NO:
      fprintf(stderr, "I'm sorry, Cap'n, there's no more room\n");
      free(message);
      message = NULL;
      return 1;
      break;
    case CMD_YO:
      printf("We're good to go, there's room!\n");
      free(message);
      message = NULL;
      break;
    default:
      fprintf(stderr, "Errr, got something that's not YO or NO (%s)\n",
          message);
      free(message);
      message = NULL;
      return 1;
      break;
  }
  /* we have yo by this stage */
  if((message = read_message(server_socket, &length)) == NULL)
  {
    fprintf(stderr, "READ MESSAGE FAILED\n");
    return 1;
  }
  uint32_t num_teams;
  struct team *teams = get_teams(message, length, &num_teams);
  if(teams == NULL)
    return 1;
  print_teams(teams, num_teams);
  free(message);

  if((message = read_message(server_socket, &length)) == NULL)
  {
    fprintf(stderr, "READ MESSAGE FAILED\n");
    return 1;
  }
  uint32_t num_chars;
  struct character *characters = get_chars(message, length, &num_chars);
  if(characters == NULL)
    return 1;
  print_chars(characters, num_chars);
  free(message);

  
  free_teams(teams, num_teams);
  free_chars(characters, num_chars);
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
    fprintf(stderr,
        "ARGH - wrong number of args. Gimme a server and port...\n");
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
