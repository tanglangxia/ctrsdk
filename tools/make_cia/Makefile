OBJS = utils.o ctr_crypto.o settings.o main.o srl.o ncsd.o ncch.o cia.o ticket.o tmd.o meta.o
POLAR_OBJS = polarssl/aes.o polarssl/bignum.o polarssl/rsa.o polarssl/sha1.o polarssl/sha2.o polarssl/padlock.o polarssl/md.o polarssl/md_wrap.o polarssl/md2.o polarssl/md4.o polarssl/md5.o polarssl/sha4.o polarssl/base64.o polarssl/cipher.o polarssl/cipher_wrap.o polarssl/camellia.o polarssl/des.o polarssl/blowfish.o
LIBS = -static-libgcc -static-libstdc++
CXXFLAGS = -I.
CFLAGS = --std=c99 -Wall -I.
OUTPUT = make_cia
CC = gcc

main: $(OBJS) $(POLAR_OBJS)
	g++ -o $(OUTPUT) $(LIBS) $(OBJS) $(POLAR_OBJS)


clean:
	rm -rf $(OUTPUT) $(OBJS) $(POLAR_OBJS)