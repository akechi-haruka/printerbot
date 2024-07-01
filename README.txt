printerbot (roll out!)
CHC-series shim DLL to convert between printers
2024 Haruka
Licensed under the GPLv3.

Tested on CHC330->310.

Note: Firmware updating is blocked.

--- Usage ---

* Place CHCXXXusb.dll and CHCXXXFWDLUsb.dll into game folder.
    * If the existing printer is a CHC-330, use CHC330Ausb.dll instead
* Backup the original files and rename the shim files to your printer model (ex. C310Ausb.dll, C310AFWDLUsb.dll)
* Copy the real DLLs from your target printer into the game folder (ex. C330Ausb.dll, C330AFWDLUsb.dll)
* Edit printerbot.ini that incoming and outgoing model numbers match.
* Make sure segatools printerhooks are disabled.
* Start the game (no inject required for printerbot)

--- Compiling ---

have msys2 installed at the default location and run compile.bat

or use CLion
