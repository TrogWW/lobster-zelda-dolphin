To build a new version of the Dolphin Python DLL for your computer:

1. Enter this directory from the command line, and create a directory called "build" (ex. "mkdir build")
2. cd into this new directory (i.e. "cd build")
3. Run "cmake .." (make sure you have cmake downloaded).

On Windows, this will create a Visual Studio Solution file in the build folder called PythonDLL.sln. Open this up with Visual Studio, and in the menu bar at the top of the screen, click on "Build" > "Rebuild Solution" to generate the new DLL.

On other operating systems, the "make" command (or some similar command) should be available on the command line to build the DLL at this step.

Now, the next time you rebuild the main dolphin-emu.sln solution file, the Python DLL will be copied to the correct location to be available to Dolphin at runtime.