# Build Instructions (Mac)

MacOS Version: 10.12.3

## Install Xcode

Firstly, please install Xcode from App Store.


## Install wxWidgets

Download the latest version wxWidgets from: http://wxwidgets.org/. Currently, it's 3.1.0.
Unzip the package, execute the following commands:
```bash
$ cd wxWidgets-3.1.0
$ mkdir build-osx && cd $_
$ ../configure --enable-debug --disable-shared --disable-stc --disable-mediactrl --with-osx_cocoa CXX='clang++ -std=c++11 -stdlib=libc++' CC=clang --with-macosx-version-min=10.10
```
The key point is build in a separate directory to avoid ruining the source tree.

Configure options:
- `--enable-debug` Build wxWidgets in debug mode. Normally you don't need it. (libwx-baseu-3.1.a is nearly 41MB in debug while only 4MB in release.)
- `--disable-shared` Build static libraries only.
- `--disable-stc` Don't build "stc" because we don't need it.
- `--disable-mediactrl` Disable "mediactrl" because it leads to compilation errors.

Note that C++11 is enabled for wxWidgets.
The minimal version of the target MacOS is specified as 10.10. You can change it to 10.7, 10.8, etc.

Build with make:
```bash
$ make -j4
$ make install
```
You might need to use `sudo make install` to install.
After installation, you can find static libraries in "/usr/local/lib/", and header files in "/usr/local/include/wx-3.1/wx".
If you want to install to HOME instead, add `--prefix=$HOME` to configure.

If you want to verify the build, run a sample.
Start from "build-osx" folder:
```bash
$ cd samples/dialogs/
$ make
$ open dialogs.app
```
You should see a dialog.


## Build Libconfig

Libconfig provides a configure format similar to JSON, it's widely used by Jil for preferences.

Download libconfig from http://www.hyperrealm.com/libconfig/.
Unzip the package, execute the following commands:
```bash
$ ./configure --prefix=$HOME --disable-cxx --disable-shared --disable-examples CC=clang
$ make -j4
$ make install
```
The difference from building wxWidgets is that we don't create a sub folder "build-osx". If we do it, the test code won't find the header file.
Please also note that we install it to HOME. This is required because the path is hard coded in Jil's CMakeLists.txt. To be fixed.
Configure options:
- `--disable-cxx` Don't build C++ wrapper. We only use C APIs.
- `--disable-shared` Build static libraries only.
- `--disable-examples` Don't build examples.

After installation, you can find static libraries in "~/lib/", and header files in "~/include/".


## Install C++ Boost Library

Download latest version Boost from http://www.boost.org/. Currently, it's 1.63.
Unzip the package, execute the following commands:
```bash
$ cd boost_1_63_0
$ ./bootstrap.sh
$ ./b2 --with-system link=static variant=release install
```
Configure options:
- `--with-system` Only build "system" library. Actually, Jil only depends on Boost header-only libraries. Build "system" so that other libraries won't be built.
- `link=static` Build static libraries only.
- `variant=debug` Build debug version libraries. Another option is "release".

If you want to install Boost to HOME instead of /usr/local, add `--prefix=$HOME` to b2.


## CMake

Download CMake DMG file from https://cmake.org/.
Install it, open it.
Specify the folder of source code and the folder to build. E.g.,

- **Where is the source code**: /Users/adam/github/jil
- **Where to build the binaries**: /Users/adam/github/jil/build_make

The build folder is where Makefiles or Xcode project will be genereted.

If your wxWidgets or Boost has been installed to HOME instead of /usr/local, add HOME to CMAKE_INSTALL_PREFIX:
> CMAKE_INSTALL_PREFIX = ~;/usr/local

Otherwise, CMake might not be able to find them.

Press Configure button to configure, select a generator, "Unix Makefiles" or "Xcode".
After configure, check logs to see if everything is OK. If you see two lines like this:
> Found wxWidgets: -L/usr/local/lib; ...
> Boost version: 1.63.0
It means that wxWidgets and Boost have been found.

But you will also notice the following lines about installation:
> [debug] CMake install prefix: /usr/local
> Mac install bin dir: /usr/local/jil.app/Contents/MacOS
> Mac install res dir: /usr/local/jil.app/Contents/Resources

Well, "/usr/local/" is not a good place for the bundle during development. So change `CMAKE_INSTALL_PREFIX` again as below:
> CMAKE_INSTALL_PREFIX = /Users/adam/github/jil/build_make/src/app
If the generator is "Xcode", it should be:
> CMAKE_INSTALL_PREFIX = /Users/adam/github/jil/build_make/src/app/Debug

Now configure again, then press Generate button to generate the Makefiles or Xcode project.

Execute the following commands in "build_make" folder:
```
$ make -j4
$ make install
```
If everything is OK, you can open the bundle with:
```
$ open src/app/jil.app
```

For Xcode generator, open jil.xcodeproj, build target "app", then build target "install".
Switch back to target "app" and you can run.

