#!/bin/bash

role=0 # 0 - зритель; 1 - первый игрок; 2 - второй игрок

P1=/tmp/pipe1 # FIFO-файлы первого и второго игрока
P2=/tmp/pipe2

LOCK=/var/lock/.prod.ex.lock

pl1=/tmp/pl1 # Временные файлы каждого игрока, в них каждый игрок будет писать свои позиции (нужен, так как у кода внутри блокировки и остального разные области видимости)
pl2=/tmp/pl2 # Каждый игрок пишет только в свой файл и читает только свой (все общение через FIFO, а не общие файлы)


gameFinished=0 
invalidInput=0  

cellWidth=10  # Отрисовочные характеристики
positions=(- - - - - - - - -)  # массив позиций
lineSegment="---------"
line="  ""$lineSegment""|""$lineSegment""|""$lineSegment"
pink="\033[35m"
cyan="\033[36m"
blue="\033[34m"
green="\033[32m"
reset="\033[0m"
player1Str=$green"Player One"$reset
player2Str=$blue"Player Two"$reset


# TODO Убрать moveNow, лишние функции, добавить зрителя, выход, удаление файлов по прерываннию

# Проверяем есть ли первый игрок
function firstPlayerCheck() {
    local  __resultvar=$1
    local  pc=$([[ -p /tmp/pipe1 ]] || echo 'NO') 
    eval $__resultvar="'$pc'"
}

# Проверяем есть ли второй игрок
function secondPlayerCheck() {
    local  __resultvar=$1
    local  pc=$([[ -p /tmp/pipe2 ]] || echo 'NO') 
    eval $__resultvar="'$pc'"
}

# Определяем роль игрока
function determineRole() {
	firstPlayerCheck result
	if [ "$result" == "NO" ]; then
		echo "You are first player!"
		role=1
	else
		secondPlayerCheck result
		if [ "$result" == "NO" ]; then
			echo "You are second player!"
			role=2
		else
			echo "You are spectator!"
		fi
	fi
}

# Ожидаем первого игрока
function waitPlayer1() {
	echo 'Waiting opponent...'
	firstPlayerCheck result
	while [ "$result" == "NO" ];
	do 
		sleep 2
		firstPlayerCheck result
	done
}

# Ожидаем второго игрока
function waitPlayer2() {
	echo 'Waiting opponent...'
	secondPlayerCheck result
	while [ "$result" == "NO" ];
	do 
		sleep 2
		secondPlayerCheck result
	done
}

# Удаляем FIFO и временный файл
function deletePlayer() {
	if [ $role -eq 1 ]; then
			rm $P1
		else
			if [ $role -eq 2 ]; then
					rm $P2
			fi
	fi
}

# Записываем позиции 1 игрока в временный файл
function printPl1Pos(){
	pos=$(printf "%s " "${positions[@]}")
	$(echo $pos > $pl1)
}

# Записываем позиции 2 игрока в временный файл
function printPl2Pos(){
	pos=$(printf "%s " "${positions[@]}")
	$(echo $pos > $pl2)
}


# Считываем сообщение из FIFO первого игрока
function readMsgP1() {
	(
		flock -w 0.03 -n 200
		read MSG < $P1
	
		positions=( $MSG )
		drawBoard positions
		
		printPl1Pos
		
		positions_str=$(printf "%s" "${positions[@]}")
		checkEndGame $positions_str
		
		
		if [ "$gameFinished" -eq 1 ]; then
			echo "END" > $pl1
		fi

	) 200>$LOCK
}

# Считываем сообщение из FIFO второго игрока
function readMsgP2() {
	(
		flock -w 0.03 -n 200
		read MSG < $P2

		positions=( $MSG )
		drawBoard positions
		
		printPl2Pos
		
		positions_str=$(printf "%s" "${positions[@]}")
		checkEndGame $positions_str
		
		
		if [ "$gameFinished" -eq 1 ]; then
			echo "END" > $pl2
		fi
		
	) 200>$LOCK
}

# Проверка на победителя/ничью
function checkEndGame {
	rows=${1:0:3}" "${1:3:3}" "${1:6:8}
	cols=${1:0:1}${1:3:1}${1:6:1}" "${1:1:1}${1:4:1}${1:7:1}" "${1:2:1}${1:5:1}${1:8:1}
	diagonals=${1:0:1}${1:4:1}${1:8:1}" "${1:2:1}${1:4:1}${1:6:1}
	if [[ $rows =~ [X]{3,} || $cols =~ [X]{3,} || $diagonals =~ [X]{3,} ]]; then
		echo -e $player1Str" wins! \n"
		gameFinished=1
		return
	fi
	if [[ $rows =~ [O]{3,} || $cols =~ [O]{3,} || $diagonals =~ [O]{3,} ]]; then
		echo -e $player2Str" wins! \n"
		gameFinished=1
		return
	fi
	if [[ ! $positions_str =~ [-] ]]; then 
		echo -e "End with a "$pink"draw"$reset"\n"
		gameFinished=1
	fi
}

# Отрисовка поля
function drawBoard {
	clear

	name=$1[@]
	argPositions=("${!name}")
	echo -e "\nQ W E       _|_|_\n A S D   →   | | \nZ X C       ‾|‾|‾\n\n"

	for (( row_id=1; row_id<=3; row_id++ ));do
		row="  "
		emptyRow="  "
		for (( col_id=1; col_id<$(($cellWidth*3)); col_id++ ));do
			if [[ $(( $col_id%$cellWidth )) == 0 ]]; then
				row=$row"|"
				emptyRow=$emptyRow"|"
			else
				if [[ $(( $col_id%5 )) == 0 ]]; then

					x=$(($row_id-1))
					y=$((($col_id - 5) / 10))

					if [[ $x == 0 ]]; then
						what=${argPositions[$y]}
					elif [[ $x == 1 ]]; then
						what=${argPositions[(($y+3))]}
					else
						what=${argPositions[(($y+6))]}
					fi

					if [[ $what == "-" ]]; then what=" "; fi

					if [[ $what == "X" ]] ; then  
						row=$row$green$what$reset
					else
						row=$row$blue$what$reset
					fi

					emptyRow=$emptyRow" "  
				else
					row=$row" "
					emptyRow=$emptyRow" "
				fi
			fi
		done
		echo -e "$emptyRow""\n""$row""\n""$emptyRow"
		if [[ $row_id != 3 ]]; then
			echo -e "$line"
		fi
	done
	echo -e "\n"
}


function firstPlayerGame() {
	`[[ -f /tmp/pl1 ]] || touch /tmp/pl1`
	mkfifo $P1
	printPl1Pos
	
	waitPlayer2
	
	drawBoard positions
			
	while true; do
		(
			flock -w 0.03 -n 200
			read -d'' -s -n1 input  # Считываем новый символ
					
			index=20  # Если не сходил правильно - сам виноват, пропуск хода
			case $input in
				  q) index=0;;
				  a) index=3;;
				  z) index=6;;
				  w) index=1;;
				  s) index=4;;
				  x) index=7;;
				  e) index=2;;
				  d) index=5;;
				  c) index=8;;
			esac
			positions=( $(cat $pl1) )
			
			if [ "${positions["$index"]}" == "-" ]; then
			  positions["$index"]="X"
			fi
			
			drawBoard positions
			printPl1Pos
			
			pos=$(printf "%s " "${positions[@]}")
			echo "$pos" > $P2 # отправляем новый массив другому игроку
			
			positions_str=$(printf "%s" "${positions[@]}")
			checkEndGame $positions_str
			
			if [ "$gameFinished" -eq 1 ]; then
				echo "END" > $pl1
			fi
			
			
		) 200>$LOCK
		
		p=$(cat $pl1)
		if [ "$p" == "END" ]; then
			break
		fi
		
		echo "wait Player2"	
		readMsgP1
		
		p=$(cat $pl1)
		if [ "$p" == "END" ]; then
			break
		fi
		
	done
	deletePlayer
}


function secondPlayerGame() {
	`[[ -f /tmp/pl1 ]] || touch /tmp/pl1`
	mkfifo $P2
	printPl2Pos
			
	waitPlayer1
	
	drawBoard positions
	
	while true; do
		echo "waitPlayer1"
		readMsgP2
		
		p=$(cat $pl2)
		if [ "$p" == "END" ]; then
			break
		fi
		
		(
			flock -w 0.03 -n 200
			read -d'' -s -n1 input 
					
			index=20
			case $input in
				  q) index=0;;
				  a) index=3;;
				  z) index=6;;
				  w) index=1;;
				  s) index=4;;
				  x) index=7;;
				  e) index=2;;
				  d) index=5;;
				  c) index=8;;
			esac
			positions=( $(cat $pl2) )
			
			if [ "${positions["$index"]}" == "-" ]; then
			  positions["$index"]="O"
			fi
		
			drawBoard positions
			printPl2Pos

			pos=$(printf "%s " "${positions[@]}")
			echo "$pos" > $P1
			
			positions_str=$(printf "%s" "${positions[@]}")
			checkEndGame $positions_str
			
			if [ "$gameFinished" -eq 1 ]; then
				echo "END" > $pl2
			fi
			
		) 200>$LOCK
		
		p=$(cat $pl2)
		if [ "$p" == "END" ]; then
			break
		fi
	done
	deletePlayer
}


function spectatorGame() {
	while true; do	
		p=$(cat $pl2)
		if [ "$p" == "END" ]; then
			break
		fi
		(
			flock -w 0.03 -n 200
			positions=( $p )
			drawBoard positions
			
			positions_str=$(printf "%s" "${positions[@]}")
			checkEndGame $positions_str
			sleep 1
		) 200>$LOCK
	done
}


# === main ===

determineRole

if [ $role -eq 1 ]; then
		firstPlayerGame
	else
		if [ $role -eq 2 ]; then
			secondPlayerGame
		else 
			spectatorGame
		fi
		
fi

