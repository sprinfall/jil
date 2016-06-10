# Jil Text

Yet another text editor developed with C++ and wxWidgets.

[[https://github.com/sprinfall/jil/blob/master/doc/screenshots/main%20window.png|alt=main_window]]

## Advantages

* Cross platform (Windows, Linux and Mac).
* Intuitive and beautiful interface.
* Low memory usage, super fast.
* GUI based configuration: support both global and file type specific options.
* Extensible indent functions using Lua script.

## Disadvantages

* No plugin support.
* No resources, no documentation.

## Build Instructions

The build system of Jil Text is based on CMake, with which you can generate Visual Studio solution, XCode project or Unix makefiles.

### Linux (Ubuntu)

Install CMake:
```
$ sudo apt install cmake cmake-qt-gui
```
The GUI version "cmake-qt-gui" is optional.

Install C++ Boost Library:
```
$ sudo apt install libboost-dev
```

Install wxWidgets:
```
$ sudo apt install libwxgtk3.0-dev
```

Suppose the source code is cloned to ~/jil, create a build directory next to it:
```
$ mkdir ~/jilbuild
```

Generate Unix makefiles with CMake:
```
$ cd ~/jilbuild
$ cmake -G"Unix Makefiles" -DCMAKE_INSTALL_PREFIX=~/jilbuild/src/app ../jil
```

Now build it with make:

```
$ make
```

Note that CMAKE_INSTALL_PREFIX is set to the directory where the executable file is located. The resource files will also be installed to this directory by:
```
$ make install
```

You may want to create a symbolic link in ~/bin to the executable:
```
$ ln -s ~/jilbuild/src/app/jil ~/bin/jil
```

