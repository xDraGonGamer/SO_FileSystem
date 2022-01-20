#/bin/bash


make
echo
echo "==================================="

for profsTest in tests/*.c
do
	s=${profsTest##*/}
	echo ${s%.*}
	./tests/${s%.*}
	echo 
done


for ourTest in tests/ourTests/*.c
do
	s=${ourTest##*/}
	echo ${s%.*}
	./tests/ourTests/${s%.*}
	echo 
done
