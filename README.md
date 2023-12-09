# Nifty File Storage World (NFSW)
A distributed file system over a network, implemented from scratch in C, using **only POSIX compliant** functions
![image](https://github.com/PraneethJain/nifty-file-storage-world/assets/49565677/a53e8b96-385e-4afc-b669-4c31ab66331e)

### Available Operations
- Read from a file
- Write to a file
- Get metadata of a file/folder
- Create file/folder
- Delete file/folder
- Copy file/folder

### Highlights
- Multiple clients are handled concurrently, each in their own thread, with race conditions handled
- Reader Writer lock used wherever applicable
- Detailed error messages
- Frequent logging
- Redundancy in case some storage servers go down
- LRU caching for efficient file/folder search

## Detailed Documentation
https://nifty-file-storage-world.web.app/

## Getting Started
- Clone the repository and `cd` into it
- Run `make`. This will build 3 executables, `naming_server.out`, `storage_server.out` and `client.out`.
- Run the naming server in any directory first only once.
- Run storage servers in their directories at any time. Input the number of inaccessible paths followed by the list of inaccessible paths in each storage server.
- Run clients in any directory at any time.

## References
- https://man7.org/linux/man-pages/
- https://docs.oracle.com/cd/E19455-01/806-5257/6je9h032u/index.html
- https://stackoverflow.com/questions/30440188/sending-files-from-client-to-server-using-sockets-in-c
- https://stackoverflow.com/questions/8842404/get-random-port-for-udp-socket
