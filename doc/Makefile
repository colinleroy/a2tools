clean:
		rm -f *.pdf

dist: clean
	libreoffice --headless --convert-to pdf *.odt && \
	cp *.pdf ../dist
