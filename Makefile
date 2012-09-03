all:
	@waf build

configure:
	@waf configure

install:
	@waf install --progress

uninstall:
	@waf uninstall

dist:
	@waf dist

clean:
	@waf clean
