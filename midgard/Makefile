CC = gyc
CFLAGS = 
LDFLAGS = 


SRC = $(wildcard **/*.yr)
OBJS = $(SRC:.yr=.o)
AOUT = libgymidgard


all : $(AOUT) 

libgymidgard : $(OBJS)
	ar rcs libgymidgard.a obj/*.o

%.o : %.yr
	$(CC) $(CFLAGS) -c $<
	mv *.o obj/

install: $(INCLUDE)
	cp libgymidgard.a /usr/lib/

clean :
	@rm *.o

cleaner : clean
	@rm $(AOUT)
