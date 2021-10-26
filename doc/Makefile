
all: iotcpd.1.html

iotcpd.1.html: iotcpd.1
	@mandoc -T html iotcpd.1 > $@

clean:
	@rm *.html
