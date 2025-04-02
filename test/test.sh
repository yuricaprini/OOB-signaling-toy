#!/bin/bash  

echo "========================================================================="
echo "-----------------------------TEST STARTED--------------------------------"
echo "========================================================================="

(./OOB_supervisor 8 >> OOB_supervisor.log ) &
SUPERVISOR_PID=$!

sleep 2

for i in {1..10}; do
			(./OOB_client 5 8 20 | tee -a OOB_client.log &);
			(./OOB_client 5 8 20 | tee -a OOB_client.log &)
			sleep 1
done  

for i in {1..6}; do
    kill -SIGINT $SUPERVISOR_PID
		sleep 10
done

kill -SIGINT $SUPERVISOR_PID
kill -SIGINT $SUPERVISOR_PID

../test/misura.sh OOB_client.log OOB_supervisor.log

echo "========================================================================="
echo "------------------------------TEST ENDED---------------------------------"
echo "========================================================================="

exit 0
