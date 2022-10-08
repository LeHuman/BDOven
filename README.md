Teensy 3.6 Project Template
===========================
 
Purpose
-------

An easy starting point for Teensy 3.6 projects on Windows.  
This template is meant to be used along side the TeensyToolchain.  
This setup is not meant for Unix systems.  
This setup is also meant only for the Teensy 3.6 although some tweaking can be done to be used on other boards.

Software
-----

This template assumes you installed the following.

- [VSCode](https://code.visualstudio.com/)
  - This template is based around VSCode
- [CMake](https://cmake.org/download/)
  - Make sure to select the `add CMake to path` option when installing
- [Git](https://git-scm.com/download)
  - For version control of this repository

Setup
-----

Download [TeensyToolchain](https://github.com/LeHuman/TeensyToolchain), unzip, and put it next to this template folder.

If it is not named already, rename the downloaded folder from to `TeensyToolchain`  
e.g. `TeensyToolchain-master` -> `TeensyToolchain`

Alternatively, you can modify the `TOOLCHAIN_OFFSET` option inside `.vscode/settings.json` in order to tell the project where the toolchain is. This path is relative to this template folder.

Inside `.vscode/settings.json`, modify the options for `TEENSY_USB_PORTNAME` for the port that the project should use to connect to your teensy. ( Set to COM9 by default )  
You can also modify the default values for other tasks in this file.

Using
-----

Put your code in `src/main.cpp`  
Put any libraries you need in `libraries`

### VSCode Tasks

* `Clean`: Clean up build files
* `Reset`: Clean up entire CMake project
* `Build`: Compile project
* `Upload`: Upload compiled binary to teensy
* `Monitor`: Monitor teensy over a CLI, select the appropriate options when running, defaults to `ASCII` mode

By default, `Ctrl + Shift + B` (Run Build Task) should run `Build`, `Upload`, and `Monitor` in that order.

Alternatively, install the [Tasks](https://marketplace.visualstudio.com/items?itemName=actboy168.tasks) extension so that tasks appear on the status bar.

### CMake

You can modify the set values and compile flags inside `CmakeLists.txt` if you know what you are doing.  
They are mostly under the function calls `add_compile_definitions` and `add_compile_options` towards the beginning.
