#!/bin/bash
#
REV=unknown
if [ "$(uname)" == 'Darwin' ]; then
  gcc -fPIC -shared -Wall -Wno-main -Wno-sign-compare -Wshadow -Wcast-align -Wpointer-arith -Wswitch -Wredundant-decls -Wreturn-type -Wshadow -Wunused -Wl,-install_name,dx2lib_m.so.2 -O3 -D__MAKE_LIB__ dx2lib_intuitive.cpp dx2lib.cpp -o dx2lib.so.2.9
else
  gcc -fPIC -shared -Wall -Wno-main -Wno-sign-compare -Wshadow -Wcast-align -Wpointer-arith -Wswitch -Wredundant-decls -Wreturn-type -Wshadow -Wunused -Wl,-soname=dx2lib_m.so.2 -O3 -D__MAKE_LIB__ dx2lib_intuitive.cpp dx2lib.cpp -o dx2lib.so.2.9
fi

if [ $? -ne 0 ]; then
    echo "Oops..."
    exit 1
fi
rm -f ../SampleCode/Python/dx2lib.so.2.9
rm -f ../SampleCode/Ruby/dx2lib.so.2.9
rm -f ../SampleCode/Python/dx2lib.py
cp dx2lib.so.2.9 ../SampleCode/Python
cp dx2lib.so.2.9 ../SampleCode/Ruby
cp dx2lib.py ../SampleCode/Python

gcc -fPIC -shared -Wall -Wno-main -Wno-sign-compare -Wshadow -Wcast-align -Wpointer-arith -Wswitch -Wredundant-decls -Wreturn-type -Wshadow -Wunused -o libdx2lib.a -O3 -D__MAKE_LIB__ dx2lib_intuitive.cpp dx2lib.cpp
if [ $? -ne 0 ]; then
    echo "Oops..."
    exit 1
fi
rm -f ../SampleCode/C/libdx2lib.a
rm -f ../SampleCode/C/dx2lib.h
rm ../SampleCode/C/dx2memmap.h
cp libdx2lib.a ../SampleCode/C
cp dx2lib.h ../SampleCode/C
cp dx2memmap.h ../SampleCode/C
