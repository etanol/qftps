#!/bin/sh

#
# Build and package a set of binaries depending on the platform you want to
# target.  Different flavours and debug binaries included alltogheter.
#

platform=${1-'linux'}
targets='all debug'
flavours='generic mmap'
exe=''
version=`hg id -t | head -1`
if [ -z "$version" ] || [ "$version" = 'tip' ]
then
    version=`hg id -i | head -1`
fi

case $platform in
    linux) platform="linux_`uname -m`"
           flavours='generic mmap linux' ;;
    hase)  targets='hase dhase'
           flavours='generic mmap hase'
           exe='.exe'                    ;;
    *) ;;
esac

id="${version}_$platform"

test -d uftps-$id || mkdir uftps-$id

for flavour in $flavours
do
    make RETR=$flavour $targets
    mv -f uftps$exe      uftps-$id/uftps-$flavour$exe
    mv -f uftps.dbg$exe  uftps-$id/uftps-$flavour.dbg$exe
done

if [ $platform = hase ]
then
    zip -rm9 uftps-$id.zip uftps-$id
else
    tar cvf - uftps-$id | gzip -vfc9 >uftps-$id.tar.gz
    rm -rf uftps-$id
fi

