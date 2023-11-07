#include "headers.h"

/**
 * @brief Establish a connection to a server using TCP
 *
 * @param port A server should be bound and listening to this
 * @return i32 file descriptor
 */
i32 connect_to_port(const i32 port)
{
  const i32 sockfd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(sockfd, -1);

  struct sockaddr_in addr;
  memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(LOCALHOST);
  CHECK(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)), -1);

  return sockfd;
}

/**
 * @brief Establish a server using TCP
 *
 * @param port
 * @return i32 file descriptor
 */
i32 bind_to_port(const i32 port)
{
  const i32 serverfd = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(serverfd, -1);

  struct sockaddr_in server_addr, client_addr;
  memset(&server_addr, '\0', sizeof(server_addr));
  memset(&client_addr, '\0', sizeof(client_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);

  CHECK(bind(serverfd, (struct sockaddr *)&server_addr, sizeof(server_addr)), -1);
  CHECK(listen(serverfd, MAX_CONNECTIONS), -1);

  return serverfd;
}

/**
 * @brief Get the port number of a socket
 *
 * @param fd file descriptor of the socket
 * @return i32 port number
 */
i32 get_port(const i32 fd)
{
  struct sockaddr_in server_addr;
  socklen_t len = sizeof(server_addr);
  CHECK(getsockname(fd, (struct sockaddr *)&server_addr, &len), -1);

  return ntohs(server_addr.sin_port);
}

void send_file(FILE *f, const i32 sockfd)
{
  char buffer[MAX_STR_LEN] = {0};

  while (fgets(buffer, MAX_STR_LEN, f) != NULL)
  {
    CHECK(send(sockfd, buffer, sizeof(buffer), 0), -1)
    bzero(buffer, MAX_STR_LEN);
  }
}

void receive_and_print_file(const i32 sockfd)
{
  char buffer[MAX_STR_LEN];
  while (1)
  {
    const i32 size = recv(sockfd, buffer, sizeof(buffer), 0);
    CHECK(size, -1);
    if (size == 0)
      return;
    printf("%s", buffer);
    bzero(buffer, MAX_STR_LEN);
  }
}
