@echo off
cd %~dp0
mkdir dist

copy buildDir\chcfwdl\chcfwdl.dll L:\SDVX\kantai7\C310FWDLusb.dll
rem copy buildDir\chcusb\chcfwdl.dll dist\C310Ausb.dll
copy buildDir\chcfwdl\chcfwdl.dll dist\CHCXXXFwdlusb.dll
copy buildDir\chcusb\chcfwdl.dll dist\CHCXXXusb.dll
copy printerbot.ini dist\printerbot.ini
C:\msys64\usr\bin\bash --login -c "DIR=%cd:\=\\\\%; cd $(cygpath $DIR) && find ./ -type f -name *.dll -exec strip -s {} \;"
