#!/bin/sh

SCRIPT_PATH="$( cd "$( dirname "$0" )" && pwd )"
NAME=fructose-1.4.0
FILENAME=${NAME}.tar.gz
OS=$(uname -s)

if [ "x${OS}y" = "xLinuxy" ]; then
	SHA512_CMD=sha512sum
else
	SHA512_CMD=shasum
fi

echo "$SHA512_CMD"
pwd

cd ${SCRIPT_PATH}

if ! ${SHA512_CMD} -c ${NAME}.sha512; then
	wget https://downloads.sourceforge.net/project/fructose/fructose/${NAME}/${FILENAME}
fi

rm -rf "${SCRIPT_PATH}/fructose"
tar xf ${FILENAME}
mv ${NAME}/fructose/include/fructose/ ${SCRIPT_PATH}/
rm -rf ${NAME}/
