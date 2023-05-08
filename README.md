# Simple C chat server

## Server
The server uses a linked list for multiple clients connection.

It can authenticate username and password sent from the client by checking
the file `userpass.txt`.
The username & password are stored in a simple format, `username password`.

When it receive a message from a user,
it send the message to all other users w/ send_to_all_clients.

When a client exit, it also notify other clients.

The server also has a prompt to ask if they want to close down the server
when <kbd>Ctrl-C</kbd> is pressed.

## Client
The client uses two threads to send and receive messages.
When the user open up the client, there is a prompt for them to type in their username & password.

The username & password are send to the server to authenticate.
If the authentication failed, close the client.
The attempted username & password are print out in the server.

It also detects when the user typed `\something` and will not send that message to the server.
If the `\something` is a valid command, like `\q`, the client will exit (no prompt).

If the server closed down, the client will also close down immediately.

The <kbd>Ctrl-C</kbd> handler is buggy to implement so I commented it out (explained further in the code).

## Commands
| command | description |
|---------|-------------|
|`\q`|exit|
<!-- |`\p <password>`|change the password of the login username| -->



## Possible features / TODO list

- [X] store username & password in local file
- [X] when username entered, ask for password
- [X] send to server to check
- [ ] if no then restart back to the username prompt
- [ ] change password via \p
- [ ] ban list

