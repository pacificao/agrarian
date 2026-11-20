#!/bin/sh
# Read Qt's installed qmake link metadata without evaluating it as shell code.
set -eu

libdir=${1:?Qt library directory required}
plugindir=${2:?Qt plugin directory required}
prefix=${3:-$(dirname -- "$libdir")}

set -- "$plugindir/platforms/libqxcb.prl" "$libdir/libQt6Widgets.prl" \
    "$libdir/libQt6Network.prl" "$libdir/libQt6Gui.prl" "$libdir/libQt6Core.prl"

for plugin in platforms/libqminimal.prl tls/libqopensslbackend.prl; do
    if [ -r "$plugindir/$plugin" ]; then
        set -- "$@" "$plugindir/$plugin"
    fi
done

awk -v libdir="$libdir" -v plugindir="$plugindir" -v prefix="$prefix" '
    FNR == 1 { files++ }
    $1 == "QMAKE_PRL_LIBS" && $2 == "=" {
        records++
        for (i = 3; i <= NF; i++) {
            flag = $i
            if (index(flag, "$$[QT_INSTALL_LIBS]/") == 1)
                flag = libdir substr(flag, length("$$[QT_INSTALL_LIBS]") + 1)
            else if (index(flag, "$$[QT_INSTALL_PLUGINS]/") == 1)
                flag = plugindir substr(flag, length("$$[QT_INSTALL_PLUGINS]") + 1)
            else if (index(flag, "$$[QT_INSTALL_PREFIX]/") == 1)
                flag = prefix substr(flag, length("$$[QT_INSTALL_PREFIX]") + 1)
            if (flag ~ /[$\047"\\]/) {
                print "Unsupported Qt link metadata in " FILENAME ": " flag > "/dev/stderr"
                bad = 1
            }
            # Qt repeats the same resource initializer in several modules.
            # Unlike an archive, linking an object twice defines symbols twice.
            if (flag ~ /[.]o$/ && seen_objects[flag]++) continue
            flags = flags " " flag
        }
    }
    END {
        if (bad || files != ARGC - 1 || files != records) exit 1
        print substr(flags, 2)
    }' "$@"
