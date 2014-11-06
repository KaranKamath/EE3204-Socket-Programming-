for i in 1 2 3 4 5
do
	./server $i &
	./client localhost $i
	killall -v ./server
done
