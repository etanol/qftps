#!/bin/sh

#
# Build and package a set of binaries depending on the platform you want to
# target.  Different flavours and debug binaries included alltogheter.
#

platform=${1-'linux'}
targets='all debug'
flavours='generic mmap'
cflags=''
ldflags=''
exe=''

version=`hg id -t | head -1`
if [ -z "$version" ] || [ "$version" = 'tip' ]
then
    version=`hg id -i | head -1`
    if [ -z "$version" ]
    then
        version='XXX'  # Just in case
    fi
fi

case $platform in
    linux) platform="linux_`uname -m`"
           flavours='generic mmap linux'
           ;;
    hase)  targets='hase dhase'
           flavours='generic mmap hase'
           exe='.exe'
           ;;
    mac)   cflags='-arch i386 -arch ppc -arch ppc64'
           ldflags='-Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk'
           ;;
    *) ;;
esac

id="${version}_$platform"

[ -d uftps-bin-$id  ] || mkdir uftps-bin-$id

for flav in $flavours
do
    if [ -n "$cflags" ] || [ -n "$ldflags" ]
    then
        make EXTRA_CFLAGS="$cflags" EXTRA_LDFLAGS="$ldflags" RETR=$flav $targets
    else
        make RETR=$flav $targets
    fi

    mv -f uftps$exe     uftps-bin-$id/uftps-$flav$exe
    mv -f uftps.dbg$exe uftps-bin-$id/uftps-$flav.dbg$exe
done

if [ $platform = hase ]
then
    zip -rm9 uftps-bin-$id.zip uftps-bin-$id
else
    tar cvf - uftps-bin-$id | gzip -vfc9 >uftps-bin-$id.tar.gz
    rm -rf uftps-bin-$id
fi

