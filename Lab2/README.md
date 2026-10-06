# ЛР2. Прозрачное шифрование дискового ввода/вывода драйвером ядра ОС

**Цель:** ознакомление с приёмами использования модулей ядра ОС для защиты
автоматизированных систем.

## Описание

Минифильтр-драйвер файловой системы Windows, который «прозрачно» шифрует и
расшифровывает содержимое файлов с заданным расширением (`.lab2ext`). При записи
(`IRP_MJ_WRITE`) буфер пользователя шифруется по алгоритму **AES-256-CBC** до
попадания на диск, при чтении (`IRP_MJ_READ`) — расшифровывается уже после чтения
с диска. Таким образом приложение, работающее через стандартные `ReadFile`/
`WriteFile` (обёрнутые в `fopen`/`fread`/`fwrite`), видит открытый текст, а на
диске и для сторонних программ (например, `notepad.exe`) содержимое остаётся
зашифрованным. Файлы с другими расширениями проходят через фильтр без изменений.

Драйвер построен на основе официального образца Microsoft
[`passThrough`](https://github.com/microsoft/Windows-driver-samples/tree/main/filesys/miniFilter/passThrough),
в который добавлены ветки шифрования/расшифровки в функциях
`PtPreOperationPassThrough()` и `PtPostOperationPassThrough()`. Криптография —
header-only библиотека [tiny-AES-c](https://github.com/kokke/tiny-AES-c)
(файлы `aes.c`/`aes.h`), в которой включён режим `AES256`.

### Что изменено относительно образца passThrough

| Файл | Изменение |
|------|-----------|
| `aes.h` | включён `#define AES256 1` (ключ 32 байта) вместо `AES128` |
| `passThrough.c` | `#include "aes.h"`, константный ключ/IV, функции `Lab2IsTargetFile()` и `Lab2CryptBuffer()` |
| `passThrough.c` → `PtPreOperationPassThrough()` | ветка `IRP_MJ_WRITE`: шифрование `Parameters.Write.WriteBuffer` |
| `passThrough.c` → `PtPostOperationPassThrough()` | ветка `IRP_MJ_READ`: расшифровка `Parameters.Read.ReadBuffer`; закомментирован `UNREFERENCED_PARAMETER(Data)` |
| `passThrough.inf` | класс `Encryption`, `Altitude = 147000` (диапазон FSFilter Encryption 140000–149999), группа загрузки `FSFilter Encryption` |
| `passThrough.vcxproj` | в сборку добавлен `aes.c` |

### Допущения и условности лабораторной реализации

- постоянный ключ и IV зашиты в коде драйвера (в доп. задании ключ передаётся из
  клиентского приложения через порт связи minifilter);
- работа только с буфером фиксированного размера, кратным 16 байтам (без паддинга);
- обрабатываются только `ReadFile`/`WriteFile`; memory-mapped files (notepad и др.)
  не поддерживаются;
- возможное двойное срабатывание `IRP_MJ_READ`/`IRP_MJ_WRITE` не фильтруется.

---

## Предварительные требования

| Компонент | Версия | Примечание |
|-----------|--------|------------|
| Visual Studio | Community 2022 | с комплектом разработки C++ |
| WDK + SDK | одинаковый номер версии! | Windows Driver Kit, см. методичку |
| Виртуальная машина | Hyper-V / VirtualBox | гостевая Windows 10 x64 для тестов |
| WinDbg / DebugView | опционально | отладка и просмотр `DbgPrint` |

> **Важно:** версия SDK и WDK должны полностью совпадать по номеру. Драйвер
> нельзя безопасно тестировать на рабочей машине — только в виртуальной машине с
> включённой тестовой подписью (`bcdedit /set testsigning on`).

---

## Сборка

### Драйвер (на рабочей машине с VS + WDK)

Проект `PassThroughCrypt/passThrough.vcxproj` открывается в Visual Studio 2022.
Конфигурация — `Debug` / `x64`. Результат сборки — три файла в каталоге вывода:

- `passThrough.sys` — сам драйвер,
- `passThrough.inf` — файл установки,
- `passThrough.cat` — каталог подписи.

> Если проект не открывается (несовпадение версии WDK), создайте новый проект из
> шаблона **«Filter Driver: Filesystem Mini-Filter»** и добавьте в него файлы
> `passThrough.c`, `aes.c`, `aes.h`, `passThrough.inf`, `passThrough.rc`.
> При ошибках сборки отключите библиотеки Spectre/Meltdown в свойствах проекта
> (C/C++ → Code Generation → Spectre Mitigation → Disabled).

### Тестовое приложение (пользовательский уровень)

```bat
cd Lab2\TestApp
cmake -S . -B build && cmake --build build --config Release
```

или из *x64 Native Tools Command Prompt*:

```bat
cl /W4 test_file_io.c
```

---

## Развёртывание и проверка (на тестовой ВМ)

Соберите `passThrough.sys`, подпишите его тестовым сертификатом и сгенерируйте
каталог (`.cat`) на рабочей машине (`signtool`, `stampinf`, `Inf2Cat` из WDK —
подробная команда ниже), либо перенесите уже собранные `passThrough.sys` /
`passThrough.inf` в ВМ и подпишите прямо там. Перенесите на тестовую ВМ
`passThrough.sys`, `.inf`, `.cat`, тестовый сертификат (`Lab2TestCert.cer`),
собранный `test_file_io.exe` и скрипты из каталога `scripts/`.

```bat
1_setup_signing.bat    REM импорт тестового сертификата + bcdedit testsigning on, затем reboot
2_load_and_demo.bat    REM после перезагрузки: регистрация службы, fltmc load, демонстрация
```

`2_load_and_demo.bat` сам регистрирует службу минифильтра (через `sc.exe` и
ключи `Instances\...\Altitude` в реестре — это надёжнее, чем ПКМ → «Установить»,
который у части версий Windows не регистрирует службу фильтра), загружает
драйвер и прогоняет полный цикл: запись → чтение с драйвером (открытый текст) →
выгрузка → чтение без драйвера (сырой шифртекст на диске).

Этот сценарий **проверен вживую** на Hyper-V (Windows 10, testsigning on):

```
С драйвером:     48 65 6C 6C 6F 2C 20 4C 61 62 32 20 ...   ("Hello, Lab2 ...")
Без драйвера:    01 26 13 DB 99 2F 1D A2 7C 7D D9 D7 ...   (шифртекст AES на диске)
```

Проверка наличия драйвера в стеке:

```bat
fltmc
```

Отладочные сообщения (`*** Lab2: IRP_MJ_WRITE matched ...`) смотрите в **DebugView**,
запущенном внутри ВМ с опцией *Capture → Capture Kernel*.

### Подпись драйвера тестовым сертификатом (кратко)

```bat
REM 1. Тестовый сертификат для подписи кода
powershell -Command "New-SelfSignedCertificate -Type CodeSigningCert -Subject 'CN=Lab2 Test Cert' -CertStoreLocation Cert:\CurrentUser\My"

REM 2. Подпись .sys и .inf -> .cat (замените <thumbprint> на отпечаток из шага 1)
signtool sign /fd sha256 /sha1 <thumbprint> passThrough.sys
stampinf -f passThrough.inf -d "*" -v 1.0.0.0
Inf2Cat /driver:. /os:10_X64
signtool sign /fd sha256 /sha1 <thumbprint> passthrough.cat

REM 3. Экспорт сертификата для переноса в ВМ
powershell -Command "Export-Certificate -Cert Cert:\CurrentUser\My\<thumbprint> -FilePath Lab2TestCert.cer"
```

### Чек-лист демонстрации

1. Создать файл `secret.lab2ext`, показать его открытое содержимое **до** драйвера.
2. Установить и загрузить драйвер.
3. Прочитать файл при загруженном драйвере — содержимое нечитаемо (на диске ещё
   открытый текст, а драйвер его «расшифровывает»).
4. Перезаписать файл — теперь на диске шифртекст, драйвер расшифровывает при
   чтении, и приложение видит открытый текст.
5. Открыть `secret.lab2ext` в `notepad.exe` — содержимое нечитаемо.
6. Показать срабатывание точек останова / печать в DebugView для веток
   `IRP_MJ_READ` и `IRP_MJ_WRITE`.
7. Выгрузить драйвер — файл снова читается как шифртекст (доказательство, что
   данные на диске действительно зашифрованы).

---

## Структура файлов

```
Lab2/
├── README.md
├── theory_answers.md             — ответы на контрольные вопросы
├── PassThroughCrypt/             — проект драйвера
│   ├── passThrough.c             — драйвер (базовый passThrough + ветки AES)
│   ├── passThrough.inf           — установка (класс Encryption, Altitude 147000)
│   ├── passThrough.rc            — ресурсы версии
│   ├── passThrough.vcxproj       — проект Visual Studio (WDK)
│   ├── aes.c / aes.h             — tiny-AES-c (режим AES-256)
│   └── aes_LICENSE.txt           — лицензия tiny-AES-c (Unlicense)
├── TestApp/                      — тестовое приложение пользовательского уровня
│   ├── test_file_io.c            — запись/чтение файла фиксированного размера
│   └── CMakeLists.txt
└── scripts/                      — автоматизация развёртывания в тестовой ВМ
    ├── 1_setup_signing.bat        — импорт тестового сертификата + testsigning on
    ├── 2_load_and_demo.bat        — регистрация службы, fltmc load, демонстрация
    └── Lab2TestCert.cer           — тестовый сертификат для подписи драйвера
```

## Скриншоты

*Скриншот 1: вывод `fltmc` со загруженным драйвером PassThrough*

![fltmc](screenshots/fltmc_loaded.png)

*Скриншот 2: DebugView — срабатывание веток IRP_MJ_WRITE/IRP_MJ_READ*

![DebugView](screenshots/debugview.png)
