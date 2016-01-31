SA-MP IRC Plugin
================

This plugin allows for the creation and management of IRC bots through the SA-MP server. There are many features, including:

- SSL support and local IP address binding
- Automatic reconnection attempts with configurable options
- Bot grouping (built-in support for message distribution to prevent
  accidental flooding)
- No limit on number of concurrent bot connections (support for
  multiple servers)
- Efficient channel command system (slightly modified zcmd by Zeex)

Compilation (Windows)
---------------------

Open the solution file (irc.sln) in Microsoft Visual Studio 2015 or higher. Build the project.

Compilation (Linux)
-------------------

Install the GNU Compiler Collection and GNU Make. Type "make" in the top directory to compile the source code.

Download
--------

The latest binaries for Windows and Linux can be found [here](https://github.com/samp-incognito/samp-irc-plugin/releases).

Documentation
-------------

More information can be found in the [SA-MP forum thread](http://forum.sa-mp.com/showthread.php?t=98803) as well as the README file in the binary package.
