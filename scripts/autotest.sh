#!/bin/bash 

disk_array=(chocolate strawberry vanilla yam durian mango)

function clear {
	rm bitflurry.db;
	rm /disks/*/*;
	./bitflurry init;
	./bitflurry put "$1";
}  

function repair {
    ./bitflurry fsck
    ./bitflurry get $1 test.avi
} 

for i in {1..5};
do
	echo =============== CLEARING $i ===============
        clear $2;
	rm /disks/${disk_array[2]}/*;
	#rm /disks/${disk_array[1]}/*;
	echo =============== REPAIRING $i =============== 
        repair $2;
        echo ========== TEST $i ========== >> tests/$1_result.md5	
	md5sum $2 test.avi >> tests/$1_result.md5
done

