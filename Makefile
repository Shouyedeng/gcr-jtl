
OBJ=main.o gcrChannel.o gcrColl.o gcrDebug.o  gcrEdge.o gcrFeas.o gcrFlags.o gcrInit.o gcrLib.o gcrRiver.o gcrRoute.o gcrShwFlgs.o gcrUnsplit.o malloc.o geometry.o hash.o lookup.o rtrPin.o search.o

gcr: ${OBJ}
	cc -o gcr ${OBJ}
	mv *.o ./obj
clean:
	rm obj/*
	
main.o: main.c magic.h geometry.h gcr.h router.h tile.h
	gcc -c main.c -o main.o

gcrChannel.o: gcrChannel.c magic.h geometry.h gcr.h malloc.h
	gcc -c gcrChannel.c -o gcrChannel.o

gcrColl.o: gcrColl.c magic.h geometry.h tile.h gcr.h malloc.h
	gcc -c gcrColl.c -o gcrColl.o

gcrDebug.o: gcrDebug.c magic.h geometry.h textio.h tile.h hash.h database.h gcr.h heap.h router.h malloc.h
	gcc -c gcrDebug.c -o gcrDebug.o

gcrEdge.o: gcrEdge.c magic.h geometry.h tile.h gcr.h
	gcc -c gcrEdge.c -o gcrEdge.o

gcrFeas.o: gcrFeas.c magic.h geometry.h tile.h gcr.h hash.h database.h router.h
	gcc -c gcrFeas.c -o gcrFeas.o

gcrFlags.o: gcrFlags.c magic.h geometry.h tile.h gcr.h
	gcc -c gcrFlags.c -o gcrFlags.o

gcrInit.o: gcrInit.c magic.h geometry.h hash.h heap.h tile.h database.h router.h gcr.h malloc.h
	gcc -c gcrInit.c -o gcrInit.o

gcrLib.o: gcrLib.c magic.h geometry.h tile.h gcr.h malloc.h
	gcc -c gcrLib.c -o gcrLib.o

gcrRiver.o: gcrRiver.c magic.h geometry.h gcr.h textio.h malloc.h
	gcc -c gcrRiver.c -o gcrRiver.o

gcrRoute.o: gcrRoute.c magic.h geometry.h gcr.h signals.h malloc.h styles.h
	gcc -c gcrRoute.c -o gcrRoute.o

gcrShwFlgs.o: gcrShwFlgs.c magic.h geometry.h hash.h tile.h database.h windows.h dbwind.h gcr.h heap.h router.h main.h styles.h textio.h
	gcc -c gcrShwFlgs.c -o gcrShwFlgs.o

gcrUnsplit.o: gcrUnsplit.c magic.h geometry.h tile.h gcr.h malloc.h
	gcc -c gcrUnsplit.c -o gcrUnsplit.o

malloc.o: malloc.c magic.h malloc.h
	gcc -c malloc.c -o malloc.o

geometry.o: geometry.c magic.h geometry.h utils.h textio.h tile.h 
	gcc -c geometry.c -o geometry.o


hash.o: hash.c magic.h hash.h malloc.h
	gcc -c hash.c -o hash.o

lookup.o: lookup.c magic.h utils.h
	gcc -c lookup.c -o lookup.o

rtrPin.o: rtrPin.c magic.h geometry.h styles.h hash.h heap.h debug.h tile.h database.h gcr.h windows.h main.h dbwind.h signals.h router.h grouter.h textio.h grouteDebug.h
	gcc -c rtrPin.c -o rtrPin.o 

search.o: search.c magic.h geometry.h tile.h
	gcc -c search.c -o search.o
