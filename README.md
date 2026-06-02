# Multi-Room TCP Chat Server

A terminal-based multi-room chat application written in pure C.

The project uses POSIX TCP sockets for networking and pthreads for concurrency. Users join a room by entering a room code. Anyone with the same room code can join that room and chat in real time.

## Features

- User-chosen username
- User-chosen room code
- Any number of users can join the same room code
- Same username allowed in different rooms
- Duplicate usernames rejected inside the same room
- Real-time message broadcasting
- Sender sees their own messages as `You: message`
- Other users see messages as `username: message`
- `/help` command
- `/quit` command
- Thread-per-client server design
- Mutex-protected room list
- In-memory rooms only
- Rooms are deleted when empty

## Folder Structure

```text
chat-server/
├── server/
│   ├── main.c
│   ├── room.c
│   ├── room.h
│   ├── client_handler.c
│   └── client_handler.h
├── client/
│   └── client.c
├── Makefile
└── README.md
```

## Requirements

Use Linux or WSL.

Install build tools:

```sh
sudo apt update
sudo apt install build-essential
```

If your WSL environment does not have `sudo`, run the same commands as root:

```sh
apt update
apt install build-essential
```

## Compile

From inside the project folder:

```sh
make
```

This creates:

```text
server/server
client/client
```

You can also compile manually:

```sh
gcc -Wall -Wextra -Werror -std=c11 -pthread -o server/server server/main.c server/room.c server/client_handler.c
gcc -Wall -Wextra -Werror -std=c11 -pthread -o client/client client/client.c
```

## Run

Start the server:

```sh
./server/server 8080
```

Open a second terminal and start a client:

```sh
./client/client 127.0.0.1 8080
```

Open more terminals and run the same client command for more users:

```sh
./client/client 127.0.0.1 8080
```

Each client will be asked for:

```text
Enter Username:
Enter Room Code:
```

Users who enter the same room code will join the same chat room.

## Example

Terminal 1:

```sh
./server/server 8080
```

Terminal 2:

```sh
./client/client 127.0.0.1 8080
```

Input:

```text
Enter Username: Rushil
Enter Room Code: ABC123
```

Terminal 3:

```sh
./client/client 127.0.0.1 8080
```

Input:

```text
Enter Username: John
Enter Room Code: ABC123
```

If Rushil sends:

```text
Hello
```

Rushil sees:

```text
You: Hello
```

John sees:

```text
Rushil: Hello
```

## Chat Commands

Show available commands:

```text
/help
```

Leave the chat:

```text
/quit
```

## Clean Build Files

Remove compiled binaries:

```sh
make clean
```

## Notes

- This project does not use a database.
- Rooms and users exist only in memory.
- All rooms are removed when the server shuts down.
