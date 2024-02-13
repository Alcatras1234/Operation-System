#!/bin/bash
#Данная программа проверяет, введенное значение пользователем больше 10, меньше или является отрицательным. Если пользователь ввел не число, говорится что введенное им значение не является числом. Так же реализован повтор ввода и возможность выйти из программы завершив ее и считает сколько раз была воспроизведенна программа

ch=4

counter=0

while ! [ "$ch" -eq 0 ]; do
	echo "Код был воспроизведен $counter раз "
	echo ""
	echo ""
	read -p "Enter number: " num
	if [[ $num =~ ^[0-9]+$ ]]; then
		if [ "$num" -ge 10 ]; then 
			echo "Your number is bigger than or equal 10, Являеется ЧИСЛОМ"
		elif [ "$num" -lt 10 ]; then
			echo "Your number is less than 10, Является ЦИФРОЙ"
		elif [ "$num" -lt 0 ]; then 
			echo "$num less than zero"
		fi
			
	else 
		echo "$num is not a Number"
	fi
	echo "ВВедите 0 для выхода или нажмите 1 для повтора"
	read ch
	counter=$(($counter + 1))
done
echo "Programm end"
exit 0
