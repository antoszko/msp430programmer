import sys
import serial
import os

def printuseage():
	print('useage: python3 msp430programmer.py <serial port> <baud rate> <hex file> [options]')
	print('options:')
	print('      -v --verbose	log verbose messages')

def main():
	# strlen args < 4 error
	# else set all vars
	# parse options for --verbose or -v
	if len(sys.argv) < 4:
		printuseage()
		return
	serialport = sys.argv[1]
	baudrate = int(sys.argv[2])
	hexfile = sys.argv[3]
	
	verbose = False
	for i in range(4, len(sys.argv)):
		if sys.argv[i] == '-v' or sys.argv[i] == '--verbose':
			verbose = True
		else:
			print('unknown option', sys.argv[i])
			
			
	# open port
	print('[Host] Opening serial port', serialport,'at', baudrate, 'baud rate')
	ser = serial.Serial(serialport, baudrate)
	
	# wait for 'Ready'
	msg = ser.readline().decode(errors='replace')
	while(not msg.startswith('Ready')):
		if not msg.startswith('-v'):
			print('      ', msg, end='')
		elif msg.startswith('-v') and verbose:
			print('      ', msg[2:], end='')
		# else print nothing	
		
		msg = ser.readline().decode(errors='replace')
	
	print('[Host] Programmer is ready')
	print('[Host] Opening file', hexfile)
	# open file
	f = open(hexfile, 'rb')

	# transmit size of file
	file_len = os.path.getsize(hexfile)
	print('[Host] Writing file length', file_len,'to programmer...')
	ser.write(file_len.to_bytes(2, 'little'))
		
	# maybe print response
	msg = ser.readline().decode(errors='replace')
	if not msg.startswith('-v'):
		print('      ', msg, end='')
	elif msg.startswith('-v') and verbose:
		print('      ', msg[2:], end='')
	
	# transmit file
	print('[Host] Writing file to programmer...')
	ser.write(f.read())
	
	if verbose:
		print('[Host] Finished writing file to programmer')
	
	#print responses until 'Exit'
	msg = ser.readline().decode(errors='replace')
	while(not msg.startswith('Exit')):
		if not msg.startswith('-v'):
			print('      ', msg, end='')
		elif msg.startswith('-v') and verbose:
			print('      ', msg[2:], end='')
		# else print nothing	
		
		msg = ser.readline().decode(errors='replace')
		
	print('[Host] Exit')

if __name__ == '__main__':
	main()
