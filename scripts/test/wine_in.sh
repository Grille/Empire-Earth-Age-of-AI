# The Linux side of scripts/test/wine.sh, run as root in the WSL distro (its working directory: the repository).
# usage: wine_in.sh setup                       packages (WineHQ's stable Wine with i386, Mesa, Xvfb, screenshots)
#        wine_in.sh prefix <reg file>           the prefix /root/eeprefix: the game folder copied into C:\ee, the registry
#        wine_in.sh run <seconds> "<ini keys>"  a test game in an X server of its own (Xvfb :77 - nothing on the screen)
# The run: local/work/wine_game's ageofai.dll copied over, the test ini (tests/config/autotest.ini, the keys given
# replacing the file's), the game started with -loadgame "aitest.ees" through a batch file (the game's parser needs the
# quotes); a screenshot every SHOT seconds (default 20) into local/work/wine_shots/, the log to local/work/wine_log.csv.
# Xvfb draws in software (Mesa's llvmpipe): Wine's Direct3D 11 runs on its OpenGL 4.5. The PC's graphics card is
# there only on WSLg's own display (GALLIUM_DRIVER=d3d12 LD_LIBRARY_PATH=/usr/lib/wsl/lib) - a window on the screen.
set -u
REPO="$PWD"
export WINEPREFIX=/root/eeprefix WINEDEBUG="${WINEDEBUG:--all}" WINEDLLOVERRIDES="${WINEDLLOVERRIDES:-mscoree,mshtml=}"
G="$WINEPREFIX/drive_c/ee"
case "${1:-}" in
setup)
    set -e
    export DEBIAN_FRONTEND=noninteractive
    dpkg --add-architecture i386
    apt-get update -q
    apt-get install -y -q wget gnupg2 ca-certificates
    mkdir -pm755 /etc/apt/keyrings
    wget -qO /etc/apt/keyrings/winehq-archive.key https://dl.winehq.org/wine-builds/winehq.key
    wget -qNP /etc/apt/sources.list.d/ "https://dl.winehq.org/wine-builds/ubuntu/dists/$(. /etc/os-release; echo "$VERSION_CODENAME")/winehq-$(. /etc/os-release; echo "$VERSION_CODENAME").sources"
    apt-get update -q
    apt-get install -y -q --install-recommends winehq-stable
    # the game is 32-bit: Wine's Direct3D needs the i386 OpenGL / EGL libraries (without them D3D11CreateDevice fails
    # with 887a0004 and the own renderer has no device)
    apt-get install -y -q mesa-utils vulkan-tools mesa-vulkan-drivers mesa-vulkan-drivers:i386 libgl1-mesa-dri:i386 \
        libgl1:i386 libegl1:i386 libglx-mesa0:i386 libvulkan1:i386 xvfb x11-apps imagemagick
    wine --version ;;
prefix)
    [ -d "$WINEPREFIX" ] || { wineboot -i >/dev/null 2>&1; wineserver -w; }
    rm -rf "$G"; cp -r "$REPO/local/work/wine_game" "$G"
    wine regedit /S "$2"; wineserver -w
    echo "prefix ready: $G" ;;
run)
    SECS=${2:-120}; EXTRA=${3:-}; SHOT=${SHOT:-20}
    [ -f "$G/Empire Earth.exe" ] || { echo "no game in the prefix: sh scripts/test/wine.sh prefix"; exit 1; }
    cp "$REPO/local/work/wine_game/ageofai.dll" "$G/ageofai.dll"
    cp "$REPO/tests/config/eesettings.ini" "$G/test_settings.ini"
    tr -d '\r' < "$REPO/tests/config/autotest.ini" | awk -v extra="$EXTRA" '
        BEGIN { n = split(extra, kv, ","); for (i = 1; i <= n; i++) { k = kv[i]; sub(/=.*/, "", k); gsub(/ /, "", k); if (k != "") drop[k] = 1 } }
        { k = $0; sub(/=.*/, "", k); gsub(/ /, "", k); if (!(k in drop)) print }
        END { for (i = 1; i <= n; i++) if (kv[i] != "") print kv[i] }' > "$G/ageofai/ageofai_autotest.ini"
    rm -f "$G/ageofai/ageofai_log.csv"
    export EEAIMOD_SETTINGS='C:\ee\test_settings.ini'
    SH="$REPO/local/work/wine_shots"; rm -rf "$SH"; mkdir -p "$SH"
    Xvfb :77 -screen 0 1920x1080x24 >/dev/null 2>&1 & XP=$!
    sleep 1; export DISPLAY=:77
    cd "$G"
    echo '"C:\ee\Empire Earth.exe" -loadgame "aitest.ees"' > run.bat
    wine cmd /c 'C:\ee\run.bat' > /root/wine_out.txt 2>&1 & GP=$!
    t=0
    while kill -0 $GP 2>/dev/null && [ $t -lt "$SECS" ]; do
        sleep 1; t=$((t + 1))
        [ $((t % SHOT)) -eq 0 ] && xwd -root -silent | convert xwd:- -resize 50% "$SH/t$(printf %04d $t).png" 2>/dev/null
    done
    if kill -0 $GP 2>/dev/null; then echo "timeout at $t s"; else wait $GP; echo "exit $? at $t s"; fi
    wineserver -k 2>/dev/null; kill $XP 2>/dev/null
    cp "$G/ageofai/ageofai_log.csv" "$REPO/local/work/wine_log.csv" 2>/dev/null && echo "log: local/work/wine_log.csv"
    grep -v 'X connection\|^XIO\|after \|ALSA\|DRI3' /root/wine_out.txt | tail -5 ;;
*)
    echo "usage: wine_in.sh setup | prefix <reg file> | run <seconds> \"<ini keys>\""; exit 1 ;;
esac
