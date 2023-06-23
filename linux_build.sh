#!/bin/sh
cc="clang++"
if [ $cc = "g++" ] ; then
	diag="-fno-diagnostics-show-caret"
else
	diag="-fno-caret-diagnostics"
fi
warn="-Werror -Wall -Wno-char-subscripts -Wno-unused-function"
opt="-std=c++17 -Isrc/ -g -Dm_app"
opt_release="-std=c++17 -O2 -DNDEBUG -Dm_app"
libs="-lm -lGL -lX11 -lenet"

if [ $# -gt 0 ] ; then
	case $1 in
		"tags" )
			ctags src/*.cpp src/*.h
			;;
		"client" )
			$cc $warn $diag $opt src/linux_platform.cpp src/client.cpp -o client $libs
			;;
		"server" )
			$cc $warn $diag $opt src/server.cpp -o server $libs
			;;
		"clean" )
			rm -f client server
			;;
		"release" )
			$cc $warn $diag $opt_release src/linux_platform.cpp src/client.cpp -o client $libs &
			$cc $warn $diag $opt_release src/server.cpp -o server $libs &
			wait $(jobs -p)
			;;
	esac
else
	$cc $warn $diag $opt src/linux_platform.cpp src/client.cpp -o client $libs &
	$cc $warn $diag $opt src/server.cpp -o server $libs &
	wait $(jobs -p)
fi

