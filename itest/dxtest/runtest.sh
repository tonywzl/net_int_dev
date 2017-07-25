#!/bin/bash
echo "Begin clean ENV ..."

./nidmanager -r s -S
./nidmanager -r a -S

cp /etc/ilio/nidserver.conf /etc/ilio/nidserver.conf.bak
cp nidserver.conf /etc/ilio/nidserver.conf

echo "Start nidserver ..."
./nidserver; 
sleep 5;

mv /etc/ilio/nidserver.conf.bak /etc/ilio/nidserver.conf

echo "Check nidserver state ..."
trycnt0=20

while [[ $trycnt0 > 0 ]]
do
	sleep 2;
	ret=$(./nidmanager -r s -s get | grep "rc:-");
        if [[ "$ret" == "" && $trycnt0 == 1 ]]; then
                ./nidmanager -r s -s get;
                echo "NID server state faild.";
                exit -1;
        elif [[ "$ret" != "" ]]; then
                echo "Try time left $trycnt0 : $ret"
                trycnt0=$[$trycnt0 - 1];
        else
                echo "Verify nidserver state working"
                break;
        fi
done

echo ""
echo "Verify nidserver state ..."

trycnt2=10
while [[ $trycnt2 > 0 ]]
do
	sleep 2;
	ret=$(./nidmanager -r s -s get | grep "successfully");
	if [[ "$ret" == "" && $trycnt2 == 1 ]]; then
		./nidmanager -r s -s get
        	echo "NID server state error.";
		exit -1;
	elif [[ "$ret" == "" ]]; then
                trycnt2=$[$trycnt2 - 1];
        else
		echo "Verify nidserver state ok."
                break;
	fi
done

echo "ITest for dxtest start nidserver successfully."
exit 0
