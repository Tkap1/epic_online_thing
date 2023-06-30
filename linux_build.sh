#!/bin/sh
cc="clang++"
warn="-Werror -Wall -Wno-char-subscripts -Wno-unused-function -Wno-switch -Wno-sign-compare"
if [ $cc = "g++" ] ; then
	diag="-fno-diagnostics-show-caret"
	warn="$warn -Wno-write-strings -Wno-class-memaccess"
else
	diag="-fno-caret-diagnostics"
	warn="$warn -Wno-writable-strings"
fi

serverdef="-Dm_server -Dm_app"
clientdef="-Dm_client -Dm_app"
opt_debug="-std=c++17 -Isrc/ -g -Dm_debug"
opt_release="-std=c++17 -O2 -DNDEBUG"
libs="-lm -lGL -lX11 -lenet"

compile_client=0
compile_server=0
debug=1

if [ $# -gt 0 ] ; then
	case $1 in
		"tags" )
			ctags src/*.cpp src/*.h
			;;
		"client" )
			compile_client=1
			;;
		"server" )
			compile_server=1
			;;
		"clean" )
			rm -f client server client.so server.so
			;;
		"release" )
			compile_client=1
			compile_server=1
			debug=0
			;;
	esac
else
	# compile both client and server by defaultif no other args given
	compile_client=1
	compile_server=1
fi

if [ $debug = 1 ] ; then
	opt=$opt_debug
else
	opt=$opt_release
fi

if [ $compile_client = 1 ] ; then
	$cc -shared -fPIC $warn $diag $opt $clientdef src/client.cpp -o client.so $libs &
	$cc -fPIC $warn $diag $opt $clientdef src/linux_platform_client.cpp src/client.cpp -o client $libs &
fi

if [ $compile_server = 1 ] ; then
	$cc -shared -fPIC $warn $diag $opt $serverdef src/server.cpp -o server.so $libs &
	$cc -fPIC $warn $diag $opt $serverdef src/linux_platform_server.cpp src/server.cpp -o server $libs &
fi

if [ $compile_server = 1 ] || [ $compile_client = 1 ] ; then
	wait $(jobs -p)
fi
