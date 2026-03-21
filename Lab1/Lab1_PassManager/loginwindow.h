#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);

    // Show an attack warning and lock the PIN input
    void showAttackWarning(const QString &message);

signals:
    void pinAccepted(const QString &pin);

private slots:
    void onLoginClicked();

private:
    QLineEdit *pinEdit_;
    QPushButton *loginButton_;
    QLabel *warningLabel_;
    QLabel *titleLabel_;
};

#endif // LOGINWINDOW_H
