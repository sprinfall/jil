# Jil Text

Yet another text editor developed with C++ and wxWidgets.

## Advantages

* Cross platform (Windows, Linux and Mac).
* Beautiful graphic user interface.
* Low memory usage, super fast.
* Simple configuration.

## Disadvantages

* No plugin support.
* No resources, no documentation.

## Build Instructions

Make sure CMake has been installed in your platform.

### Linux (Ubuntu)

Install CMake:
```
$ sudo apt install cmake cmake-qt-gui
```

Install C++ Boost Library:
```
$ sudo apt install libboost-dev
```

Install wxWidgets:
```
$ sudo apt install libwxgtk3.0-dev
```

Suppose the source code is cloned to ~/jil, make a build directory just next to it
```
$ mkdir ~/jilbuild
```

Generate Unix make files with CMake.
```
$ cd ~/jilbuild
$ cmake -G"Unix makefiles" ../jil
```

