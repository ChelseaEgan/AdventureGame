rooms:
	gcc -g -o eganch.buildrooms eganch.buildrooms.c
adventure:
	gcc -g -o eganch.adventure eganch.adventure.c
clean:
	find . -name "eganch.r*" -exec rm -rf {} \;
	rm -f eganch.buildrooms eganch.adventure