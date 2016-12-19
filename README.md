#HAT - Hotkey Abstraction Tool #

`HAT` is a simple console tool, which allows to simulate keyboard input to the currently active window on desktop. It uses android phone or tablet as a controller. The keyboard commands are organized and stored in special configuration files, which are passed to the `HAT` program when it starts.

## Requirements ##

### `HAT` tool (compiler requirements): ###

Requirements for building the tool code are:
 - C++14 - compatible compiler
 - `boost` libraries (`boost::asio` and `boost::program_options` are used in the project)

Also, `HAT` uses `ROBOT` c++ library for the keyboard simulation, so, please check the [requirements page](http://getrobot.net/docs/usage.html) for this library (there are different requirements for different platforms).

### Client device: ###
Android device, for the user input should have android v 3.0 or newer with `TAU client` app installed ([link to the app on google play store](https://play.google.com/store/apps/details?id=com.tau.client)).

## Compilation ##
### Visual Studio 2015: ###

The project has a set of solution and project files for Microsoft Visual Studio 2015. In order to build it, the following steps should be done:
 1. Build the static library for the `Robot` submodule:
  * Open the solution file for the submodule with vs2015: `src/external_dependencies/robot/Robot.vs15.sln`
  * Select the desired build configuration (`LIB` for release, or `LIBd` for debug versions of the library)
  * select the target platform (`Win32` or `Win64`)
  * Build the solution

 A *lib file will be placed into the `src/external_dependencies/robot/Binaries/[platform]` directory (`[platform]` could be either `Win32` or `Win64`, depending on configuration)
 2. Provide the paths for the `boost` include and library directories in order to build the main project:
   * Open the solution file for the main project: `src/hat.sln`
   * Provide the environment variables for the `boost` resources. This is done via the `User-defined macros` section of the project setup window. See the [MSDN documentation page](https://msdn.microsoft.com/en-us/library/669zx6zc.aspx#User-defined-macros) on how to do it. The following variables should be added:
     * include path for the `boost` headers: `BOOST_INCLUDE_DIR`
     * the library directories for the needed platforms and configurations (not all the variables need to be specified):
        * 32-bit debug libraries `BOOST_LIB32D_DIR`
        * 32-bit release libraries `BOOST_LIB32R_DIR`
        * 64-bit debug libraries `BOOST_LIB64D_DIR`
        * 64-bit release libraries `BOOST_LIB64R_DIR`

 After these steps, the project should be able to build without problems.

### gcc: ###
In order to compile the project with GCC compiler go into `src` folder of the project and execute `make all` command.

## Execution ##

The `hat` is a command line tool, which has the following arguments:

|command line argument|is optional?|description|
|---------|---------|---------|
|`--command`|no|The csv file that holds the information about the hotkeys. See the [description](doc/commands_config.md) for it.|
|`--layout`|no|The custom text configuration file for the layout, which is displayed to client. See the [description](doc/layout_config.md) for it.|
|`--keysDelay`|yes|The interval in milliseconds between each of the simulated keystrokes. The default value is 0 (no delays).|
|`--port`|yes|The port number, on which the tool will listen for the incoming connections. The default value is 12345.|
|`--stickEnvToWindow`|yes|The parameter, which, if specified, will instruct the tool to ensure that the simulated keyboard events are sent to a specific window.|

Please see the [general_design](doc/general_design.md) section for more details on the usage of the tool.