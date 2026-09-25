#!/bin/sh
set -eu

apk=${1:?usage: emulator-smoke.sh APK SCREENSHOT_DIR}
screens=${2:?usage: emulator-smoke.sh APK SCREENSHOT_DIR}

package=org.tokipona.drills
component="$package/android.app.NativeActivity"

mkdir -p "$screens"

adb install -r "$apk"
adb logcat -c

adb shell am start -W -n "$component" > /tmp/toki-pona-start.txt
grep -Fq 'Status: ok' /tmp/toki-pona-start.txt

tries=0
while [ "$tries" -lt 20 ]
do
    if adb shell pidof "$package" | tr -d '\r' | grep -q .
    then
        break
    fi
    tries=$((tries + 1))
    sleep 1
done

test -n "$(adb shell pidof "$package" | tr -d '\r')"

tries=0
while [ "$tries" -lt 20 ]
do
    if adb logcat -d | grep -Fq 'TokiPonaNative: loaded 56 exercises; daily session=12'
    then
        break
    fi
    tries=$((tries + 1))
    sleep 1
done

adb logcat -d | grep -Fq 'TokiPonaNative: loaded 56 exercises; daily session=12'

capture() {
    name=$1
    adb shell screencap -p "/sdcard/$name.png"
    adb pull "/sdcard/$name.png" "$screens/$name.png" >/dev/null
}

capture before

screen_size=$(adb shell wm size | tr -d '\r' | sed -n 's/.*: \([0-9][0-9]*\)x\([0-9][0-9]*\).*/\1 \2/p' | tail -n 1)
set -- $screen_size
screen_width=$1
screen_height=$2
center_x=$((screen_width / 2))

# The first answer card begins at native content y=202. The first question's
# correct answer is A, so one tap near the upper half of the first card should
# produce an exact log receipt as well as a visible feedback state.
adb shell input tap "$center_x" 340
sleep 1

adb logcat -d | grep -Fq 'TokiPonaNative: answered core-01 choice=A correct=true'
capture answered
if cmp -s "$screens/before.png" "$screens/answered.png"
then
    echo 'native screen did not change after answering' >&2
    exit 1
fi

# The Next button occupies the bottom 54 px of the app content. Try the two
# likely centers around the system navigation inset; stop as soon as the native
# state machine records question 2.
for y in     $((screen_height - 115))     $((screen_height - 145))     $((screen_height - 175))
do
    adb shell input tap "$center_x" "$y"
    sleep 1
    if adb logcat -d | grep -Fq 'TokiPonaNative: advanced to question 2/12'
    then
        break
    fi
done

adb logcat -d | grep -Fq 'TokiPonaNative: advanced to question 2/12'
capture next

if cmp -s "$screens/answered.png" "$screens/next.png"
then
    echo 'native screen did not change after advancing' >&2
    exit 1
fi

test -n "$(adb shell pidof "$package" | tr -d '\r')"

if adb logcat -d | grep -E 'FATAL EXCEPTION|Fatal signal|AndroidRuntime.*FATAL|UnsatisfiedLinkError'
then
    echo 'fatal Android runtime evidence found' >&2
    exit 1
fi

echo 'PASS NativeActivity launched, answered core-01, and advanced to question 2'
