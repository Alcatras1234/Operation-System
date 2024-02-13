#!/bin/bash
#Данная программа проверяет, введенное значение пользователем больше 10, меньше или является отрицательным. Если пользователь ввел не число, говорится что введенное им значение не является числом.
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
echo "Programm end"
