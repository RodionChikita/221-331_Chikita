#include "credentialswindow.h"
#include "cryptoutils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QClipboard>
#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

static void secureErase(QByteArray &data)
{
#ifdef Q_OS_WIN
    SecureZeroMemory(data.data(), data.size());
#else
    memset(data.data(), 0, data.size());
#endif
    data.clear();
}

CredentialsWindow::CredentialsWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("Password Manager — Учётные данные");
    setMinimumSize(700, 450);

    auto *mainLayout = new QVBoxLayout(this);

    filterEdit_ = new QLineEdit;
    filterEdit_->setPlaceholderText("Фильтр по URL...");
    mainLayout->addWidget(filterEdit_);

    table_ = new QTableWidget(0, 3);
    table_->setHorizontalHeaderLabels({"URL", "Логин", "Пароль"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(table_);

    auto *btnLayout = new QHBoxLayout;
    copyLoginBtn_ = new QPushButton("Скопировать логин");
    copyPasswordBtn_ = new QPushButton("Скопировать пароль");
    logoutBtn_ = new QPushButton("Выход");

    btnLayout->addWidget(copyLoginBtn_);
    btnLayout->addWidget(copyPasswordBtn_);
    btnLayout->addStretch();
    btnLayout->addWidget(logoutBtn_);
    mainLayout->addLayout(btnLayout);

    connect(filterEdit_, &QLineEdit::textChanged, this, &CredentialsWindow::onFilterChanged);
    connect(copyLoginBtn_, &QPushButton::clicked, this, &CredentialsWindow::onCopyLogin);
    connect(copyPasswordBtn_, &QPushButton::clicked, this, &CredentialsWindow::onCopyPassword);
    connect(logoutBtn_, &QPushButton::clicked, this, &CredentialsWindow::logoutRequested);
}

void CredentialsWindow::setCredentials(const QVector<Credential> &credentials)
{
    credentials_ = credentials;
    populateTable();
}

void CredentialsWindow::onFilterChanged(const QString &text)
{
    populateTable(text);
}

void CredentialsWindow::populateTable(const QString &filter)
{
    table_->setRowCount(0);

    for (int i = 0; i < credentials_.size(); ++i) {
        const auto &cred = credentials_[i];
        if (!filter.isEmpty() &&
            !cred.url.contains(filter, Qt::CaseInsensitive)) {
            continue;
        }

        int row = table_->rowCount();
        table_->insertRow(row);

        auto *urlItem = new QTableWidgetItem(cred.url);
        auto *loginItem = new QTableWidgetItem("*****");
        auto *passItem = new QTableWidgetItem("*****");

        // Store the original index in the URL item
        urlItem->setData(Qt::UserRole, i);

        table_->setItem(row, 0, urlItem);
        table_->setItem(row, 1, loginItem);
        table_->setItem(row, 2, passItem);
    }
}

bool CredentialsWindow::requestPin(QString &outPin)
{
    bool ok = false;
    QString pin = QInputDialog::getText(
        this,
        "Подтверждение",
        "Введите пин-код для расшифровки:",
        QLineEdit::Password,
        QString(),
        &ok);

    if (ok && !pin.isEmpty()) {
        outPin = pin;
        return true;
    }
    return false;
}

void CredentialsWindow::onCopyLogin()
{
    int row = table_->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Информация", "Выберите запись в таблице");
        return;
    }

    int credIndex = table_->item(row, 0)->data(Qt::UserRole).toInt();
    const auto &cred = credentials_[credIndex];

    QString pin;
    if (!requestPin(pin)) return;

    QByteArray decrypted = CryptoUtils::decryptField(cred.encryptedLogin, pin);
    if (decrypted.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Неверный пин-код или ошибка расшифровки");
        return;
    }

    QApplication::clipboard()->setText(QString::fromUtf8(decrypted));
    secureErase(decrypted);

    QMessageBox::information(this, "Готово", "Логин скопирован в буфер обмена");
}

void CredentialsWindow::onCopyPassword()
{
    int row = table_->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Информация", "Выберите запись в таблице");
        return;
    }

    int credIndex = table_->item(row, 0)->data(Qt::UserRole).toInt();
    const auto &cred = credentials_[credIndex];

    QString pin;
    if (!requestPin(pin)) return;

    QByteArray decrypted = CryptoUtils::decryptField(cred.encryptedPassword, pin);
    if (decrypted.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Неверный пин-код или ошибка расшифровки");
        return;
    }

    QApplication::clipboard()->setText(QString::fromUtf8(decrypted));
    secureErase(decrypted);

    QMessageBox::information(this, "Готово", "Пароль скопирован в буфер обмена");
}
