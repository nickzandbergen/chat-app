Nick Zandbergen

******
README
******

FILES
  > bin <- doesn't exist until you run make

  > doc
    DESIGN <- Describes design
    README <- You are here

  > src
    client.cpp  <- source code of client
    server.cpp  <- source code of server
    auxlib.cpp  <- utility functions
    auxlib.h    <- includes
	
  Makefile <- note that "make" does a lot of stuff

Assumptions
- Both client and server agree on endianness
- map's keys are ordered alphabetically by code point is ok
- Some operations on ints are atomic or an interrupt won't happen
  at the checks and leave a partial value

Limitations

- See line 4

- /connect parsing is incredibly lazy and assumes 1 space after connect then a nonzero length ID.
  - "/connect" without a space isn't recognized

- /list is alphabetical by code point: so 'b' is between 'A' and 'a'

- Prompt displaying is reliant on rl_clear_visible_line()
  - From https://ftp.gnu.org/gnu/readline/ 

- Does not have empty prompts scroll up
- Indexes messages with uint16_t, which could theoretically overflow

******
DESIGN
******

SERVER-CLIENT INTERACTION

Every single client/server follows a query and optional response. The client opens and closes
a new socket for each request because TCP has no packet boundaries and SCTP isn't on the timeshare.

PROTOCOL
Protocols are identified by lowercase letters, which is the first byte the client sends.

The server supports the following:
  a) availability check
    - also includes port of sending client

  w) wait a client
    - sets a client's waiting to true

  u) unwait a client
    - sets a client's waiting to false

  r) remove a client

  c) connection details request

  l) list clients
All protocol codes are followed by an ID. Clients are trusted.

The server does not deal with client liveliness. 
If a client attempts to connect to another client in the list of 
waiting clients and fails, it removes the presumably dead client.
As such, there can be a dead client on the waiting list only until
someone attempts to connect to it: then it is removed. Any silent
dead client not waiting will take up an ID, but that shouldn't be
an issue since my program will crash long before the 62^255 IDs are
taken.

CHATTING
Chatting is entirely between clients.
- A client is always listening with select()

A chat connection between A and B (initiated by A) does as follows
1) A queries the server for B's INFO
  - if B doesn't exist or isn't waiting, A stops.
2) A sends B a request to chat: the character 'c'
  - if connect fails for A, it stops and removes B's info from the server
3) B responds with either a yes or no to chat, depending on if it's waiting
  - if A receives a no, it stops
  - At some point in the above, B receives A's ID from it.

After which, both of them are chatting: messages are prefixed with their
length and the prompt and actual body are set as two separate messages.
A client leaving is noted by the socket closing

CLIENT
  In general, the client is a very large while loop with a select() waiting for things to happen.
There's many, many, if/else statements and switches because of the different behaviour in states.
This is compounded with branching options: the worst of it is triaged into functions.
