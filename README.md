# 221-331 Chikita — Лабораторные работы

Репозиторий лабораторных работ по дисциплине «Защита автоматизированных систем».
Группа 221-331, студент: Чикита.

---

## ЛР1. Менеджер паролей с защитой от утечки данных, отладки и модификации

**Цель:** получение навыков реализации технических методов защиты в ПО пользовательского уровня.

### Описание

Приложение представляет собой менеджер паролей для безопасного хранения учётных данных.
Реализовано на C++/Qt 6.11.0 (MinGW) с использованием библиотеки OpenSSL 3.x для криптографических операций.

#### Функциональность

- **Аутентификация**: окно ввода пин-кода (мастер-пароля) для разблокировки хранилища
- **Отображение учётных данных**: таблица с фильтрацией по URL, логины и пароли замаскированы
- **AES-256 шифрование файла**: файл учётных данных зашифрован алгоритмом AES-256-CBC; ключ выводится из пин-кода через PBKDF2 (SHA-256, 100 000 итераций)
- **Двухслойное шифрование**: после расшифровки файла URL доступен для фильтрации, а логин/пароль остаются зашифрованы вторым ключом и расшифровываются только по запросу пользователя
- **Антиотладка (IsDebuggerPresent)**: при обнаружении отладчика — блокировка и предупреждение
- **Антиотладка (самоотладка)**: приложение-спутник (Protector) подключается к менеджеру паролей как отладчик через DebugActiveProcess, блокируя подключение сторонних отладчиков
- **Проверка целостности**: при запуске вычисляется SHA-256 хеш сегмента .text и сравнивается с эталоном

---

### Предварительные требования

| Компонент | Версия | Примечание |
|-----------|--------|------------|
| Windows | 10/11 x64 | Обязательно — WinAPI используется для антиотладки и PE-заголовков |
| Qt | 6.11.0 | Установка через Qt Online Installer, выбрать компонент MinGW 64-bit |
| CMake | 3.16+ | Устанавливается вместе с Qt (Qt Tools → CMake) или отдельно |
| OpenSSL | 3.x | Устанавливается вместе с Qt (Qt Tools → OpenSSL) или через winget/slproweb |
| MinGW | (из Qt) | Устанавливается вместе с Qt (Qt Tools → MinGW) |
| Python | 3.8+ | Для утилиты шифрования файла данных |

---

### Быстрый старт (одна команда)

Откройте **обычный cmd.exe** (или PowerShell) и выполните:

```bat
cd 221-331_Chikita\Lab1
build_all.bat
```

Скрипт автоматически:
1. Найдёт Qt и OpenSSL на вашем диске
2. Проверит наличие CMake и компилятора (MinGW или MSVC)
3. Установит Python-зависимость и зашифрует файл данных (пин-код `1234`)
4. Запустит CMake configure + build для обоих проектов
5. Скопирует все exe, DLL и данные в папку `Lab1\dist\`

После завершения:
```bat
cd dist
Lab1_PassManager.exe
```

Если Qt или OpenSSL установлены в нестандартные пути, задайте их перед запуском:
```bat
set QTDIR=D:\MyQt\6.11.0\mingw_64
set OPENSSL_DIR=D:\OpenSSL\Win_x64
build_all.bat
```

Для очистки всех артефактов сборки:
```bat
clean.bat
```

---

### Ручная сборка (пошагово)

<details>
<summary>Если вы хотите собрать вручную без build_all.bat</summary>

#### Шаг 1. Настройка переменных окружения

Откройте **cmd.exe** и выполните:

```bat
set QTDIR=C:\Qt\6.11.0\mingw_64
set PATH=%QTDIR%\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%
set OPENSSL_DIR=C:\OpenSSL-Win64
```

Путь `C:\Qt\Tools\mingw1310_64` может отличаться — посмотрите какая папка `mingw*` есть в `C:\Qt\Tools\`.

#### Шаг 2. Подготовка зашифрованного файла учётных данных

```bat
cd Lab1\Lab1_PassManager
pip install cryptography
python encrypt_credentials.py 1234
```

#### Шаг 3. Сборка через CMake

```bat
cd Lab1
mkdir build
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QTDIR% -DOPENSSL_ROOT_DIR=%OPENSSL_DIR%
cmake --build build --config Release
```

Для MSVC (из x64 Native Tools Command Prompt):
```bat
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QTDIR% -DOPENSSL_ROOT_DIR=%OPENSSL_DIR%
cmake --build build --config Release
```

#### Шаг 4. Развёртывание

```bat
mkdir dist
copy /Y build\Lab1_PassManager\Lab1_PassManager.exe dist\
copy /Y build\Lab1_Protector\Lab1_Protector.exe dist\
copy /Y Lab1_PassManager\credentials.json.enc dist\
%QTDIR%\bin\windeployqt.exe --release dist\Lab1_PassManager.exe
copy /Y "%OPENSSL_DIR%\bin\libcrypto-3-x64.dll" dist\
copy /Y "%OPENSSL_DIR%\bin\libssl-3-x64.dll" dist\
```

#### Шаг 5. Запуск

```bat
cd dist
Lab1_PassManager.exe
```

</details>

---

### Проверка базовой работы приложения

```bat
cd dist
Lab1_PassManager.exe
```

1. Откроется окно **"Password Manager — Вход"** с полем ввода пин-кода
2. Введите `1234` (или тот пин-код, который вы использовали при шифровании) и нажмите **"Войти"**
3. При верном пин-коде откроется окно **"Password Manager — Учётные данные"** с таблицей
4. В таблице будут видны URL-адреса сайтов, а логины и пароли отображены как `*****`
5. Введите текст в поле **"Фильтр по URL..."** — таблица отфильтруется в реальном времени
6. Выберите строку в таблице и нажмите **"Скопировать логин"** или **"Скопировать пароль"**
7. Появится диалог повторного ввода пин-кода — введите тот же пин
8. После успешной расшифровки данные будут скопированы в буфер обмена

**Проверка неверного пин-кода:** введите любой неверный пин-код — появится сообщение "Неверный пин-код или файл повреждён."

---

### Шаг 7. Проверка защиты от отладки (IsDebuggerPresent)

1. Откройте файл `main.cpp` и **раскомментируйте** блок на строках 30-38:
```cpp
if (AntiDebug::isDebuggerAttached()) {
    loginWindow.show();
    loginWindow.showAttackWarning(
        "ВНИМАНИЕ: Обнаружен отладчик!\n"
        "Работа приложения заблокирована.");
    return app.exec();
}
```
2. Пересоберите проект (`cmake --build build --config Release`) и повторите deploy
3. Запустите `Lab1_PassManager.exe` из проводника — приложение работает нормально
4. Запустите `Lab1_PassManager.exe` из отладчика (x64dbg или Qt Creator в режиме Debug) — увидите красное предупреждение, ввод пин-кода будет заблокирован
5. После демонстрации **закомментируйте** блок обратно (он мешает работе Protector)

---

### Шаг 8. Проверка защиты самоотладкой (Protector)

1. Убедитесь, что блок `IsDebuggerPresent` в `main.cpp` **закомментирован**
2. Скопируйте `Lab1_Protector.exe` в ту же папку `release\`, где лежит `Lab1_PassManager.exe`
3. Запустите Protector из командной строки:
```bat
cd release
Lab1_Protector.exe
```
4. В консоли Protector появятся сообщения:
```
[Protector] Starting: ...\Lab1_PassManager.exe
[Protector] CreateProcessW() success, PID = XXXXX
[Protector] DebugActiveProcess() success — attached as debugger
```
5. Одновременно откроется окно менеджера паролей — работает нормально
6. Теперь попробуйте подключиться к этому процессу менеджера паролей из **x64dbg** (File → Attach → выберите PID менеджера) — отладчик **не сможет** подключиться, т.к. слот отладчика уже занят Protector'ом
7. Закройте менеджер паролей — в консоли Protector появится:
```
[Protector] Password manager exited (code 0)
```

---

### Шаг 9. Проверка защиты от модификации (.text hash)

1. Соберите release-версию и запустите `Lab1_PassManager.exe`
2. В отладочной консоли Qt Creator (или в stderr) появится вывод:
```
textBase = 0x7ff6XXXXXXXX
textSize = XXXXX
calculatedTextHashBase64 = "ваш_хеш_base64="
IntegrityCheck: reference hash not set, skipping check
```
3. Скопируйте значение `calculatedTextHashBase64` (без кавычек)
4. Откройте файл `integritycheck.cpp` и замените строку:
```cpp
static const QByteArray REFERENCE_HASH_BASE64 =
    QByteArray("PLACEHOLDER_HASH_UPDATE_AFTER_BUILD");
```
на:
```cpp
static const QByteArray REFERENCE_HASH_BASE64 =
    QByteArray("скопированный_хеш_base64=");
```
5. Пересоберите проект и повторите deploy
6. Запустите — в консоли должно быть `IntegrityCheck result = true`, приложение работает
7. Теперь откройте `Lab1_PassManager.exe` в hex-редакторе и измените любой байт в сегменте `.text` (или используйте x64dbg для патча)
8. Запустите изменённый exe — увидите красное предупреждение: "Обнаружена модификация исполняемого файла!"

**Важно:** хеш `.text` сегмента меняется при каждой перекомпиляции. После каждого изменения кода нужно повторить процедуру (шаги 2-6). Рекомендуется вписывать хеш самым последним, когда весь остальной код стабилен.

---

### Шаг 10. Оформление в Git

Все операции выполняются из командной строки:

```bat
cd 221-331_Chikita
git checkout -b Lab1
git add .
git commit -m "Lab1: Password Manager with AES-256, two-layer encryption, anti-debug and integrity check"
git push -u origin Lab1
```

После завершения работы — слияние в main:

```bat
git checkout main
git merge Lab1
git push origin main
```

---

### Структура файлов

```
Lab1/
├── CMakeLists.txt                 — корневой CMake (подключает подпроекты)
├── build_all.bat                  — ПОЛНАЯ сборка одной командой
├── clean.bat                      — очистка артефактов сборки
├── build/                         — каталог сборки CMake (создаётся автоматически)
├── dist/                          — готовые к запуску файлы (создаётся build_all.bat)
├── Lab1_PassManager/
│   ├── CMakeLists.txt             — CMake проект менеджера паролей
│   ├── main.cpp                   — точка входа, связка всех модулей
│   ├── loginwindow.h / .cpp       — окно аутентификации (пин-код)
│   ├── credentialswindow.h / .cpp — окно учётных данных (таблица, фильтр, копирование)
│   ├── cryptoutils.h / .cpp       — AES-256 шифрование/расшифрование (OpenSSL, 2 слоя)
│   ├── antidebug.h / .cpp         — IsDebuggerPresent()
│   ├── integritycheck.h / .cpp    — SHA-256 хеш сегмента .text
│   ├── credentials_plain.json     — исходные данные (НЕ коммитится!)
│   ├── credentials.json.enc       — зашифрованный файл данных
│   └── encrypt_credentials.py     — утилита шифрования
└── Lab1_Protector/
    ├── CMakeLists.txt             — CMake проект Protector (консольное)
    └── main.cpp                   — CreateProcessW + DebugActiveProcess
```

### Скриншоты

*Скриншот 1: Окно аутентификации*

![Окно входа](screenshots/login_window.png)

*Скриншот 2: Окно учётных данных*

![Учётные данные](screenshots/credentials_window.png)
