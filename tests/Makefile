# Testy modulow firmware niezaleznych od sprzetu, uruchamiane na PC.
#   make -C tests        (Linux / WSL: wsl make -C tests)
FW      := ../firmware/CanGnssNode/Core
CC      ?= gcc
CFLAGS  := -std=c11 -O2 -Wall -Wextra -Werror -Wconversion -Wno-sign-conversion \
           -fsanitize=address,undefined -fno-omit-frame-pointer -I$(FW)/Inc
BUILD   := build

.PHONY: all clean
all: $(BUILD)/test_gnss_nmea
	./$(BUILD)/test_gnss_nmea

$(BUILD)/test_gnss_nmea: test_gnss_nmea.c $(FW)/Src/gnss_nmea.c $(FW)/Inc/gnss_nmea.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ test_gnss_nmea.c $(FW)/Src/gnss_nmea.c

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
