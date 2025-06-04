OUT = ../../initrd/usr/bin/specseek

PHONY: all
all:
	if [ ! -d "SpecSeek-forTacOS" ]; then \
		git clone https://github.com/UnmappedStack/SpecSeek-forTacOS --depth=1; \
	else \
		echo "SpecSeek-forTacOS already exists, skipping clone."; \
	fi
	make -C SpecSeek-forTacOS tacos
	cp SpecSeek-forTacOS/bin/tacos/64/specseek_tacos_64 $(OUT)

clean:
	rm -rf SpecSeek-forTacOS
