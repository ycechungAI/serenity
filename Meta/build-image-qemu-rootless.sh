#!/usr/bin/env bash

# Builds _disk_image without root privileges using genext2fs.
# build-root-filesystem.sh is run with a shimmed `chown` that records the requested
# ownership instead of applying it; the recorded ownership is then baked into the
# image through a genext2fs device table. Everything else is squashed to root (-U).

set -e

die() {
    echo "die: $*"
    exit 1
}

[ -z "$SERENITY_SOURCE_DIR" ] && die "SERENITY_SOURCE_DIR is not set"
command -v genext2fs >/dev/null || die "genext2fs is required for rootless image builds"

: "${DISK_SIZE_BYTES:?}" "${BYTES_PER_INODE:?}"

shim_dir=$(mktemp -d)
chown_log="$shim_dir/chown.log"
devtable="$shim_dir/devtable.txt"
trap 'rm -rf "$shim_dir" mnt' EXIT

# Record chown calls as "R|-<TAB>uid<TAB>gid<TAB>path".
cat > "$shim_dir/chown" <<'EOF'
#!/bin/sh
recursive=-
if [ "$1" = "-R" ]; then recursive=R; shift; fi
owner="$1"; shift
uid="${owner%%:*}"; gid="${owner#*:}"
for path in "$@"; do
    printf '%s\t%s\t%s\t%s\n' "$recursive" "$uid" "$gid" "$path" >> "$CHOWN_LOG"
done
EOF
# rsync's --chown cannot be honored without root; drop it (ownership defaults to root via -U).
cat > "$shim_dir/rsync" <<EOF
#!/bin/sh
for arg do
    shift
    case "\$arg" in --chown=*) continue ;; esac
    set -- "\$@" "\$arg"
done
exec $(command -v rsync) "\$@"
EOF
chmod +x "$shim_dir/chown" "$shim_dir/rsync"
: > "$chown_log"

rm -rf mnt
mkdir -p mnt
CHOWN_LOG="$chown_log" PATH="$shim_dir:$PATH" SERENITY_USE_GENEXT2FS=1 \
    "$SERENITY_SOURCE_DIR/Meta/build-root-filesystem.sh"

printf "generating ownership table... "
emit_entry() {
    local path="$1" uid="$2" gid="$3" rel type mode
    rel="/${path#mnt/}"
    rel="${rel%/}"
    [ -L "$path" ] && return 0
    case "$rel" in *[[:space:]]*) echo "warning: skipping ownership for '$rel'" >&2; return 0 ;; esac
    if [ -d "$path" ]; then type=d; elif [ -f "$path" ]; then type=f; else return 0; fi
    mode=$(stat -f '%Mp%Lp' "$path")
    printf '%s %s %s %s %s - - - - -\n' "$rel" "$type" "$mode" "$uid" "$gid" >> "$devtable"
}
: > "$devtable"
while IFS=$'\t' read -r recursive uid gid path; do
    [ "$uid" = 0 ] && [ "$gid" = 0 ] && [ "$recursive" = R ] && [ "$path" = "mnt/" ] && continue
    if [ "$recursive" = R ]; then
        while IFS= read -r p; do emit_entry "$p" "$uid" "$gid"; done < <(find "$path" ! -type l)
    else
        emit_entry "$path" "$uid" "$gid"
    fi
done < "$chown_log"
echo "done"

rm -f _disk_image
genext2fs -B 4096 -b $((DISK_SIZE_BYTES / 4096)) -i "${BYTES_PER_INODE}" -U -d mnt -D "$devtable" _disk_image \
    || die "try increasing image size (genext2fs -b)"
chmod 0666 _disk_image
