all: supervisor client 
# supervisor_udp client_udp 
#test_client test_server
#test_client_udp test_server_udp
#test_priorities test_nanosleep test_rta test_wcet test_pthread

supervisor: supervisor.c
	${CC} -O0 -g3 $^ -o $@ -lm -lpthread

client: client.c
	${CC} -O0 -g3 $^ -o $@ -lm

supervisor_udp: supervisor_udp.c
	${CC} -O0 -g3 $^ -o $@ -lm -lpthread

client_udp: client_udp.c
	${CC} -O0 -g3 $^ -o $@ -lm

test_priorities: test_priorities.c
	${CC} -O0 -g3 $^ -o $@ -lm -lpthread

test_client_udp: test_client_udp.c
	${CC} -O0 -g3 $^ -o $@ -lm -lpthread

test_server_udp: test_server_udp.c
	${CC} -O0 -g3 $^ -o $@ -lm -lpthread

test_client: test_client.c
	${CC} -O0 -g3 $^ -o $@ -lm -lpthread

test_server: test_server.c
	${CC} -O0 -g3 $^ -o $@ -lm -lpthread

test_nanosleep: test_nanosleep.c
	${CC} -O0 -g3 $^ -o $@ -lm -lpthread

test_rta: test_rta.c
	${CC} -O0 -g3 $^ -o $@ -lm -lpthread

test_wcet: test_wcet.c
	${CC} -O0 -g3 $^ -o $@ -lm -lpthread

test_pthread: test_pthread.c
	${CC} -O0 -g3 $^ -o $@ -lm -lpthread

clean:
	rm -f supervisor client 
#	rm -f supervisor_udp client_udp
#	rm -f test_priorities test_nanosleep test_rta test_wcet test_pthread
#	rm -f test_client test_server
# 	rm -f test_client_udp test_server_udp