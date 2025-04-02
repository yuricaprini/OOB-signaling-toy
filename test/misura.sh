#!/bin/bash
 
cli_log="OOB_client.log"
sup_log="OOB_supervisor.log"

sup_ok=0
cli_ok=0

#check degli argomenti
for i in $@; do
	if [ ! -e $i ];then
		echo "$i non è un file"
		exit 1
	fi
	if [ $i = $cli_log ];then
		((cli_ok++));
	fi
	if [ $i = $sup_log ];then
		((sup_ok++));
	fi
done

echo "-----------------------------AVVIO MISURA--------------------------------"

if [ $cli_ok -ne 1 ] || [ $sup_ok -ne 1 ]; then
	echo "Argomenti errati"	
	exit 1
fi

#variabile contenente tutti gli id dei client terminati
cli_done=`more $cli_log | grep DONE | cut -d ' ' -f2`

right_est=0
estimate=0
secret=0
wrong_est=0
err_sum=0

for i in $cli_done; do

	estimate=`more $sup_log | grep $i | grep BASED | cut -d ' ' -f3`
	secret=`more $cli_log | grep $i | grep SECRET| cut -d ' ' -f4`

	if [ -z "$estimate" ]; then
		estimate=0
	fi

	if [ $(($secret-25)) -lt $estimate ] && [ $estimate -lt $(($secret+25)) ]; then
		((right_est++));
	else
		diff=$(($estimate-$secret))
		if [ $diff -lt 0 ];then
			diff=$(($diff * -1))
		fi
		err_sum=$(($err_sum+$diff))
		((wrong_est++))
	fi
		
done

echo "Numero stime corrette: $right_est"

if [ $wrong_est -gt 0 ]; then
	echo "Errore medio di stima: $(($err_sum/$wrong_est)) "
else
	echo "Errore medio di stima: 0 "
fi

echo "----------------------------CHIUSURA MISURA------------------------------"


