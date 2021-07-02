#!/bin/bash

INSTDIR="${XDG_DATA_HOME-$HOME/.local/share}/multiservermc"

if [ `getconf LONG_BIT` = "64" ]
then
    PACKAGE="msmc-stable-lin64.tar.gz"
else
    PACKAGE="msmc-stable-lin32.tar.gz"
fi

deploy() {
    mkdir -p $INSTDIR
    cd ${INSTDIR}

    wget --progress=dot:force "https://files.multimc.org/downloads/${PACKAGE}" 2>&1 | sed -u 's/.* \([0-9]\+%\)\ \+\([0-9.]\+.\) \(.*\)/\1\n# Downloading at \2\/s, ETA \3/' | zenity --progress --auto-close --auto-kill --title="Downloading MultiServerMC..."

    tar -xzf ${PACKAGE} --transform='s,MultiServerMC/,,'
    rm ${PACKAGE}
    chmod +x MultiServerMC
}

runmsmc() {
    cd ${INSTDIR}
    ./MultiServerMC "$@"
}

if [[ ! -f ${INSTDIR}/MultiServerMC ]]; then
    deploy
    runmsmc "$@"
else
    runmsmc "$@"
fi
