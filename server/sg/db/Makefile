
all:
	@curl https://easylist-downloads.adblockplus.org/easylist.txt > easylist.txt
	@sed -f patterns.sed easylist.txt > expressions
