#!/bin/sh
# Run on Force: --check is read-only; --apply rolls back this plugin only.
# Copy this file, plugin_list.awk and the selected rollback manifest together.
set -eu
MODE=${1:---check}
case "$MODE" in --check|--apply) ;; *) echo 'Usage: rollback_force.sh [--check|--apply] [1.0.0|1.0.1|1.0.2]' >&2; exit 2;; esac
VERSION=${2:-1.0.0}
case "$VERSION" in
    1.0.0) BACKUP_NAME=1.0.0-before-1.0.1 ;;
    1.0.1) BACKUP_NAME=1.0.1-before-1.0.2 ;;
    1.0.2) BACKUP_NAME=1.0.2-before-2.0.0 ;;
    *) echo 'Supported rollback versions: 1.0.0, 1.0.1, 1.0.2' >&2; exit 2 ;;
esac
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# Test harness prefix isolates every device path and disables service control.
PREFIX=${DUB_FORCE_ROLLBACK_TEST_ROOT:-}
if [ -n "$PREFIX" ]; then
    case "$PREFIX" in /tmp/*) ;; *) echo 'Test root must be under /tmp' >&2; exit 2;; esac
fi
LIBRARY="$PREFIX/media/AKAI_SSD/Synths"
NAME='Dub Force - VST - Dub Force Siren'
TARGET="$LIBRARY/$NAME"
BACKUP="$PREFIX/media/AKAI_SSD/Dub-Force-Siren-backups/$BACKUP_NAME"
SETTINGS="$PREFIX/media/az01-internal/Settings/MPC/MPC.settings"
AWKFILE="$HERE/plugin_list.awk"
MANIFEST="$HERE/rollback-$VERSION.sha256"
[ -f "$AWKFILE" ] || AWKFILE="$HERE/../vendor/mpc-vst-plugins/tools/release/plugin_list.awk"
[ -f "$MANIFEST" ] || MANIFEST="$HERE/../resources/rollback-$VERSION.sha256"
[ -f "$AWKFILE" ] && [ -f "$MANIFEST" ] && [ -f "$SETTINGS" ] || { echo 'Missing rollback companion/settings file' >&2; exit 1; }
[ -d "$BACKUP" ] && [ ! -L "$BACKUP" ] && [ -d "$TARGET" ] && [ ! -L "$TARGET" ] || { echo 'Backup/current plugin folder missing or symbolic' >&2; exit 1; }
(cd "$BACKUP" && sha256sum -c "$MANIFEST" >/dev/null)
grep -q 'uid="44625372"' "$BACKUP/plugin-meta.xml"
grep -Fq "<version>$VERSION.0</version>" "$BACKUP/version.xml"
if [ -z "$PREFIX" ]; then
    [ "$(id -u)" = 0 ] || { echo 'Run as root on Force' >&2; exit 1; }
    case "$(uname -m)" in armv7*) ;; *) echo 'Expected ARMv7 Force' >&2; exit 1;; esac
    systemctl cat acvs >/dev/null
fi
WORK=$(mktemp -d "${PREFIX:-/tmp}/dub-force-rollback.XXXXXX")
trap 'rm -rf "$WORK"' EXIT
sed "s|%payload-path%|$LIBRARY|g" "$BACKUP/plugin-meta.xml" > "$WORK/entry.xml"
make_settings() {
    awk -v mode=add -v file="$TARGET/dub_force_siren.so" -v alt='' -v uid=44625372 \
        -v entryfile="$WORK/entry.xml" -f "$AWKFILE" "$SETTINGS" > "$WORK/settings.xml"
    python3 - "$SETTINGS" "$WORK/settings.xml" <<'PY'
import sys, xml.etree.ElementTree as E
before,after=[E.parse(p).getroot() for p in sys.argv[1:]]
def entries(root):return list(root.iter('PLUGIN'))
# ElementTree includes each entry's following indentation in tostring(). The
# installer can change that separator while preserving the entry itself.
def other(root):return sorted(E.tostring(p).strip() for p in entries(root) if p.get('uid')!='44625372')
assert other(before)==other(after),'Unrelated plugin registrations changed'
def fingerprint(e):
 if e.tag=='PLUGIN' and e.get('uid')=='44625372':return None
 return (e.tag,sorted(e.attrib.items()),(e.text or '').strip(),[v for c in e if (v:=fingerprint(c)) is not None])
assert fingerprint(before)==fingerprint(after),'Unrelated settings changed'
target=[p for p in entries(after) if p.get('uid')=='44625372']
assert len(target)==1 and target[0].get('name')=='Dub Force Siren','Target registration invalid'
PY
}
make_settings
printf 'PASS: complete %s backup verified; target-only registration edit verified.\n' "$VERSION"
if [ "$MODE" = --check ]; then echo 'No plugin/settings/service changes made.'; exit 0; fi
# Stop only after all verification. Save current project before --apply.
ctl() { if [ -z "$PREFIX" ]; then systemctl "$1" acvs; fi; }
ctl stop
RESTART=1
trap 'rm -rf "$WORK"; if [ "$RESTART" = 1 ]; then ctl start; fi' EXIT
if [ -z "$PREFIX" ]; then
    i=0;while pidof MPC >/dev/null && [ "$i" -lt 30 ]; do sleep 1;i=$((i+1));done
    ! pidof MPC >/dev/null || { echo 'MPC did not stop' >&2; exit 1; }
fi
# Re-read settings after MPC has flushed them. Preserve all unrelated settings.
make_settings
STAMP=$(date +%Y%m%d-%H%M%S)-$$
RECOVERY="$PREFIX/media/AKAI_SSD/Dub-Force-Siren-backups/pre-rollback-$STAMP"
mkdir "$RECOVERY"
cp -a "$TARGET" "$RECOVERY/plugin"
cp -a "$SETTINGS" "$RECOVERY/MPC.settings"
STAGE="$LIBRARY/.Dub Force Siren.rollback-$STAMP"
cp -a "$BACKUP" "$STAGE"
(cd "$STAGE" && sha256sum -c "$MANIFEST" >/dev/null)
# Every writable path is this plugin's path or a new backup. Never restore whole old settings.
mv "$TARGET" "$RECOVERY/replaced-plugin"
mv "$STAGE" "$TARGET"
cp -p "$SETTINGS" "$WORK/settings-permissions.xml"
cat "$WORK/settings.xml" > "$WORK/settings-permissions.xml"
mv "$WORK/settings-permissions.xml" "$SETTINGS"
sync
ctl start
RESTART=0
printf 'Restored %s. Previous current plugin/settings retained at %s\n' "$VERSION" "$RECOVERY"
