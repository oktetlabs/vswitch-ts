# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved.

fail() {
    echo "$*" >&2
    exit 1
}

if test -z "${TE_BASE}" ; then
    if test -e dispatcher.sh ; then
        export TE_BASE="${RUNDIR}"
    elif test -e "${TS_TOPDIR}/dispatcher.sh" ; then
        export TE_BASE="${TS_TOPDIR}"
    elif test -e "${TS_TOPDIR}/../te/dispatcher.sh" ; then
        export TE_BASE="${TS_TOPDIR}/../te"
    else
        fail "Path to TE sources MUST be specified in TE_BASE"
    fi
fi

if test -z "${TE_BUILD}" ; then
    if test "${TS_TOPDIR}" = "${RUNDIR}" ; then
        TE_BUILD="${TS_TOPDIR}/build"
        mkdir -p build
    else
        TE_BUILD="${RUNDIR}"
    fi
    export TE_BUILD
fi

if test -z "${SF_TS_CONFDIR}" ; then
    SF_TS_CONFDIR="${TS_TOPDIR}"/../conf
    if test ! -d "${SF_TS_CONFDIR}" ; then
        SF_TS_CONFDIR="${TS_TOPDIR}"/../ts-conf
        if test ! -d "${SF_TS_CONFDIR}" ; then
            SF_TS_CONFDIR="${TS_TOPDIR}"/../ts_conf
            if test ! -d "${SF_TS_CONFDIR}" ; then
                fail "Path to ts-conf MUST be specified in SF_TS_CONFDIR"
            fi
        fi
    fi
    echo "Guessed SF_TS_CONFDIR=${SF_TS_CONFDIR}"
    export SF_TS_CONFDIR
fi

if test -z "${TE_TS_RIGSDIR}" ; then
    TE_TS_RIGSDIR="${TS_TOPDIR}"/../ts-rigs
    if [[ -d "${TE_TS_RIGSDIR}" ]] ; then
        TE_TS_RIGSDIR="$(realpath "${TE_TS_RIGSDIR}")"
        echo "Guessed TE_TS_RIGSDIR=${TE_TS_RIGSDIR}"
        export TE_TS_RIGSDIR
    else
        unset TE_TS_RIGSDIR
    fi
fi


