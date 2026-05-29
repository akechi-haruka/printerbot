printerbot (roll out!)
CHC-series shim DLL to convert between printers
2024 Haruka
Licensed under the GPLv3.

Note: Firmware updating is blocked.
Note: Cards WILL be stretched, simply because the cards between printers are different size. (A bicubic filter is used.)

Tested with:
CHC-310 -> CHC-310
CHC-310 -> CHC-310
CHC-320 -> CHC-320
CHC-320 -> CHC-330
CHC-330 -> CHC-310
CHC-330 -> CHC-330

--- Usage ---

* Place chcusb.dll and chcfwdl.dll into the game folder.
* Append _orig to the original filenames that already exist. (ex. C310Ausb_orig.dll, C310AFWDLUsb_orig.dll)
* Rename chcusb.dll and chcfwdl.dll to whatever the original files were called.
* Copy the real DLLs from your target printer model into the game folder (ex. C330Ausb.dll, C330AFWDLUsb.dll)
* Edit printerbot.ini that incoming and outgoing model numbers are correct.
* Make sure segatools printer hooks are disabled.
* Start the game (no inject/launch.bat changes required)

--- Compiling ---

have msys2 installed at the default location and run compile.bat

or use CLion

--- Other ---

In memory of all servants and nice boats that wanted to be, but sacrificed themselves for the greater good of this project.

https://puu.sh/Kaf2x/6cb4e0e239.jpg