printerbot (roll out!)
CHC-series shim DLL to convert between printers
2024 Haruka
Licensed under the GPLv3.

Note: Firmware updating is blocked.
Note: Cards WILL be stretched, simply because the cards between printers are different size. (A bicubic filter is used.)

Tested with:
CHC-310 -> CHC-310
CHC-310 -> CHC-310
CHC-320 -> CHC-330
CHC-330 -> CHC-310
CHC-330 -> CHC-330

--- Usage ---

* Place CHCXXXusb.dll and CHCXXXFWDLUsb.dll into game folder.
    * If the original printer is a CHC-330, use CHC330Ausb.dll instead, and rename CHCXXXusb.dll to chcusb.dll
* Backup the original files and rename the shim files to your printer model (ex. C310Ausb.dll, C310AFWDLUsb.dll)
    * If the incoming and outgoing model are the same (= you're sniffing printer communication), append "_orig" to the filename (ex. C310Ausb_orig.dll / C310AFWDLUsb_orig.dll)
* Copy the real DLLs from your target printer into the game folder (ex. C330Ausb.dll, C330AFWDLUsb.dll)
* Edit printerbot.ini that incoming and outgoing model numbers match.
* Make sure segatools printer hooks are disabled.
* Start the game (no inject required for printerbot)

--- Compiling ---

have msys2 installed at the default location and run compile.bat

or use CLion

--- Other ---

In memory of all servants and nice boats that wanted to be, but sacrificed themselves for the greater good of this project.

https://puu.sh/Kaf2x/6cb4e0e239.jpg