def main():
#	f= open("10G.txt","a+")
#	for i in range(1000000):
#		f.write("This is line %d\r\n" % (i+1))
#	f.close()

	f=open("file", "r")
	while True:
	    line = f.readline()
	    if not line: break

if __name__== "__main__":
	main()
