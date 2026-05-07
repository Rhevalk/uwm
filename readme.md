
# `uwm` - micro window manager

```text
      d8      d8  8b      db      d8  88,dPYba,,adPYba, 
     d8'     d8'  `8b    d88b    d8'  88P'    "88"    "8a
    d8'     d8'    `8b  d8'`8b  d8'   88      88      88
   d8a,   ,a8'      `8bd8'  `8bd8'    88      88      88
  d8'lbµwmd8l ,p      YP      YP      88      88      88
 d8'       '8A'                                         
d8'                                                     
```

**uwm** adalah pengelola jendela (Window Manager) super minimalis yang dibangun di atas X11 menggunakan library XCB. Didesain untuk efisiensi kognitif dan performa hardware maksimal.

## Filosofi "Organic Memory"
`uwm` tidak menyediakan bar, widget, atau indikator status. Sistem ini memaksa Anda untuk menggunakan memori organik (otak) untuk mengingat status workspace dan jendela. Jika Anda merasa kewalahan (*overwhelmed*), itu adalah indikator sistem bahwa Anda perlu mengurangi beban kerja.

## Aturan Ketat
* **No Border:** Visual yang bersih, tanpa gangguan garis tepi.
* **No Bar:** Tidak ada polling status, tidak ada jam, tidak ada informasi workspace.
* **Only 2 Windows:** Maksimal 2 jendela per workspace untuk menjaga fokus tunggal atau perbandingan minimal.
* **8 Workspaces:** Ruang terbatas untuk kedisiplinan alur kerja.

## Fitur
* **XCB Based:** Tanpa overhead, penggunaan RSS memori sangat rendah (~2MB).
* **Vertical Split:** Otomatis membagi layar secara vertikal jika ada 2 jendela.
* **Workspace Swapping:** Pindahkan seluruh konteks workspace dengan cepat.
* **Focus Toggle:** Berpindah fokus antar 2 jendela secara instan.
* **Static Configuration:** Semua pengaturan dilakukan melalui `config.h`.

## Instalasi

### Dependensi
Anda membutuhkan header `xcb`, `xcb-keysyms`, dan `X11` terpasang di sistem Anda (misalnya `libxcb` di Arch Linux).

### Kompilasi
```bash
git clone https://github.com/username/uwm.git
cd uwm
make
sudo make install
```

## Konfigurasi
Edit file `config.h` untuk menyesuaikan shortcut dan aplikasi default. Setelah perubahan, kompilasi ulang `uwm`.

## Menjalankan
Tambahkan baris berikut ke file `.xinitrc` Anda:
```bash
exec uwm
```
