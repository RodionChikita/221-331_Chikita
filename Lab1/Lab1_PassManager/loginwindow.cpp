#include "loginwindow.h"

#include <QVBoxLayout>
#include <QFont>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("Password Manager — Вход");
    setMinimumSize(400, 300);

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    titleLabel_ = new QLabel("Менеджер паролей");
    QFont titleFont = titleLabel_->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel_->setFont(titleFont);
    titleLabel_->setAlignment(Qt::AlignCenter);

    pinEdit_ = new QLineEdit;
    pinEdit_->setEchoMode(QLineEdit::Password);
    pinEdit_->setPlaceholderText("Введите пин-код...");
    pinEdit_->setMaximumWidth(280);
    pinEdit_->setMinimumHeight(36);

    loginButton_ = new QPushButton("Войти");
    loginButton_->setMaximumWidth(280);
    loginButton_->setMinimumHeight(36);

    warningLabel_ = new QLabel;
    warningLabel_->setStyleSheet("color: red; font-weight: bold;");
    warningLabel_->setAlignment(Qt::AlignCenter);
    warningLabel_->setWordWrap(true);
    warningLabel_->hide();

    layout->addStretch();
    layout->addWidget(titleLabel_, 0, Qt::AlignCenter);
    layout->addSpacing(20);
    layout->addWidget(pinEdit_, 0, Qt::AlignCenter);
    layout->addSpacing(8);
    layout->addWidget(loginButton_, 0, Qt::AlignCenter);
    layout->addSpacing(12);
    layout->addWidget(warningLabel_, 0, Qt::AlignCenter);
    layout->addStretch();

    connect(loginButton_, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(pinEdit_, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
}

void LoginWindow::showAttackWarning(const QString &message)
{
    warningLabel_->setText(message);
    warningLabel_->show();
    pinEdit_->setEnabled(false);
    loginButton_->setEnabled(false);
}

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
