for i in 0 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 85 90
do
	./server $i &
	./client localhost $i
	killall -v ./server
done
