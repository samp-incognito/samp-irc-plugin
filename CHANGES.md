SA-MP IRC Plugin
================

v1.4.8
------

- Fixed server password SSL connection bug
- Update: Fixed const correctness issues in include file (thanks
  spacemud)

v1.4.7
------

- Fixed IRC_OnUserNickChange bug

v1.4.6
------

- Fixed IRC_Quit bug (thanks Renegade334)
- Added E_IRC_RESPAWN option to IRC_SetIntData to set whether bots
  automatically reconnect upon disconnection (thanks Renegade334)

v1.4.5
------

- Added server password parameter to IRC_Connect

v1.4.4
------

- Made plugin attempt to resolve addresses again (with maximum number
  of attempts set to 5 by default) if resolving ever fails
- Disabled receive timeout by default
- Added IRC_OnReceiveNumeric

v1.4.3
------

- Fixed a bug that could possibly occur if the server never closes
  the connection upon using IRC_Quit
- Added client-side timeout functionality with a configurable
  setting (E_IRC_RECEIVE_TIMEOUT using IRC_SetIntData with a default
  value of 180 seconds)
- Updated libraries, optimized some code, and fixed a few small bugs
- Added IRC_OnInvitedToChannel

v1.4.2
------

- Fixed crash that sometimes occurred when the IRC server closed the
  socket
- Fixed bug with IRC_GetUserChannelMode that caused it not to return
  "-" when the user had no mode

v1.4.1
------

- Added CTCP natives and callbacks

v1.4
----

- Rewrote networking code to support asynchronous I/O
- Added functionality for trying each IP address linked to an IRC
  server's hostname
- Added IRC_SetIntData for adjusting a bot's connect attempts,
  connect delay, and connect timeout
- Added additional parameters to IRC_OnConnect and IRC_OnDisconnect
  for determining the IP address and port on which the bot connects
- Added IRC_OnConnectAttempt, IRC_OnConnectAttemptFail,
  IRC_OnKickedFromChannel, and IRC_OnUserKickedFromChannel
- Update: Fixed bug with maintaining user lists on multiple channels
- Update: Fixed bug with parsing user lists on non-public channels

v1.3.6
------

- Resolved some deadlocking issues

v1.3.5
------

- Fixed internal user list storage bug and possible crash

v1.3.4
------

- Made the plugin strip the '%' character from all messages sent by
  the IRC server
- Increased internal buffer size and fixed message parsing bug
- Fixed bug in channel command system
- Improved quite a bit of code

v1.3.3
------

- Fixed IRC_OnReceiveRaw crash

v1.3.2
------

- Added SSL support
- Added ability to bind local IP addresses

v1.3.1
------

- Made some optimizations to the connection and message parsing code

v1.3
----

- Fixed bot and group ID assignment bug
- Improved channel command system

v1.2
----

- Added IRC_IsUserOnChannel native
- Added IRC_GetChannelUserList native
- Renamed IRC_SetChannelMode to IRC_SetMode

v1.1
----

- Fixed bug that crashed the server when internal callback queue
  began to be filled
- Fixed bug with bots in groups that caused IRC_OnUserSay to not be
  called for individual private messages
- Added check to IRC_AddToGroup that prevents bots from being added
  to a group more than once
- Added extra argument to IRC_JoinChannel that lets bots specify a
  channel key

v1.0
----

- Initial release
