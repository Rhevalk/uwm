# `uwm` - micro window manager

```
      d8      d8  8b      db      d8  88,dPYba,,adPYba,
     d8'     d8'  `8b    d88b    d8'  88P'   "88"    "8a
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

- **No Bar**: Tidak ada panel, jam, atau workspace indicator.
- **8 Workspaces**: Workspace terbatas untuk menjaga alur kerja tetap fokus dan terstruktur.
- **Hybrid Window System**:
    - Maksimal **2 tiling window per workspace**
    - Maksimal **8 floating window global**
- **Floating Windows**:
    - Bisa drag & resize bebas
    - Tetap terikat ke workspace aktif
- **Focus System**:
    - Hanya 1 window aktif (focus utama)
- **Auto Promotion**:
    - Jika tiling window ditutup, floating tertua otomatis menjadi tiling
- **Mode Switching**:
    - Window bisa dipindah antara tiling ↔ floating secara manual

## Fitur

- **XCB Based:** Tanpa overhead, penggunaan RSS memori sangat rendah (~2MB).
- **Vertical Split:** Otomatis membagi layar secara vertikal jika ada 2 jendela.
- **Workspace Swapping:** Pindahkan seluruh konteks workspace dengan cepat.
- **Focus Toggle:** Berpindah fokus antar 2 jendela secara instan.
- **Static Configuration:** Semua pengaturan dilakukan melalui `config.h`.

## Instalasi

### Dependensi

Anda membutuhkan header `xcb`, `xcb-keysyms`, dan `X11` terpasang di sistem Anda (misalnya `libxcb` di Arch Linux).

### Kompilasi

```bash
git clone <https://github.com/username/uwm.git>
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

## Konfigurasi & Pemetaan Tombol

Semua pengaturan dilakukan melalui `config.h`. Karena setiap hardware memiliki pemetaan tombol fungsi (Fn) yang berbeda, gunakan tool berikut untuk mencari nama `keysym`:

1. Jalankan `xev` di terminal.
2. Tekan tombol yang diinginkan (misal: Volume Up).
3. Cari baris `keysym`, contoh: `(keysym 0x1008ff13, XF86AudioRaiseVolume)`.
4. Masukkan nama tersebut ke dalam `config.h` dengan awalan `XK_` atau sesuai definisi di `X11/XF86keysym.h`.

## Shortcut Default (Standard)

- **Mod + Enter**: Membuka Terminal (`st` atau sesuai `termcmd`).
- **Mod + d**: Menjalankan Menu (`rofi` atau sesuai `menu`).
- **Mod + 1-8**: Pindah Workspace.
- **Mod + Shift + 1-8**: Memindahkan jendela ke Workspace lain.
- **Mod + Tab**: Berpindah fokus selanjutnya antar jendela (Stack).
- **Mod + Shift + Tab**: Berpindah fokus sebelumnya antar jendela (Stack).
- **Mod + Shift + q**: Menutup jendela aktif.
- **Mod + Shift + Escape**: Keluar dari `uwm`.
