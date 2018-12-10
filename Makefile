.PHONY: all
all: update gzip clean
gzip:
	cp data_work/index.html data/
	cp data_work/WebSocket.js data/
	cp data_work/main.css data/
	# TODO: add minifier stage
	-gzip -9f data/index.html 2> /dev/null
	-gzip -9f data/WebSocket.js 2> /dev/null
	-gzip -9f data/main.css 2> /dev/null
	-gzip -9f data/iro.min.js 2> /dev/null
update:
	git submodule update
	cp iro.js/dist/iro.min.js data/iro.min.js	
clean:
	rm -rf data/.DS_Store
