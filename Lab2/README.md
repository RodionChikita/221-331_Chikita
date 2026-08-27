# Лабораторная работа №2

Тема: защита автоматизированной системы на уровне ядра Windows; прозрачное шифрование дискового ввода-вывода минифильтр-драйвером.

Проект содержит минифильтр на основе официального образца Microsoft PassThrough, реализацию AES-256-CBC из tiny-AES-c, консольный WinAPI-клиент и команды установки, загрузки и выгрузки. Драйвер обрабатывает только файлы с расширением `.lab2ext`: перед записью шифрует буфер, после чтения расшифровывает его. Остальные файлы проходят без изменений. Учебная реализация использует постоянные ключ и IV и работает с длинами, кратными блоку AES (16 байт); клиент записывает ровно 256 байт.

## Структура

- `driver/` — Visual Studio/WDK-проект минифильтра.
- `client/` — исходный код программы, вызывающей `ReadFile` и `WriteFile`.
- `scripts/` — установка, загрузка и выгрузка драйвера из командной строки администратора.
- `test-data/` — исходный открытый файл для демонстрации.

## Требования и безопасность

Нужны Visual Studio 2022 с C++, Windows SDK, совместимый WDK, WinDbg, DebugView и тестовая Windows VM с test signing. Не запускайте учебный драйвер на основной машине: ошибка режима ядра может вызвать BSOD и потерю данных. Работайте в VM со снимком состояния.

## Сборка

Откройте `driver/passThrough.sln`, выберите `Debug | x64` и выполните Build. Должны появиться `passThrough.sys`, `passThrough.inf` и `passThrough.cat`.

Клиент собирается в Developer Command Prompt:

```bat
cmake -S client -B build-client -A x64
cmake --build build-client --config Release
```

## Установка и запуск

В командной строке администратора тестовой VM:

```bat
scripts\install-driver.bat driver\x64\Debug\passThrough\passThrough.inf
scripts\load-driver.bat
build-client\Release\lab2_client.exe read test-data\sample.lab2ext
build-client\Release\lab2_client.exe write test-data\sample.lab2ext "LR2 transparent encryption works"
build-client\Release\lab2_client.exe read test-data\sample.lab2ext
scripts\unload-driver.bat
```

После первой загрузки чтение исходного открытого файла выглядит испорченным: фильтр пытается расшифровать plaintext. После `write` клиент читает понятный текст, а Notepad показывает шифротекст.

## Чек-лист защиты

1. Показать открытый `sample.lab2ext` до загрузки.
2. Показать установку и PassThrough в выводе `fltmc`.
3. Показать нечитаемое первое чтение открытого файла при активном драйвере.
4. Записать 256 байт через `WriteFile`; показать читаемый результат в клиенте и шифротекст в Notepad.
5. Показать ветки `IRP_MJ_WRITE` и `IRP_MJ_READ` в DebugView/WinDbg.
6. Выгрузить фильтр и повторить проверку.

## Ограничения

Ключ и IV зашиты в драйвер; нет аутентификации шифротекста и управления ключами; размер операции должен быть кратен 16; не поддержаны частичные, paging и memory-mapped операции; один IV переиспользуется; изменяются исходные I/O-буферы. Для промышленного решения нужны swap-buffers, контексты файлов, случайные IV, AEAD, защищённое хранение ключей и корректная обработка offset/EOF/cached I/O.
