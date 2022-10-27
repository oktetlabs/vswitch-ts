#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved.
#
# Helper script to run Test Environment for the Test Suite
#
# Author: Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
#

RUNDIR="$(pwd -P)"
export TS_TOPDIR="$(cd "$(dirname "$(which "$0")")"/.. ; pwd -P)"

source "${TE_BASE}/scripts/lib"
source "${TE_BASE}/scripts/lib.grab_cfg"

if [[ -e "${TE_TS_RIGSDIR}/scripts/lib/grab_cfg_handlers" ]] ; then
    source "${TE_TS_RIGSDIR}/scripts/lib/grab_cfg_handlers"
fi

cleanup() {
    call_if_defined grab_cfg_release
}
trap "cleanup" EXIT

run_fail() {
    echo $* >&2
    exit 1
}

if test -z "${TE_BASE}" ; then
    if test -e dispatcher.sh ; then
        export TE_BASE="${RUNDIR}"
    elif test -e "${TS_TOPDIR}/dispatcher.sh" ; then
        export TE_BASE="${TS_TOPDIR}"
    else
        run_fail "Path to TE sources MUST be specified in TE_BASE"
    fi
fi


usage() {
cat <<EOF
USAGE: run.sh [run.sh options] [dispatcher.sh options]
Options:
  --cfg=<CFG>               Configuration to be used

EOF

    call_if_defined grab_cfg_print_help

    cat <<EOF

  --reuse-pco               Do not restart RPC servers and re-init EAL in each test
                            (it makes testing significantly faster)

EOF
"${TE_BASE}"/dispatcher.sh --help
exit 1
}

function process_cfg() {
    local cfg="$1" ; shift
    local run_conf
    local cfg_mod

    call_if_defined grab_cfg_process "${cfg}" || exit 1
    if test "${cfg}" != "${cfg%-p[0-9]}" ; then
        run_conf="${cfg%-p[0-9]}"
        cfg_mod="${cfg#${run_conf}-}"
    else
        run_conf="${cfg}"
        cfg_mod=
    fi
    RUN_OPTS="${RUN_OPTS} --opts=run/\"${run_conf}\""
    test -z "${cfg_mod}" ||
        RUN_OPTS="${RUN_OPTS} --script=scripts/only-${cfg_mod}"
    # Add test suite default options after configuration specifics
    RUN_OPTS="${RUN_OPTS} --opts=opts.ts"
}

CFG=

for opt ; do
    if call_if_defined grab_cfg_check_opt "${opt}" ; then
        shift 1
        continue
    fi

    case "${opt}" in
        --help) usage ;;
        --cfg=*)
            test -z "${CFG}" ||
                run_fail "Configuration is specified twice: ${CFG} vs ${opt#--cfg=}"
            CFG="${opt#--cfg=}"
            ;;
        --reuse-pco)
            export TE_ENV_REUSE_PCO=yes
            ;;
        *)  RUN_OPTS="${RUN_OPTS} \"${opt}\"" ;;
    esac
    shift 1
done

if test -n "${CFG}" ; then
    process_cfg ${CFG}
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
                run_fail "Path to shared Xilinx ts-conf MUST be specified in SF_TS_CONFDIR"
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

MY_OPTS=
MY_OPTS="${MY_OPTS} --conf-dirs=\"${TS_TOPDIR}/conf:${TE_TS_RIGSDIR}${TE_TS_RIGSDIR:+:}${SF_TS_CONFDIR}\""

test -e "${TS_TOPDIR}/trc/top.xml" &&
    MY_OPTS="${MY_OPTS} --trc-db=\"${TS_TOPDIR}\"/trc/top.xml"
MY_OPTS="${MY_OPTS} --trc-comparison=normalised"
MY_OPTS="${MY_OPTS} --trc-html=trc-brief.html"
MY_OPTS="${MY_OPTS} --trc-no-expected"
MY_OPTS="${MY_OPTS} --trc-no-total --trc-no-unspec"
MY_OPTS="${MY_OPTS} --trc-keep-artifacts"
MY_OPTS="${MY_OPTS} --trc-key2html=${TE_TS_RIGSDIR}/trc.key2html"

# Add to RUN_OPTS since it specified in user environment and should
# override everything else
test "$TE_NOBUILD" = "yes" &&
    RUN_OPTS="${RUN_OPTS} --no-builder --tester-nobuild"
test -z "${TE_BUILDER_CONF}" ||
    RUN_OPTS="${RUN_OPTS} --conf-builder=\"${TE_BUILDER_CONF}\""
test -z "${TE_TESTER_CONF}" ||
    RUN_OPTS="${RUN_OPTS} --conf-tester=\"${TE_TESTER_CONF}\""

test -z "${TE_TS_SRC}" -a -d "${TS_TOPDIR}/vswitch-ts" &&
    export TE_TS_SRC="${TS_TOPDIR}/vswitch-ts"

# Make sure that old TRC report is not kept
rm -f trc-brief.html

eval "${TE_BASE}/dispatcher.sh ${MY_OPTS} ${RUN_OPTS}"
RESULT=$?

if test ${RESULT} -ne 0 ; then
    echo FAIL
    echo ""
fi

echo -ne "\a"
exit ${RESULT}
