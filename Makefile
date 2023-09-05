mac:
	g++ -I /usr/local/include -L /usr/local/lib -o stars -std=c++11 -lSDL2main -lSDL2 -lSDL2_image stars.cc 
	g++ -I /usr/local/include -L /usr/local/lib -o boxes -std=c++11 -lSDL2main -lSDL2 -lSDL2_image boxes.cc 
	g++ -I /usr/local/include -L /usr/local/lib -o tris -std=c++11 -lSDL2main -lSDL2 -lSDL2_image tris.cc 
