 Pour tester :

gcc mini_serv.c -Wall -Wextra -Werror -o mini_serv
./mini_serv 8080
# Puis dans un autre terminal :
nc 127.0.0.1 8080
