#!/bin/bash

JOYSTICK=${1:-/dev/input/js0}
DISPLAY=/dev/cx_display
PIPE=test.pipe

page=0
my_pages=( show_date "show_eth 0" "show_eth 1" show_load show_mem)

show_date() {
	echo -e "\f`date +"%d.%m.%Y\n%H:%M:%S"`" > ${DISPLAY}
}

show_eth() {
	echo -e "\feth$1:\n`ifconfig eth$1 | grep "inet " | awk -F'[: ]+' '{ print $4 }'`" > ${DISPLAY}
}

show_load() {
	echo -e "\fload:\n`cat /proc/loadavg | awk '{print $1,$2,$3}'`" > ${DISPLAY}
}

show_mem() {
	echo -e "\fmem: used free\n`free -h | awk '{if (NR==2) {print $2,$3,$4}}'`" > ${DISPLAY}
}

inc_page() {
	page=$(($(($page + 1)) % ${#my_pages[@]}))
}

dec_page() {
	page=$(($((${#my_pages[@]} + $page - 1)) % ${#my_pages[@]}))
}

update_page() {
	${my_pages[$(($page % ${#my_pages[@]}))]}
}

decode_value() {
	case $1 in
	"8001") echo $2;;
	"7fff") echo $3;;
	*) echo "UNKOWN $1";;
	esac
}

decode_key() {
	key=`echo $1 | awk '{printf $9}'`
	value=`echo $1 | awk '{printf $8}'`

	case $key in
	"0102") decode_value "$value" "DOWN" "UP";;
	"0002") decode_value "$value" "LEFT" "RIGHT";;
	"0001") echo "ENTER";;
	esac
}

setup_event_pipe() {
	if [[ ! -p ${PIPE} ]]; then
		mkfifo ${PIPE}
	fi

	stdbuf -oL hexdump ${JOYSTICK}> ${PIPE} &
	PID=$!
	trap "{ rm ${PIPE}; kill $PID; exit 255; }" EXIT
}

# main() starts here...
setup_event_pipe
while true
do
	update_page
	while read -rs -t 0.5 line < ${PIPE}
	do
		case $(decode_key "${line}") in
		"UP")    printf "Up\n";;
		"DOWN")  printf "Down\n";;
		"RIGHT") inc_page; printf "Right\n";;
		"LEFT")  dec_page; printf "Left\n";;
		"ENTER") printf "Enter\n";;
		esac
		update_page
	done
done
