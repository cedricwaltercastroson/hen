@echo off
PATH = c:\pspsdk\bin;%cd%;
cd rebootex\systemctrl
make clean
make all
cd ..
make clean
make all
cd ..
make clean
make
pause