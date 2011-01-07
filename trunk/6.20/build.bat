@echo off
PATH = c:\pspsdk\bin;%cd%;
cd rebootex
make clean
make all
cd ..
make clean
make
pause