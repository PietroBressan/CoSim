earth:
	g++ earth.cpp -o Earth -Iinclude -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

plot:
	g++ plot.cpp -o Plot -Iinclude -lraylib -lGL -lm -lpthread -ldl -lrt -lX11