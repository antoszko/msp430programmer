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

	print('[Host] Waking up programmer...')
	ser.write(b'Wakeup!')
	
	# wait for 'Starting'
	msg = ser.readline().decode(errors='replace')
	while(not msg.startswith('Starting')):
		if msg.startswith('-v'):
			if verbose:
				print('      ', msg[2:], end='') 
		elif msg.startswith('Exit'):
			print('[Host] Received exit signal from programmer');
			exit();
		else :
			print('      ', msg, end='')
		
		ser.write(b'Wakeup!')
		msg = ser.readline().decode(errors='replace')
	
	print('      ', msg, end='')
	# wait for 'Ready'
	msg = ser.readline().decode(errors='replace')
	while(not msg.startswith('Ready')):
		if msg.startswith('-v'):
			if verbose:
				print('      ', msg[2:], end='') 
		elif msg.startswith('Exit'):
			print('[Host] Received exit signal from programmer');
			exit();
		else :
			print('      ', msg, end='')
			
		msg = ser.readline().decode(errors='replace')
	
	print('      ', msg, end='')
	print('[Host] Programmer is ready')
	print('[Host] Opening file', hexfile)
	# open file
	f = open(hexfile, 'rb')

	# transmit size of file
	file_len = os.path.getsize(hexfile)
	print('[Host] Writing file length', file_len,'to programmer...')
	ser.write(file_len.to_bytes(2, 'little'))
	
	# await ack in form of 'Received %d\n'	
	msg = ser.readline().decode(errors='replace')
	while(not msg.startswith('Received')):
		if msg.startswith('-v'):
			if verbose:
				print('      ', msg[2:], end='') 
		elif msg.startswith('Exit'):
			print('[Host] Received exit signal from programmer');
			exit();
		else :
			print('      ', msg, end='')
			
		msg = ser.readline().decode(errors='replace')
	
	print('      ', msg, end='')
	_, received_file_len = msg.split(' ')
	success = False
	try:
		if int(received_file_len) == file_len:
			success = True
	except:
		pass
		
	if not success:
		print('[Host] programmer received incorrect file length. Exiting.')
		ser.write(b'Exit')
		exit()
	
	# transmit file
	print('[Host] Writing file to programmer...')
	ser.write(f.read())
	
	if verbose:
		print('[Host] Finished writing file to programmer')
	
	#print responses until 'Exit'
	msg = ser.readline().decode(errors='replace')
	while(not msg.startswith('Exit')):
		if msg.startswith('-v'):
			if verbose:
				print('      ', msg[2:], end='') 
		elif msg.startswith('Exit'):
			print('[Host] Received exit signal from programmer');
			exit();
		else :
			print('      ', msg, end='')
		
		msg = ser.readline().decode(errors='replace')
		
	print('[Host] Exit')

if __name__ == '__main__':
	main()
