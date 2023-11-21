/**
 * @file network.c
 * @brief Contains all the functions related to networking.
 * @details 
 *    - Functions for binding to port, connecting to port and finding port.
 *    - Functions for sending, receiving and transmitting files.
 *    - Functions for sending and receiving a large buffer in small packets.
 */

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

  struct sockaddr_in server_addr;
  memset(&server_addr, '\0', sizeof(server_addr));
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

/**
 * @brief Send a file in chunks of char arrays
 *
 * @param f file pointer of the file to be sent
 * @param sockfd socket to which the file is to be sent
 */
void send_file(FILE *f, const i32 sockfd)
{
  char buffer[MAX_STR_LEN] = {0};
  while (fgets(buffer, MAX_STR_LEN, f) != NULL)
  {
    CHECK(send(sockfd, buffer, sizeof(buffer), 0), -1)
    bzero(buffer, MAX_STR_LEN);
  }
}

void send_data_in_packets(void *buffer, const i32 sockfd, u32 buffer_length)
{
  i16 numpackets = buffer_length/MAX_STR_LEN;
  for (i32 i=0; i<numpackets; i++)
  {
    CHECK(send(sockfd, buffer + MAX_STR_LEN*i, MAX_STR_LEN, 0), -1);
  }
  if (buffer_length % MAX_STR_LEN != 0)
  {
    CHECK(send(sockfd, buffer + MAX_STR_LEN*numpackets, buffer_length % MAX_STR_LEN, 0), -1);
  }
}

void receive_data_in_packets(void *buffer, const i32 sockfd, u32 buffer_length)
{
  i16 numpackets = buffer_length/MAX_STR_LEN;
  for (i32 i=0; i<numpackets; i++)
  {
    CHECK(recv(sockfd, buffer + MAX_STR_LEN*i, MAX_STR_LEN, 0), -1);
  }
  if (buffer_length % MAX_STR_LEN != 0)
  {
    CHECK(recv(sockfd, buffer + MAX_STR_LEN*numpackets, buffer_length % MAX_STR_LEN, 0), -1);
  }
}

/**
 * @brief Receive a file and print it to stdout
 *
 * @param sockfd socket from which the file is to be received
 */
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


/**
 * @brief transmit file in small packets.
 * 
 * @param f File that is to be sent.
 * @param sockfd socket that is being sent to.
 */
void transmit_file_for_writing(FILE *f, const i32 sockfd)
{
  char buffer[MAX_STR_LEN] = {0};
  i32 num_bytes_read = 0;
  i32 i = 0;
  while (1)
  {
    if (i == 100)
    {
      i = 0;
      CHECK(recv(sockfd, &num_bytes_read, sizeof(num_bytes_read), 0), -1)
    }
    num_bytes_read = fread(buffer, 1, MAX_STR_LEN, f);
    CHECK(num_bytes_read, -1);
    CHECK(send(sockfd, &num_bytes_read, sizeof(num_bytes_read), 0), -1)
    if (num_bytes_read == 0)
      break;

    CHECK(send(sockfd, buffer, num_bytes_read, 0), -1)
    bzero(buffer, MAX_STR_LEN);
    i++;
  }
  CHECK(recv(sockfd, &num_bytes_read, sizeof(num_bytes_read), 0), -1)
}

/**
 * @brief Receive file from one socket and send it to other.
 * 
 * @param from_sockfd the socket that the file is being sent from.
 * @param to_sockfd the socket that the file has to be sent to.
 */
void receive_and_transmit_file(const i32 from_sockfd, const i32 to_sockfd)
{
  char buffer[MAX_STR_LEN];
  i32 num_bytes_read = 0;
  i32 i = 0;
  while (1)
  {
    if (i == 100)
    {
      i = 0;
      CHECK(recv(to_sockfd, &num_bytes_read, sizeof(num_bytes_read), 0), -1)
      CHECK(send(from_sockfd, &num_bytes_read, sizeof(num_bytes_read), 0), -1)
    }
    CHECK(recv(from_sockfd, &num_bytes_read, sizeof(num_bytes_read), 0), -1)
    if (num_bytes_read > MAX_STR_LEN || num_bytes_read < 0)
      num_bytes_read = MAX_STR_LEN;
    CHECK(send(to_sockfd, &num_bytes_read, sizeof(num_bytes_read), 0), -1)
    if (num_bytes_read == 0)
      break;

    CHECK(recv(from_sockfd, buffer, num_bytes_read, 0), -1)
    CHECK(send(to_sockfd, buffer, num_bytes_read, 0), -1)

    bzero(buffer, MAX_STR_LEN);
    i++;
  }
  CHECK(recv(to_sockfd, &num_bytes_read, sizeof(num_bytes_read), 0), -1)
  CHECK(send(from_sockfd, &num_bytes_read, sizeof(num_bytes_read), 0), -1)
}

/**
 * @brief Receive file content from a socket and write it into a file.
 * 
 * @param from_sockfd The socket that the file content is being received from.
 * @param f The file that has to be written to.
 */
void receive_and_write_file(const i32 from_sockfd, FILE* f)
{
  char buffer[MAX_STR_LEN];
  i32 num_bytes_read = 0;
  i32 i = 0;
  while (1)
  {
    if (i == 100)
    {
      i = 0;
      CHECK(send(from_sockfd, &num_bytes_read, sizeof(num_bytes_read), 0), -1)
    }
    CHECK(recv(from_sockfd, &num_bytes_read, sizeof(num_bytes_read), 0), -1)
    if (num_bytes_read > MAX_STR_LEN || num_bytes_read < 0)
      num_bytes_read = MAX_STR_LEN;
    if (num_bytes_read == 0)
      break;

    CHECK(recv(from_sockfd, buffer, num_bytes_read, 0), -1);
    fwrite(buffer, num_bytes_read, 1, f);

    bzero(buffer, MAX_STR_LEN);
    i++;
  }
  fclose(f);
  CHECK(send(from_sockfd, &num_bytes_read, sizeof(num_bytes_read), 0), -1);
}
