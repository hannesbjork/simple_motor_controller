
from ctypes import *
#import bitarray

so_file = "/home/hannes/Git/Projekt/mekatronik/Local/serial_lib.so"
serial_lib = CDLL(so_file)
print(type(serial_lib))

serial_port = serial_lib.open_port()

def bitfield(n):
    return [1 if digit=='1' else 0 for digit in bin(n)[2:]]

def read():

	raw_num = serial_lib.recieve_char(serial_port)
	bitlist = [raw_num >> i & 1 for i in range(raw_num.bit_length() - 1,-1,-1)]
	#final = bitarray.bitarray(bitlist).tobytes().decode('utf-8')
	final = "".join(chr(int("".join(map(str,bitlist[i:i+8])),2)) for i in range(0,len(bitlist),8))

	return final
	
def write(word):
	serial_lib.send_word(serial_port, word)


write("a")
print( read() )
write("b")
print( read() )
write("c")
print( read() )
write("d")
print( read() )
write("e")
print( read() )



