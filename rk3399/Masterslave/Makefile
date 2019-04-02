CROSS=arm-linux-
#CROSS=arm-none-linux-gnueabi-
#CROSS=

all: protocol 

protocol: 
	#$(CROSS)gcc -o protocol gosida.c -lpthread
	#$(CROSS)gcc -o2 -g static -o protocol gosida.c -lpthread
	#$(CROSS)gcc -o2 -DDEBUG -o protocol gosida.c -lpthread cp protocol /opt/gsd -f
	$(CROSS)gcc -O2 -o protocol *.c -pthread -lm -static
	#$(CROSS)gcc -o2 -o protocol gosida.c -lpthread
	#cp protocol /opt/samples/ -f
clean:
	@rm -vf protocol *.o *~
install:	
	arm-linux-strip protocol
	#cp protocol /opt/samples/ -f
	#cp protocol ../arm-linux-2.6.28/target/rootfs-cpio/product/samples/ -rf
	#sudo chmod 777 -R ../arm-linux-2.6.28/target/rootfs-cpio/product/samples/* 
bak:
	cp ../gosida ../sw_bak/ -rf
