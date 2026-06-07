# uwm — micro window manager

```
██╗   ██╗██╗    ██╗███╗   ███╗
██║   ██║██║    ██║████╗ ████║
██║   ██║██║ █╗ ██║██╔████╔██║
██║   ██║██║███╗██║██║╚██╔╝██║
╚██████╔╝╚███╔███╔╝██║ ╚═╝ ██║
 ╚═════╝  ╚══╝╚══╝ ╚═╝     ╚═╝ 
```

**uwm** (Micro Window Manager) adalah pengelola jendela extream minimalis untuk X11 yang dibangun menggunakan XCB.

Desain `uwm` berpusat pada sekumpulan operasi dasar yang dapat dikombinasikan secara langsung. Tata letak dan pola penggunaan muncul dari interaksi operasi-operasi tersebut, bukan dari aturan tata letak yang ditentukan sebelumnya.

---

## Filosofi

`uwm` tidak menyediakan bar, widget, mode layout otomatis, atau indikator status visual apa pun. Sistem ini memaksa Anda untuk menggunakan memori organik (otak) untuk mengingat status *workspace* dan jendela. Jika Anda merasa kelelahan, itu adalah indikator alami dari tubuh bahwa Anda perlu segera mengurangi beban kerja.

Karena tidak ada informasi visual mengenai *workspace* dan *client*, Anda perlu memastikan *launcher* pertama dapat bekerja untuk memunculkan aplikasi. Dari sanalah Anda dapat menguji dan merasakan bagaimana `uwm` sebenarnya bekerja.

---

## Konsep Pengelola Jendela

`uwm` tidak memiliki mode pengatur jendela statis (seperti *tiling/floating*), melainkan mengandalkan kumpulan operasi dasar yang dapat menghasilkan banyak perilaku tata letak tanpa perlu dideklarasikan secara eksplisit.

Secara fundamental, `uwm` melakukan *tiling* (memaksimalkan ukuran) pada setiap jendela baru, namun tidak meletakkannya berdampingan secara otomatis, melainkan langsung menumpuknya begitu saja. Sifat ini menciptakan perilaku *monocle* non-eksplisit secara instan. Anda cukup menggunakan fungsi `switch_focus` untuk berselancar di dalam tumpukan (*stack*) jendela tersebut.

Fitur unggulan `uwm` dalam manajemen jendela adalah **Fluid Grid Snapping**. Anda dapat menempatkan jendela secara *vertical split*, *horizontal split*, atau membaginya ke dalam kuadran 1/4 layar secara kumulatif. Mekanisme ini memberikan fleksibilitas *tiling* dinamis yang andal hingga **8 clients** per *workspace*, yang dapat dikombinasikan secara bebas dengan perilaku *monocle*.

---

## Arsitektur Inti

### Workspace Statis

Seluruh data *workspace* dan manajemen *client* disimpan menggunakan struktur data statis berukuran tetap (*fixed-size*). Tidak ada alokasi memori dinamis (`malloc/calloc`) apa pun saat jendela baru dibuat, yang ada hanyalah manipulasi bit murni untuk setiap entitas jendela.
Dengan pendekatan memori statis ini, data tidak perlu pernah dipindahkan atau diatur ulang di dalam memori. Alhasil, kompleksitas waktu dari hampir seluruh operasi di dalam `uwm` mendekati **O(1)** (Konstan).

```c
#define MAX_CLIENTS   8
#define MAX_WORKSPACE 4
```

---

## Sistem Snapping

Penempatan jendela dilakukan melalui sistem snapping berbasis arah.

Arah yang tersedia:

- Kiri
- Kanan
- Atas
- Bawah

Perintah snapping bersifat kumulatif. Setiap tindakan memodifikasi geometri jendela secara bertahap dan dapat diprediksi.

Melalui kombinasi operasi snapping, pengguna dapat membentuk berbagai susunan ruang kerja sesuai kebutuhan tanpa harus berpindah ke konfigurasi atau pengaturan lain.

### Mekanika Kumulatif

Karena `uwm` tidak memiliki mesin pengatur otomatis, Anda membentuk layout secara manual lewat urutan snapping yang logis pada jendela yang sedang fokus:

- **Membuat Vertical Split (2 Jendela Berdampingan):**
    1. Buka jendela pertama (otomatis berukuran penuh).
    2. Tekan `Mod + Left` (jendela pertama menyusut mengisi 1/2 layar kiri).
    3. Buka jendela kedua (otomatis menumpuk berukuran penuh).
    4. Tekan `Mod + Right` (jendela kedua menyusut mengisi 1/2 layar kanan).
- **Membuat Kuadran 1/4 Layar (Pojok Kanan Atas):**
    1. Pada jendela aktif, tekan `Mod + Right` (mengisi 1/2 kanan).
    2. Tekan `Mod + Up` (jendela otomatis menyusut lagi mengisi 1/4 kuadran kanan atas).
    3. Menekan arah berlawanan (`Mod + Down`) akan mengembalikan ukuran jendela ke setengah layar penuh terlebih dahulu sebelum berpindah posisi.

---

## Konfigurasi

Seluruh konfigurasi dilakukan melalui:

```c
config.h
```

Isi dari file `config.h` hanya mengatur Mod Key dan pintasan  untuk menjalankan fungsi fungsi yang di sediakan. Anda juga bisa memanggil aplikasi dari `uwm` dengan bantuan makro `SHCMD(cmd)`  atau dengan memanggilnya langsung jika anda sudah menetapkan alamatnya.

> ⚠️ **Penting:** Pastikan anda membuat pintasan untuk membuka Launcher dan perubahan konfigurasi memerlukan kompilasi ulang
> 

---

## Pintasan Bawaan

| Tombol | Aksi |
| --- | --- |
| Mod + Return | Membuka terminal |
| Mod + d | Membuka menu |
| Mod + Tab | Berpindah ke jendela berikutnya |
| Mod + Shift + q | Menutup jendela aktif |
| Mod + Shift + Escape | Keluar dari uwm |
| Mod + Left | Snap ke kiri |
| Mod + Right | Snap ke kanan |
| Mod + Up | Snap ke atas |
| Mod + Down | Snap ke bawah |
| Mod + 1~4 | Berpindah workspace |
| Mod + Shift + 1~4 | Mengirim jendela ke workspace tujuan |

---

## Instalasi

### Dependensi

Arch Linux:

```bash
sudo pacman -S libxcb xcb-util-keysyms
```

### Kompilasi

```bash
git clone https://github.com/Rhevalk/uwm.git
cd uwm

cp config.def.h config.h

make
sudo make install
```

## Menjalankan

Tambahkan pada `.xinitrc`:

```bash
exec uwm
```
