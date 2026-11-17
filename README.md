# read_FuelLevel_Komatsu_HD785

Pembacaan level solar Komatsu HD785 dengan **Arduino Mega 2560 (ATmega2560)**, dengan cara menyadap tegangan sinyal sensor fuel level lalu mengubahnya ke persen memakai tabel kalibrasi (interpolasi linear).

Proyek ini memakai **PlatformIO**.

## Struktur proyek

```
platformio.ini                 konfigurasi board, build, dan test
include/Config.h               SEMUA parameter lapangan: pin, kalibrasi, batas fault, filter
src/main.cpp                   timing sampling, analogRead, output Serial
lib/FuelLevel/src/FuelLevel.*  logika: ADC -> volt -> persen, deteksi fault, oversampling, filter EMA
test/test_fuel_level/          unit test (Unity)
```

Logika di `lib/FuelLevel` tidak memanggil `analogRead`/`millis`, jadi bisa diuji tanpa sensor.

## Cara pakai

Install ekstensi **PlatformIO IDE** di VS Code, buka folder repo ini, lalu:

| Perintah | Fungsi |
|---|---|
| `pio run` | Build firmware |
| `pio run -t upload` | Build & upload ke Mega 2560 |
| `pio device monitor` | Lihat output Serial (115200 baud) |
| `pio test -e simavr` | Jalankan unit test di simulator AVR (tanpa board) |
| `pio test -e megaatmega2560` | Jalankan unit test di board fisik |

Catatan: `pio test -e megaatmega2560` **menimpa firmware** di board dengan firmware test. Setelahnya upload ulang firmware utama dengan `pio run -t upload`. Env `simavr` tidak pernah meng-upload ke board.

## Wiring (Mega 2560)

```
Sinyal sensor fuel ──[ 22k–47k ]──┬────────── A0 (Mega 2560)
                                  ├──[100nF]── GND
                                  └──[Zener 5V1]── GND   (katoda ke A0)

GND Mega ──────────────────────────────────── ground sensor / monitor unit
Power: 24 V unit ──► DC-DC 24 V→9 V (ke VIN) atau 24 V→5 V (ke USB/5V), tahan surge
```

- Pin input **A0**. Di Mega, I2C ada di pin 20/21 sehingga A0..A15 bebas.
- Jangan sambungkan 24–28 V langsung ke VIN Mega (rekomendasi 7–12 V).
- Resistor seri juga mencegah Mega yang sedang mati membebani sinyal sensor.

## Output Serial (115200 baud, 1x per detik)

```
adc=215.3,v=1.051,fuel_raw=49.9,fuel=50.0,status=OK
```

| Key | Arti |
|---|---|
| `adc` | Rata-rata 20 sampel ADC (oversampling 20 Hz) |
| `v` | Tegangan sinyal sensor (Volt) |
| `fuel_raw` | Persen per detik, tanpa filter lambat (untuk dibandingkan dengan display saat unit diam). `nan` saat fault |
| `fuel` | Persen terfilter (EMA, `TAU_S` = 30 detik) untuk meredam goyangan solar saat unit jalan. `nan` sampai ada pembacaan valid pertama |
| `status` | `OK`, `FAULT_SHORT` (sinyal short ke ground), `FAULT_OPEN` (kabel putus / konektor lepas) |

## Kalibrasi (`include/Config.h`)

Tabel default (tegangan naik = solar turun):

| Volt | 0.100 | 0.575 | 1.050 | 1.390 | 1.785 |
|---|---|---|---|---|---|
| Fuel % | 100 | 75 | 50 | 25 | 0 |

- `VOLT_TABLE` / `FUEL_TABLE`: build akan **gagal** jika `VOLT_TABLE` tidak urut naik, kedua tabel beda panjang, atau batas fault berada di dalam rentang tabel.
- `ADC_REFERENCE` + `VREF`:
  - `DEFAULT` + `5.00` (bawaan): sama seperti versi Uno. Biarkan `5.00` jika tabel diambil dari angka Serial Monitor; isi hasil multimeter pin 5V jika tabel diukur dengan multimeter.
  - `INTERNAL2V56` (khusus Mega): resolusi 2x lebih halus dan tidak terpengaruh naik-turun 5V. Ukur tegangan pin **AREF** dengan multimeter, isi ke `VREF`, lalu verifikasi ulang kalibrasi.
- `V_FAULT_LOW` / `V_FAULT_HIGH`: **wajib diverifikasi di unit**. Ukur tegangan saat konektor sensor dicabut (open) dan saat sinyal di-short ke ground.
- `TAU_S`: makin besar makin halus, tapi makin lambat mengikuti perubahan (misalnya saat isi solar).

Kalibrasi dengan sumber daya yang sama dengan yang dipakai di unit. Karena board berganti dari Uno ke Mega, **verifikasi ulang** pembacaan terhadap display unit.

## Checklist observasi

1. Display unit tidak berubah saat Mega ON / OFF / dicabut.
2. Tegangan sensor sama saat kunci ON mesin mati dan saat mesin hidup (alternator ±27–28 V).
3. Uji di titik di antara titik kalibrasi (10, 40, 60, 90%) dan dekat kosong.
4. Bandingkan saat parkir datar, di tanjakan, dan saat jalan; atur `TAU_S` sesuai hasilnya.
5. Saat isi solar, catat liter dari fuel truck dan bandingkan dengan kenaikan persen (dasar tabel persen → liter).

## Changelog

**v3 (PlatformIO, Mega 2560)**
- Pindah dari sketch Arduino IDE ke proyek PlatformIO, board `megaatmega2560`.
- Parameter lapangan dipusatkan di `include/Config.h`, dengan pengecekan tabel kalibrasi saat compile.
- Logika dipisah ke library `FuelLevel` dan diuji dengan 11 unit test (Unity, jalan di simulator simavr).
- Opsi referensi ADC internal 2.56 V khusus Mega.

**v2**
- Filter tidak lagi mulai dari 0 (sebelumnya 4 detik pertama terbaca terlalu tinggi, mis. 94% padahal 50%).
- Deteksi fault sensor: short ke ground tidak lagi tampil 100%, kabel putus tidak lagi tampil 0%.
- Oversampling 20 sampel/detik dan rata-rata dalam float (tanpa pembulatan integer).
- Filter EMA (30 detik) untuk meredam sloshing; nilai per detik tetap tersedia di `fuel_raw`.
- Konversi ADC memakai pembagi 1024 (sesuai datasheet).
- Pin input pindah dari A5 ke A0.
- Output format `key=value` agar mudah di-parse.
