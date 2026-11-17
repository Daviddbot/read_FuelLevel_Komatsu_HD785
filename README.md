# read_FuelLevel_Komatsu_HD785

Pembacaan level solar Komatsu HD785 dengan Arduino Uno, dengan cara menyadap tegangan sinyal sensor fuel level lalu mengubahnya ke persen memakai tabel kalibrasi (interpolasi linear).

Sketch: [`Read_FuelLevel_HD785/Read_FuelLevel_HD785.ino`](Read_FuelLevel_HD785/Read_FuelLevel_HD785.ino)

## Wiring

```
Sinyal sensor fuel ──[ 22k–47k ]──┬────────── A0 (Arduino Uno)
                                  ├──[100nF]── GND
                                  └──[Zener 5V1]── GND   (katoda ke A0)

GND Arduino ───────────────────────────────── ground sensor / monitor unit
Power: 24 V unit ──► DC-DC 24 V→5 V (tahan surge) ──► 5V / USB Arduino
```

- Pin input sekarang **A0** (versi lama A5). A4/A5 dipakai I2C di Uno.
- Jangan sambungkan 24–28 V langsung ke VIN Uno.
- Resistor seri juga mencegah Arduino yang sedang mati membebani sinyal sensor.

## Output Serial (115200 baud, 1x per detik)

```
adc=215.3,v=1.051,fuel_raw=49.9,fuel=50.0,status=OK
```

| Key | Arti |
|---|---|
| `adc` | Rata-rata 20 sampel ADC (oversampling 20 Hz) |
| `v` | Tegangan sinyal sensor (Volt) |
| `fuel_raw` | Persen per detik, tanpa filter lambat (untuk dibandingkan dengan display saat unit diam). `nan` saat fault |
| `fuel` | Persen terfilter (EMA, `TAU_S` = 30 detik) untuk meredam goyangan solar saat unit jalan |
| `status` | `OK`, `FAULT_SHORT` (sinyal short ke ground), `FAULT_OPEN` (kabel putus / konektor lepas) |

## Kalibrasi

Tabel default (tegangan naik = solar turun):

| Volt | 0.100 | 0.575 | 1.050 | 1.390 | 1.785 |
|---|---|---|---|---|---|
| Fuel % | 100 | 75 | 50 | 25 | 0 |

Parameter di bagian atas sketch:

- `VREF`: tegangan pin 5V Arduino. Biarkan `5.00` jika tabel kalibrasi diambil dari angka Serial Monitor. Isi dengan hasil multimeter jika tabel diukur dengan multimeter.
- `V_FAULT_LOW` / `V_FAULT_HIGH`: **wajib diverifikasi di unit**. Ukur tegangan saat konektor sensor dicabut (open) dan saat sinyal di-short ke ground.
- `TAU_S`: makin besar makin halus, tapi makin lambat mengikuti perubahan (misalnya saat isi solar).

Kalibrasi dengan sumber daya yang sama dengan yang dipakai di unit, karena ADC memakai 5V Arduino sebagai referensi.

## Checklist observasi

1. Display unit tidak berubah saat Arduino ON / OFF / dicabut.
2. Tegangan sensor sama saat kunci ON mesin mati dan saat mesin hidup (alternator ±27–28 V).
3. Uji di titik di antara titik kalibrasi (10, 40, 60, 90%) dan dekat kosong.
4. Bandingkan saat parkir datar, di tanjakan, dan saat jalan; atur `TAU_S` sesuai hasilnya.
5. Saat isi solar, catat liter dari fuel truck dan bandingkan dengan kenaikan persen (dasar tabel persen → liter).

## Changelog

**v2**
- Filter tidak lagi mulai dari 0 (sebelumnya 4 detik pertama terbaca terlalu tinggi, mis. 94% padahal 50%).
- Deteksi fault sensor: short ke ground tidak lagi tampil 100%, kabel putus tidak lagi tampil 0%.
- Oversampling 20 sampel/detik dan rata-rata dalam float (tanpa pembulatan integer).
- Filter EMA (30 detik) untuk meredam sloshing; nilai per detik tetap tersedia di `fuel_raw`.
- Konversi ADC memakai pembagi 1024 (sesuai datasheet ATmega328P).
- Pin input pindah dari A5 ke A0.
- Output format `key=value` agar mudah di-parse.
