clean:
		rm -f *.pdf

dist: clean
	libreoffice --headless --convert-to pdf *.odt && \
	cp *.pdf ../dist

all: dist

upload: dist
	scp *.pdf colino.net:www/wordpress/wp-content/ && \
	ssh colino.net "curl -X PURGE https://www.colino.net/wordpress/wp-content/mastodon-user-guide.pdf"
