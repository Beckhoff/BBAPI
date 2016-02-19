#!/bin/bash

show_date() {
	echo -e "\f`date +"%d.%m.%Y\n%H:%M:%S"`" > /dev/cx_display;
}

show_eth() {
	echo -e "\feth$1:\n`ifconfig eth$1 | grep "inet " | awk -F'[: ]+' '{ print $4 }'`" > /dev/cx_display;
}

show_load() {
	echo -e "\fload:\n`cat /proc/loadavg | awk '{print $1,$2,$3}'`" > /dev/cx_display
}

show_mem() {
	echo -e "\fmem: used free\n`free -h | awk '{if (NR==2) {print $2,$3,$4}}'`" > /dev/cx_display
}

page=0
num_pages=5

inc_page() {
	page=$(($(($page + 1)) % $num_pages))
}

dec_page() {
	page=$(($(($num_pages + $page - 1)) % $num_pages))
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

update_page() {
	case $(($page % $num_pages)) in
	0)	show_date;;
	1)	show_eth 0;;
	2)	show_eth 1;;
	3)	show_load;;
	4)	show_mem;;
	esac
}

while true
do
	update_page
	stdbuf -oL hexdump /dev/input/js0 |
		while IFS= read -r line
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
