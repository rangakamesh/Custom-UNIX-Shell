sshell: bash.c
	gcc bash.c -o sshell
clean:
	rm -f sshell *.o *.config *.bak core \#*


