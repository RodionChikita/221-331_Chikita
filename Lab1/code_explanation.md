# ЛР1 — Описание исходного кода

## Общая архитектура

Проект состоит из двух приложений:
- **Lab1_PassManager** — менеджер паролей (GUI-приложение на Qt)
- **Lab1_Protector** — приложение-спутник для антиотладочной защиты (консольное)

Также есть вспомогательный Python-скрипт `encrypt_credentials.py` для подготовки зашифрованного файла данных.

### Схема взаимодействия модулей

```
main.cpp  ──────────────────────────────────────────────────────
  │                                                             
  ├─→ antidebug.h/.cpp         Проверка: есть ли отладчик?     
  ├─→ integritycheck.h/.cpp    Проверка: не изменён ли .exe?   
  ├─→ loginwindow.h/.cpp       Окно ввода пин-кода             
  ├─→ credentialswindow.h/.cpp Окно с таблицей паролей         
  │     └─→ cryptoutils.h/.cpp   Расшифровка отдельных полей   
  └─→ cryptoutils.h/.cpp       Расшифровка файла               
```

---

## main.cpp — Точка входа и координатор

**Назначение:** связывает все модули в единое приложение, управляет порядком проверок безопасности и переключением между окнами.

### Построчный разбор:

**Строки 1-13 — Подключение заголовков:**
```cpp
#include <QApplication>        // Класс Qt-приложения, управляет циклом событий
#include <QJsonDocument>       // Парсинг JSON
#include <QJsonArray>          // Работа с JSON-массивами
#include <QJsonObject>         // Работа с JSON-объектами
#include <QMessageBox>         // Диалоговые окна с сообщениями
#include <QDir>                // Работа с каталогами ФС
#include <QDebug>              // Отладочный вывод (qDebug)

#include "loginwindow.h"       // Окно аутентификации
#include "credentialswindow.h" // Окно учётных данных
#include "cryptoutils.h"       // Модуль шифрования/расшифрования
#include "antidebug.h"         // Модуль антиотладки
#include "integritycheck.h"    // Модуль проверки целостности
```

**Строки 15-18 — Путь к зашифрованному файлу:**
```cpp
static QString encryptedFilePath()
{
    return QApplication::applicationDirPath() + "/credentials.json.enc";
}
```
Возвращает полный путь к файлу `credentials.json.enc`, который лежит рядом с exe. `applicationDirPath()` возвращает каталог, в котором находится исполняемый файл.

**Строки 20-22 — Создание приложения:**
```cpp
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
```
`QApplication` — объект Qt-приложения. Управляет циклом обработки событий (нажатия кнопок, перерисовка окон и т.д.). Без него GUI не работает.

**Строки 24-25 — Создание окон:**
```cpp
    LoginWindow loginWindow;
    CredentialsWindow credentialsWindow;
```
Создаются оба окна, но ни одно пока не показано. Они живут в стеке `main()` до завершения программы.

**Строки 30-38 — Антиотладка (IsDebuggerPresent):**
```cpp
    /*
    if (AntiDebug::isDebuggerAttached()) {
        loginWindow.show();
        loginWindow.showAttackWarning(
            "ВНИМАНИЕ: Обнаружен отладчик!\n"
            "Работа приложения заблокирована.");
        return app.exec();
    }
    */
```
Закомментированный блок. При раскомментировании: если обнаружен отладчик — показывается окно с красным предупреждением, поле ввода PIN и кнопка «Войти» блокируются. Приложение продолжает работать (цикл событий запущен), но пользоваться им невозможно.

**Строки 41-48 — Проверка целостности:**
```cpp
    if (!IntegrityCheck::verify()) {
        loginWindow.show();
        loginWindow.showAttackWarning(
            "ВНИМАНИЕ: Обнаружена модификация исполняемого файла!\n"
            "Контрольная сумма сегмента .text не совпадает с эталоном.\n"
            "Работа приложения заблокирована.");
        return app.exec();
    }
```
Вызывает `IntegrityCheck::verify()` — та вычисляет SHA-256 хеш сегмента `.text` и сравнивает с эталоном. Если хеш не совпадает (exe был изменён) — блокировка. Если эталон ещё не задан (placeholder) — проверка пропускается.

**Строка 51 — Показ окна входа:**
```cpp
    loginWindow.show();
```
Окно входа становится видимым.

**Строки 53-94 — Обработчик ввода PIN:**
```cpp
    QObject::connect(&loginWindow, &LoginWindow::pinAccepted,
                     [&](const QString &pin) {
```
Когда пользователь нажимает «Войти» и LoginWindow генерирует сигнал `pinAccepted(pin)`, выполняется лямбда-функция:

1. **Строка 57** — `CryptoUtils::decryptCredentialsFile(filePath, pin)` — расшифровка файла первым слоем (AES-256-CBC, ключ из PIN через PBKDF2). Если PIN неверный — расшифровка даёт мусор и возвращается пустой QByteArray.

2. **Строки 64-70** — `QJsonDocument::fromJson(decryptedJson)` — попытка распарсить расшифрованные данные как JSON. Если PIN был неверный — JSON невалиден, показывается ошибка.

3. **Строки 72-89** — Парсинг JSON-массива в вектор структур `Credential`. Для каждого элемента:
   - `url` — читается как обычная строка (открытый текст)
   - `encrypted_login` и `encrypted_password` — читаются как base64-строки и декодируются в бинарные данные. Это зашифрованные вторым слоем данные — они **не расшифровываются** на этом этапе.

4. **Строки 91-93** — Передача данных в окно учётных данных, скрытие окна входа, показ окна с таблицей.

**Строки 96-100 — Обработчик выхода:**
```cpp
    QObject::connect(&credentialsWindow, &CredentialsWindow::logoutRequested,
                     [&]() {
        credentialsWindow.hide();
        loginWindow.show();
    });
```
При нажатии кнопки «Выход» в окне учётных данных — возврат к окну входа.

**Строка 102** — `return app.exec();` — запуск бесконечного цикла обработки событий Qt. Программа работает, пока все окна не будут закрыты.

---

## loginwindow.h — Заголовок окна аутентификации

**Назначение:** объявление класса `LoginWindow` — окна для ввода PIN-кода.

### Ключевые элементы:

```cpp
class LoginWindow : public QWidget
{
    Q_OBJECT
```
- Наследуется от `QWidget` — базовый класс всех виджетов Qt.
- `Q_OBJECT` — макрос Qt, необходимый для работы сигналов и слотов. Без него `signals:` и `slots:` не будут работать.

```cpp
signals:
    void pinAccepted(const QString &pin);
```
**Сигнал** — специальный механизм Qt для связи между объектами. Когда пользователь вводит PIN и нажимает «Войти», окно генерирует этот сигнал. Код в `main.cpp` подключен к нему и получает введённый PIN.

```cpp
private:
    QLineEdit *pinEdit_;       // Поле ввода PIN
    QPushButton *loginButton_; // Кнопка «Войти»
    QLabel *warningLabel_;     // Метка для предупреждений (красный текст)
    QLabel *titleLabel_;       // Заголовок «Менеджер паролей»
```
Указатели на виджеты. Создаются в конструкторе, Qt управляет их памятью (удаляет при уничтожении родителя).

---

## loginwindow.cpp — Реализация окна аутентификации

### Конструктор (строки 6-49):

**Строки 8-10** — настройка окна:
```cpp
    setWindowTitle("Password Manager — Вход");
    setMinimumSize(400, 300);
```

**Строки 12-13** — создание вертикальной раскладки с выравниванием по центру:
```cpp
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
```
`QVBoxLayout` располагает виджеты вертикально сверху вниз. `AlignCenter` — центрирование по горизонтали.

**Строки 22-26** — поле ввода PIN:
```cpp
    pinEdit_ = new QLineEdit;
    pinEdit_->setEchoMode(QLineEdit::Password);  // Символы скрыты точками
    pinEdit_->setPlaceholderText("Введите пин-код...");
    pinEdit_->setMaximumWidth(280);
    pinEdit_->setMinimumHeight(36);
```
`setEchoMode(Password)` — вместо символов показываются точки (как в любом поле пароля).

**Строки 38-46** — компоновка с растяжками:
```cpp
    layout->addStretch();                                    // Пружина сверху
    layout->addWidget(titleLabel_, 0, Qt::AlignCenter);      // Заголовок
    layout->addSpacing(20);                                  // Отступ 20px
    layout->addWidget(pinEdit_, 0, Qt::AlignCenter);         // Поле ввода
    layout->addSpacing(8);
    layout->addWidget(loginButton_, 0, Qt::AlignCenter);     // Кнопка
    layout->addSpacing(12);
    layout->addWidget(warningLabel_, 0, Qt::AlignCenter);    // Предупреждение
    layout->addStretch();                                    // Пружина снизу
```
`addStretch()` — упругий элемент. Две пружины (сверху и снизу) прижимают содержимое к центру окна по вертикали. При изменении размера окна пружины растягиваются — контент остаётся по центру.

**Строки 48-49** — подключение сигналов:
```cpp
    connect(loginButton_, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(pinEdit_, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
```
Кнопка «Войти» и нажатие Enter в поле ввода — оба вызывают `onLoginClicked()`.

### showAttackWarning (строки 52-58):
```cpp
void LoginWindow::showAttackWarning(const QString &message)
{
    warningLabel_->setText(message);
    warningLabel_->show();
    pinEdit_->setEnabled(false);
    loginButton_->setEnabled(false);
}
```
Показывает красный текст предупреждения и **блокирует** поле ввода и кнопку (setEnabled(false) делает их серыми и некликабельными).

### onLoginClicked (строки 60-69):
```cpp
void LoginWindow::onLoginClicked()
{
    QString pin = pinEdit_->text().trimmed();
    if (pin.isEmpty()) {
        warningLabel_->setText("Введите пин-код");
        warningLabel_->show();
        return;
    }
    emit pinAccepted(pin);
}
```
Берёт текст из поля ввода, обрезает пробелы. Если пусто — показывает предупреждение. Иначе — генерирует сигнал `pinAccepted` с введённым PIN. Слово `emit` — макрос Qt, обозначающий генерацию сигнала.

---

## credentialswindow.h — Заголовок окна учётных данных

### Структура Credential (строки 10-14):
```cpp
struct Credential {
    QString url;                    // URL сайта — в открытом виде
    QByteArray encryptedLogin;      // Логин — зашифрован вторым слоем
    QByteArray encryptedPassword;   // Пароль — зашифрован вторым слоем
};
```
Каждая запись хранит URL открыто (для фильтрации), а логин/пароль остаются зашифрованными в оперативной памяти.

### Сигналы и слоты:
- `logoutRequested()` — сигнал, генерируется при нажатии кнопки «Выход»
- `onFilterChanged(text)` — слот, вызывается при вводе текста в поле фильтра
- `onCopyLogin()` / `onCopyPassword()` — слоты для кнопок копирования

---

## credentialswindow.cpp — Реализация окна учётных данных

### secureErase (строки 16-24):
```cpp
static void secureErase(QByteArray &data)
{
#ifdef Q_OS_WIN
    SecureZeroMemory(data.data(), data.size());
#else
    memset(data.data(), 0, data.size());
#endif
    data.clear();
}
```
Безопасная очистка данных из памяти. `SecureZeroMemory` — функция WinAPI, которая гарантированно не будет оптимизирована компилятором (в отличие от обычного `memset`, который компилятор может удалить, если считает данные «неиспользуемыми»). Вызывается после копирования расшифрованного пароля в буфер обмена, чтобы пароль не оставался в памяти процесса.

### Конструктор (строки 26-63):

**Строки 34-36** — поле фильтрации:
```cpp
    filterEdit_ = new QLineEdit;
    filterEdit_->setPlaceholderText("Фильтр по URL...");
    mainLayout->addWidget(filterEdit_);
```
Текстовое поле в верхней части окна. При вводе текста — таблица фильтруется.

**Строки 38-46** — таблица учётных данных:
```cpp
    table_ = new QTableWidget(0, 3);                                     // 0 строк, 3 колонки
    table_->setHorizontalHeaderLabels({"URL", "Логин", "Пароль"});       // Заголовки
    table_->horizontalHeader()->setStretchLastSection(true);             // Последняя колонка растягивается
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // Все колонки растягиваются
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);          // Запрет редактирования
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);         // Выделение целых строк
    table_->setSelectionMode(QAbstractItemView::SingleSelection);        // Только одна строка
    table_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // Занимает всё место
```
`QTableWidget` — виджет-таблица. `Expanding` в обоих направлениях — таблица растягивается при увеличении окна, что обеспечивает адаптивную раскладку.

### populateTable (строки 76-101):
```cpp
void CredentialsWindow::populateTable(const QString &filter)
{
    table_->setRowCount(0);   // Очистка таблицы

    for (int i = 0; i < credentials_.size(); ++i) {
        const auto &cred = credentials_[i];
        if (!filter.isEmpty() &&
            !cred.url.contains(filter, Qt::CaseInsensitive)) {
            continue;          // Пропускаем записи, не подходящие по фильтру
        }

        int row = table_->rowCount();
        table_->insertRow(row);

        auto *urlItem = new QTableWidgetItem(cred.url);      // URL — виден
        auto *loginItem = new QTableWidgetItem("*****");       // Логин — замаскирован
        auto *passItem = new QTableWidgetItem("*****");        // Пароль — замаскирован

        urlItem->setData(Qt::UserRole, i);  // Сохраняем индекс записи в скрытом поле

        table_->setItem(row, 0, urlItem);
        table_->setItem(row, 1, loginItem);
        table_->setItem(row, 2, passItem);
    }
}
```
Заполняет таблицу. URL показывается как есть, логин и пароль — звёздочками. `Qt::UserRole` — скрытое пользовательское поле у каждой ячейки, в нём храним индекс записи в массиве `credentials_` (он нужен, чтобы после фильтрации найти правильную запись).

### onCopyLogin / onCopyPassword (строки 121-171):
```cpp
void CredentialsWindow::onCopyLogin()
{
    int row = table_->currentRow();                                  // Какая строка выделена
    int credIndex = table_->item(row, 0)->data(Qt::UserRole).toInt(); // Индекс в массиве
    const auto &cred = credentials_[credIndex];

    QString pin;
    if (!requestPin(pin)) return;                                    // Диалог повторного ввода PIN

    QByteArray decrypted = CryptoUtils::decryptField(cred.encryptedLogin, pin); // Расшифровка 2-го слоя
    if (decrypted.isEmpty()) { /* ошибка */ }

    QApplication::clipboard()->setText(QString::fromUtf8(decrypted)); // Копируем в буфер обмена
    secureErase(decrypted);                                           // Стираем из памяти
}
```
Полный цикл для копирования логина:
1. Определяем выделенную строку
2. Запрашиваем повторный ввод PIN через модальный диалог
3. Расшифровываем логин вторым слоем шифрования
4. Копируем в системный буфер обмена
5. Немедленно затираем расшифрованный текст из оперативной памяти

`onCopyPassword` работает аналогично, но расшифровывает `encryptedPassword`.

---

## cryptoutils.h / cryptoutils.cpp — Модуль шифрования

**Назначение:** вся криптография проекта — вывод ключей, шифрование/расшифрование AES-256-CBC через OpenSSL.

### Константы (строки 10-21):
```cpp
static const int KEY_LEN = 32;            // 256 бит = 32 байта
static const int IV_LEN  = 16;            // Размер блока AES = 128 бит = 16 байт
static const int PBKDF2_ITERATIONS = 100000;

static const QByteArray LAYER1_SALT = QByteArray::fromHex("a1b2c3d4e5f60718");
static const QByteArray LAYER2_SALT = QByteArray::fromHex("18070605d4c3b2a1");
static const QByteArray LAYER2_IV = QByteArray::fromHex("00112233445566778899aabbccddeeff");
```
- **LAYER1_SALT** — соль для вывода ключа первого слоя (шифрование файла целиком).
- **LAYER2_SALT** — другая соль для вывода ключа второго слоя (шифрование отдельных полей). Разные соли гарантируют, что из одного PIN получаются **два разных ключа**.
- **LAYER2_IV** — вектор инициализации для второго слоя (фиксированный, т.к. хранится в зашифрованном файле).

### deriveKey (строки 25-42):
```cpp
QByteArray deriveKey(const QString &pin, const QByteArray &salt)
{
    QByteArray key(KEY_LEN, '\0');
    QByteArray pinBytes = pin.toUtf8();

    int rc = PKCS5_PBKDF2_HMAC(pinBytes.constData(), pinBytes.size(),
                                reinterpret_cast<const unsigned char*>(salt.constData()),
                                salt.size(),
                                PBKDF2_ITERATIONS,   // 100 000 итераций
                                EVP_sha256(),         // Хеш-функция SHA-256
                                KEY_LEN,              // Длина выходного ключа — 32 байта
                                reinterpret_cast<unsigned char*>(key.data()));
    ...
}
```
**PBKDF2** (Password-Based Key Derivation Function 2) — стандартная функция вывода ключа из пароля:
- Принимает PIN (например, `"1234"`) и соль
- Выполняет 100 000 итераций HMAC-SHA256
- На выходе — 256-битный ключ

100 000 итераций нужны для замедления перебора: одна попытка PIN занимает ~0.1 сек, полный перебор 4-значного PIN — ~17 минут.

`reinterpret_cast<const unsigned char*>` — приведение типа указателя с `char*` (тип Qt) на `unsigned char*` (тип OpenSSL). Машинная инструкция не генерируется — это только указание компилятору.

### decryptAes256Cbc (строки 44-82):
```cpp
QByteArray decryptAes256Cbc(const QByteArray &ciphertext,
                            const QByteArray &key,
                            const QByteArray &iv)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();      // Создание контекста шифрования
    ...
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), ...);  // Инициализация: алгоритм, ключ, IV
    EVP_DecryptUpdate(ctx, ..., ciphertext, ...);      // Расшифровка основных данных
    EVP_DecryptFinal_ex(ctx, ...);                     // Расшифровка последнего блока + проверка паддинга
    EVP_CIPHER_CTX_free(ctx);                          // Освобождение контекста
    ...
}
```
Три этапа расшифровки через EVP API OpenSSL:
1. **Init** — задаём алгоритм (AES-256-CBC), ключ (32 байта), IV (16 байт)
2. **Update** — расшифровываем данные (может вызываться несколько раз для больших данных)
3. **Final** — обработка последнего блока. Здесь проверяется PKCS#7 паддинг. Если ключ неверный — паддинг будет некорректный, функция вернёт ошибку → возвращаем пустой QByteArray

### decryptCredentialsFile — блочная (масштабируемая) расшифровка:
```cpp
QByteArray decryptCredentialsFile(const QString &filePath, const QString &pin)
{
    static const int CHUNK_SIZE = 4096;

    QFile file(filePath);
    // ... открытие, проверка размера ...

    QByteArray iv = file.read(IV_LEN);             // Первые 16 байт файла = IV
    QByteArray key = deriveKey(pin, LAYER1_SALT);   // Ключ из PIN + соль слоя 1

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);

    QByteArray plaintext;
    QByteArray outBuf(CHUNK_SIZE + EVP_MAX_BLOCK_LENGTH, '\0');
    int outLen = 0;

    while (!file.atEnd()) {                         // Цикл по блокам файла
        QByteArray chunk = file.read(CHUNK_SIZE);   // Читаем 4 КБ
        EVP_DecryptUpdate(ctx, outBuf, &outLen, chunk, chunk.size());
        plaintext.append(outBuf.constData(), outLen);
    }
    file.close();

    EVP_DecryptFinal_ex(ctx, outBuf, &outLen);      // Финальный блок + паддинг
    plaintext.append(outBuf.constData(), outLen);

    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}
```
Формат файла `credentials.json.enc`: `[16 байт IV][шифротекст]`. IV генерируется случайно при шифровании и хранится в начале файла.

Расшифровка выполняется **в цикле по блокам** (по 4 КБ): файл читается порциями, каждая порция передаётся в `EVP_DecryptUpdate`. Это позволяет масштабировать расшифровку на файлы произвольного размера, не загружая весь файл в оперативную память одновременно.

### encryptField / decryptField (строки 150-162):
```cpp
QByteArray encryptField(const QByteArray &plaintext, const QString &pin)
{
    QByteArray key = deriveKey(pin, LAYER2_SALT);  // Ключ из PIN + соль слоя 2
    return encryptAes256Cbc(plaintext, key, LAYER2_IV);
}
```
Второй слой шифрования. Использует **другую соль** (`LAYER2_SALT` вместо `LAYER1_SALT`), поэтому ключ получается другой, хотя PIN тот же.

---

## antidebug.h / antidebug.cpp — Защита от отладки

**Назначение:** обнаружение подключённого отладчика.

```cpp
bool isDebuggerAttached()
{
#ifdef Q_OS_WIN
    return IsDebuggerPresent() != 0;
#else
    return false;
#endif
}
```

`IsDebuggerPresent()` — функция Windows API из `kernel32.dll`. Проверяет поле `BeingDebugged` в PEB (Process Environment Block) текущего процесса. Возвращает `TRUE`, если к процессу подключён отладчик пользовательского уровня (x64dbg, WinDbg, Visual Studio и т.д.).

`#ifdef Q_OS_WIN` — условная компиляция: код внутри компилируется только под Windows. На других ОС функция всегда возвращает `false`.

---

## integritycheck.h / integritycheck.cpp — Проверка целостности

**Назначение:** обнаружение модификации (патча) исполняемого файла.

### calculateTextSegmentHash (строки 19-45):

```cpp
QWORD imageBase = (QWORD)GetModuleHandle(NULL);
```
`GetModuleHandle(NULL)` — возвращает базовый адрес текущего процесса в виртуальной памяти (image base). Это адрес, по которому загрузчик ОС разместил PE-файл.

```cpp
QWORD baseOfCode = 0x1000;
QWORD textBase = imageBase + baseOfCode;
```
Сегмент `.text` обычно начинается со смещения 0x1000 от начала образа (первая страница — заголовки PE).

```cpp
PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(imageBase);
PIMAGE_NT_HEADERS peHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(
    imageBase + dosHeader->e_lfanew);
QWORD textSize = peHeader->OptionalHeader.SizeOfCode;
```
Разбор PE-заголовков для получения размера кодового сегмента:
1. По адресу `imageBase` лежит DOS-заголовок (начинается с "MZ")
2. `dosHeader->e_lfanew` — смещение до NT-заголовка (сигнатура "PE\0\0")
3. `OptionalHeader.SizeOfCode` — суммарный размер всех кодовых секций

```cpp
QByteArray textSegmentContents = QByteArray(
    reinterpret_cast<const char*>(textBase), static_cast<int>(textSize));
QByteArray hash = QCryptographicHash::hash(
    textSegmentContents, QCryptographicHash::Sha256);
QByteArray hashBase64 = hash.toBase64();
```
Считываем блок памяти `.text` как массив байтов, вычисляем SHA-256 хеш, кодируем в base64 для удобства сравнения.

### verify (строки 47-63):
```cpp
bool verify()
{
    QByteArray current = calculateTextSegmentHash();

    if (REFERENCE_HASH_BASE64 == "PLACEHOLDER_HASH_UPDATE_AFTER_BUILD") {
        return true;   // Эталон не задан — пропускаем проверку
    }

    return (current == REFERENCE_HASH_BASE64);
}
```
Сравнивает текущий хеш с эталонным. Если эталон — плейсхолдер, проверка пропускается (для первой сборки).

---

## Lab1_Protector/main.cpp — Приложение-спутник

**Назначение:** запускает менеджер паролей и подключается к нему как отладчик, занимая единственный «слот» отладчика в ОС. После этого никакой другой отладчик не сможет подключиться.

### Определение пути к PassManager (строки 9-26):
```cpp
if (argc > 1) {
    // Путь передан как аргумент командной строки
    MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, ...);
} else {
    // По умолчанию: Lab1_PassManager.exe рядом с Lab1_Protector.exe
    GetModuleFileNameW(NULL, selfPath, MAX_PATH);
    // ... извлекаем каталог и добавляем имя файла
    passManagerPath = dir + L"Lab1_PassManager.exe";
}
```
`GetModuleFileNameW` — получает полный путь к текущему exe. Из него извлекается каталог, и к нему добавляется имя менеджера паролей.

### Запуск процесса (строки 30-51):
```cpp
STARTUPINFOW si;
PROCESS_INFORMATION pi;
ZeroMemory(&si, sizeof(si));
si.cb = sizeof(si);
ZeroMemory(&pi, sizeof(pi));

CreateProcessW(cmdLine.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
```
`CreateProcessW` — создаёт новый процесс (запускает Lab1_PassManager.exe). Структура `PROCESS_INFORMATION` получает PID и handle процесса.

### Подключение как отладчик (строки 54-64):
```cpp
DebugActiveProcess(pi.dwProcessId);
```
`DebugActiveProcess` — подключается к процессу как отладчик через Windows Debug API. После этого:
- Только один отладчик может быть подключён к процессу одновременно
- Любая попытка подключить второй отладчик (x64dbg) будет отклонена ОС

### Цикл обработки отладочных событий (строки 70-103):
```cpp
while (true) {
    WaitForDebugEvent(&debugEvent, INFINITE);  // Ждём событие от ОС

    continueStatus = DBG_CONTINUE;

    switch (debugEvent.dwDebugEventCode) {
    case EXCEPTION_DEBUG_EVENT:
        if (...ExceptionCode != EXCEPTION_BREAKPOINT)
            continueStatus = DBG_EXCEPTION_NOT_HANDLED;
        break;
    case EXIT_PROCESS_DEBUG_EVENT:
        // Менеджер паролей завершился — выходим
        return 0;
    }

    ContinueDebugEvent(..., continueStatus);   // Разрешаем процессу продолжить
}
```
Бесконечный цикл, в котором Protector:
1. Получает отладочные события от ОС (`WaitForDebugEvent`)
2. Для `EXCEPTION_BREAKPOINT` (начальный брейкпоинт при загрузке) — пропускает (`DBG_CONTINUE`)
3. Для других исключений — передаёт обработку процессу (`DBG_EXCEPTION_NOT_HANDLED`)
4. При завершении менеджера паролей — завершается сам

Без этого цикла отлаживаемый процесс «зависнет» — ОС приостанавливает его при каждом отладочном событии, пока отладчик не вызовет `ContinueDebugEvent`.

---

## encrypt_credentials.py — Утилита подготовки данных

**Назначение:** берёт plaintext JSON с паролями и создаёт двухслойно зашифрованный файл `.enc`.

### Процесс шифрования:

**Слой 2 (строки 64-75)** — для каждой записи:
```python
layer2_key = derive_key(pin, LAYER2_SALT)
enc_login = aes_encrypt(entry["login"].encode("utf-8"), layer2_key, LAYER2_IV)
enc_pass  = aes_encrypt(entry["password"].encode("utf-8"), layer2_key, LAYER2_IV)
```
Логин и пароль шифруются AES-256-CBC. URL остаётся открытым. Результат — JSON, где login/password заменены на base64-строки шифротекстов.

**Слой 1 (строки 80-86)** — весь JSON целиком:
```python
layer1_key = derive_key(pin, LAYER1_SALT)
iv = os.urandom(IV_LEN)                     # Случайный IV
ciphertext = aes_encrypt(inner_json, layer1_key, iv)

f.write(iv + ciphertext)                     # Формат файла: [IV][шифротекст]
```
Весь внутренний JSON (с уже зашифрованными полями) шифруется первым слоем.

Соли и параметры PBKDF2 **идентичны** тем, что в C++ коде (`cryptoutils.cpp`), чтобы один и тот же PIN давал одинаковые ключи в Python и C++.

---

## CMakeLists.txt — Система сборки CMake

Проект собирается через CMake (минимум 3.16). Корневой `Lab1/CMakeLists.txt` подключает подпроекты:

```cmake
cmake_minimum_required(VERSION 3.16)
project(Lab1 LANGUAGES CXX)

add_subdirectory(Lab1_PassManager)
add_subdirectory(Lab1_Protector)
```

### Lab1_PassManager/CMakeLists.txt

```cmake
set(CMAKE_CXX_STANDARD 17)       # Стандарт C++17
set(CMAKE_AUTOMOC ON)             # Автоматическая генерация moc-файлов Qt

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)  # Поиск Qt6
find_package(OpenSSL QUIET)                             # Поиск OpenSSL через CMake

add_executable(Lab1_PassManager WIN32 ${SOURCES} ${HEADERS})

target_link_libraries(Lab1_PassManager PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets   # Модули Qt
    OpenSSL::Crypto                    # Библиотека libcrypto (AES, PBKDF2)
    advapi32                           # WinAPI (для антиотладки)
)
```

- `CMAKE_AUTOMOC ON` — CMake автоматически находит заголовки с макросом `Q_OBJECT` и генерирует для них moc-файлы (метаобъектный компилятор Qt).
- `find_package(OpenSSL)` — ищет OpenSSL через CMake модули; если не находит — выполняет ручной поиск по `OPENSSL_ROOT_DIR`.
- `WIN32` в `add_executable` — создаёт Windows GUI приложение (без консольного окна).

### Lab1_Protector/CMakeLists.txt

```cmake
add_executable(Lab1_Protector main.cpp)
target_link_libraries(Lab1_Protector PRIVATE advapi32)
```

Protector — чистый C++ без Qt, линкуется только с `advapi32` для WinAPI-функций.
