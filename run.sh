./server $1 &
./client localhost $1
killall -v ./server
