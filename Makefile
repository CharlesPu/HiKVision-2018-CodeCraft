EXEC = main.exe
DIR_CURR=./
DIR_OBJ=./OBJ
ALL_DIR= ${DIR_CURR} 
JSON_LIB_DIR = /usr/include/json-c

C_SRC  =${wildcard ${DIR_CURR}/*.c}
		
C_OBJS = ${patsubst %.c, ${DIR_OBJ}/%.o, ${notdir ${C_SRC}}}

CC = gcc	
CFLAGS = -O2 -Wall -fopenmp -I${ALL_DIR} -I${JSON_LIB_DIR} 
LDFLAGS	=-fopenmp -lm -L/usr/lib/x86_64-linux-gnu/ -ljson-c	

all:$(EXEC)

$(EXEC):$(C_OBJS)	
	$(CC) -o $@ $(C_OBJS) $(LDFLAGS)
	@echo "Make done~"
	
${DIR_OBJ}/%.o:${DIR_CURR}/%.c	
	@if [ ! -d $(DIR_OBJ) ]; then mkdir -p $(DIR_OBJ); fi; 
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf ${DIR_OBJ}/*.o
	@rm -rf ${DIR_OBJ}
	@rm -rf ${EXEC}
	@echo "Clean done~"
