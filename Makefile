CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread

# Параметры по умолчанию (можно переопределять: make run T=8 N=2000000)
N ?= 1000000
T ?= 4
M ?= 64
BIG = big.txt

all: lab3

lab3: main.o worker.o bigint.o
	$(CC) $(CFLAGS) -o lab3 main.o worker.o bigint.o

main.o: main.c bigint.h worker.h
	$(CC) $(CFLAGS) -c main.c

worker.o: worker.c worker.h bigint.h
	$(CC) $(CFLAGS) -c worker.c

bigint.o: bigint.c bigint.h
	$(CC) $(CFLAGS) -c bigint.c

$(BIG): gen.py
	python3 gen.py $(N) $(BIG)

data: $(BIG)

run: lab3 $(BIG)
	./lab3 -f $(BIG) -t $(T) -m $(M)

check: $(BIG)
	python3 check.py $(BIG)

test: lab3 $(BIG)
	@for f in small.txt $(BIG); do \
	  got=$$(./lab3 -f $$f -t $(T) -m $(M) | sed -n 's/^Среднее значение: 0x//p' | sed 's/^0*//'); \
	  exp=$$(python3 check.py $$f | sed -n 's/^mean = //p'); \
	  if [ "$$got" = "$$exp" ]; then echo "$$f: OK"; else echo "$$f: FAIL"; echo "  got: $$got"; echo "  exp: $$exp"; fi; \
	done

bench: lab3 $(BIG)
	@for t in 1 2 4 8 16; do \
	  printf "threads=%s  " $$t; ./lab3 -f $(BIG) -t $$t -m $(M) | sed -n 's/^Время работы: //p'; \
	done

threads: lab3 $(BIG)
	@./lab3 -f $(BIG) -t $(T) -m $(M) > /dev/null & pid=$$!; \
	sleep 0.1; \
	echo "PID=$$pid"; \
	ps -o pid,nlwp,cmd -p $$pid; \
	echo "Потоков в /proc: $$(ls /proc/$$pid/task 2>/dev/null | wc -l)"; \
	wait $$pid

clean:
	rm -f *.o lab3

distclean: clean
	rm -f $(BIG)

.PHONY: all data run check test bench threads clean distclean