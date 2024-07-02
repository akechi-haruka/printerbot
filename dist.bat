@echo off
cd %~dp0
mkdir dist

copy buildDir\chcfwdl\chcfwdl.dll L:\SDVX\kantai7\C310FWDLusb.dll
copy buildDir\chcusb\chcusb.dll L:\SDVX\kantai7\C310Ausb.dll
copy buildDir\chcfwdl\chcfwdl.dll W:\apm\BZ10\App\C310FWDLusb.dll
copy buildDir\chcusb\chcusb.dll W:\apm\BZ10\App\C310Ausb.dll
copy buildDir\chcfwdl\chcfwdl.dll dist\CXXXFwdlusb.dll
copy buildDir\chcusb\chcusb.dll dist\CXXXusb.dll
copy buildDir\chcusb330\chcusb330.dll dist\C330Ausb.dll
copy printerbot.ini dist\printerbot.ini
C:\msys64\usr\bin\bash --login -c "DIR=%cd:\=\\\\%; cd $(cygpath $DIR) && find ./dist -type f -name *.dll -exec strip -s {} \;"
