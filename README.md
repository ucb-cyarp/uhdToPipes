# uhdToPipes
A utility program which connects UHD to Linux/POSIX Pipes

**NOTE: Due to the GPLv3 License for UHD and GNURadio, this utility is licensed under GPLv3.**

Use `git submodule update --init` when initially cloning the reposiory

## Dependencies
This project depends on [UHD](https://github.com/EttusResearch/uhd) and [GNURadio](https://github.com/gnuradio/gnuradio).
* UHD: Used for communication with USRPs (must be installed)
* GNURadio: For the `FindUHD.cmake` file which is used for discovering the UHD install
