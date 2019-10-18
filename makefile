rooms:
	gcc -g -o eganch.buildrooms eganch.buildrooms.c
adventure:
	gcc -g -o eganch.adventure eganch.adventure.c -lpthread
clean:
	rm -f eganch.buildrooms eganch.adventure
cleanRooms:
	find . -name "eganch.r*" -exec rm -rf {} \;
