# uhdToPipes

A utility program which connects UHD to Linux/POSIX Pipes

**NOTE: Due to the GPLv3 License for UHD and GNURadio, this utility is licensed under GPLv3.**

Use `git submodule update --init` when initially cloning the reposiory

## Dependencies
This project depends on [UHD](https://github.com/EttusResearch/uhd) and [GNURadio](https://github.com/gnuradio/gnuradio).
* UHD: Used for communication with USRPs (must be installed)
* GNURadio: For the `FindUHD.cmake` file which is used for discovering the UHD install

## Citing This Software:
If you would like to reference this software, please cite Christopher Yarp's Ph.D. thesis.

*At the time of writing, the GitHub CFF parser does not properly generate thesis citations.  Please see the bibtex entry below.*

```bibtex
@phdthesis{yarp_phd_2022,
	title = {High Speed Software Radio on General Purpose CPUs},
	school = {University of California, Berkeley},
	author = {Yarp, Christopher},
	year = {2022},
}
```
