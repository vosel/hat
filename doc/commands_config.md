## Configuring the hotkeys and environments ##

All the information about the simulated hotkeys is passed to the tool through a `--command` command line option.
This option specifies a filename for the file, which format is described below.


### Approach ###
The configuration file is a simple csv-style text file, with the `\t` symbols used as delimiters. So, it the easiest way to understand it is to think of it as a table. Each row represents a command. All the information about the command is stored in various cells.

The purpose of each cell can be seen in the first row, which does not represent a command, but holds the columns names. Here is the example of this row:
```
command_id	command_category	command_note	command_description	Environment1	Environment2
```
This is a headers row for a configuration with 2 environments.

The first 4 cells hold different service information about the command. Here is the table describing these cells purpose:

|cell number|description|restrictions|
|-----------|-----------|------------|
|0|`command_id` - a command ID, which should be unique between all the commands and other IDs in the setup| allowed symbols:`A`-`Z`, `a`-`z`, `0`-`9`, `_`|
|1|`command_category` - a string representing a category of the command (currently not used in the tool)| allowed symbols:`A`-`Z`, `a`-`z`, `0`-`9`, `_`|
|2|`command_note` - a note, which will be written on the button for the given command (should be kept reasonably short, but informative for the user)| not allowed symbols: `\n`, `\t`, `"`|
|3|`command_description` - full user-readable description of the command|not allowed symbols: `\n`, `\t`|

After that, all the rest of the cells represent keyboard input descriptions for the command in different environments. In our case, we have 2 environments, but it is not mandatory. At least one environment should be defined, there is no upper limit for the allowed amount of the defined environments.

The keyboard input format is described in the [documentation of the `Robot` c++ library](http://getrobot.net/api/keyboard.html#Compile), which is used for the keyboard simulation in this project.

So, here is an example of the configuration file for the commands from the [general design](general_design.md) section of this documentation.
The environments defined there are representing one of the 3 different IDEs: Eclipse, IDEA, Visual Studio.
The commands are `save file`, `replace text`, `start debugger`.
The configuration file in this case should hold the following data (link to this file: [commands_config_commands.csv](../config_samples/commands_config_commands.csv)):

|command_id|command_category|command_note|command_description|eclipse|IDEA|Visual Studio
|------|------|------|------|------|------|------|
|saveFile|file_operations|save|save the current file|^S|^S|^S|
|replaceText|text_editor|find and replace|find and replace in the file|^F|^R|^H|
|startDebugger|debugging|debug|start in debugger|{F11}|+{F9}|{F5}|

So, to use these commands, their `id`s should be placed inside the layout configuration file (see [format description](layout_config.md) for it).

The [layout configuration file](layout_config.md) for this example can be taken from [here](../config_samples/commands_config_layout.info). Here is how it looks like:
```
page:main page
saveFile
replaceText
startDebugger
```

In order to use these configuration files the following command should be executed (with the configuration files linked from this page):

```
> hat --layout commands_config_layout.info --command commands_config_commands.csv
```