
# General design principals of the tool #

The `hat` tool is used for simulation of keyboard input for desktop applications. It's primary use is to send commands to the application, which are usually sent via a hotkey combination. It also can be used to simulate typing of some commonly used words or phrases automatically, or preforming some other repetitive keyboard input.

## Terminology ##

_client device_ - a touchscreen device that is used to connect to the `hat` tool and send commands to it.

_session_ - a period of time, when the client device is connected to the tool, allowing the user to send commands to it.

_command_ - a specific action that should be preformed in the application. Every action that can be simulated with the `hat` tool has a _command_ associated with it.

_environment_ - a set of rules, which tell the `hat` tool, which keys should be simulated in order to execute each given command. At any moment in time, only one environment can be active. For each of the _sessions_, there could be one or several _environmens_ defined. If there are several environments, the user will have to select one of them.

### Example ###
Let's consider 3 common commands, which are commonly used in different IDEs - `save file`, `find and replace text`, `start the debugger`.

Here is the table of keyboard shortcuts for these commands in 3 common IDEs (eclipse, IDEA, visual studio):

|environment| command 1: save file| command 2: find and replace| command 3: start debugger|
|------------ | ------------- | ------------- | -------------|
|eclipse|Ctrl+S|Ctrl+F|F11|
|IDEA|Ctrl+S|Ctrl+R|Shift+F9|
|visual studio|Ctrl+S|Ctrl+H|F5|

Note that the actual combinations differ for two out of three commands here. This is why the concept of the _environment_ is introduced.

The _environments_ are used to maintain disentanglement between the commands (abstract idea, what the desktop application should do) and the hotkey (actual keys that need to be pressed in order to execute that command).

## How the tool is used ##
Basically, during each _session_, the user has a fixed set of _commands_, which can be executed on the tool side.
The user activates one of the available in this session _environments_. After that, for each of the _commands_, a specific hotkey combination is attached. Now, each time a _command_ is requested, the needed combination is simulated to the target application.

Information about both _commands_ and _environments_ is fed into the `hat` tool through special configuration file at the start of the tool execution (see the [commands config format description](commands_config.md) for this file).
Another piece of information for the tool is the way the buttons for the commands are arranged on the _client device_. This is done through another config file (see the [layout config format description](layout_config.md) for this file).
