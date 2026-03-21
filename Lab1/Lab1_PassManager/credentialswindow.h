#ifndef CREDENTIALSWINDOW_H
#define CREDENTIALSWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QVector>

struct Credential {
    QString url;
    QByteArray encryptedLogin;
    QByteArray encryptedPassword;
};

class CredentialsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CredentialsWindow(QWidget *parent = nullptr);

    void setCredentials(const QVector<Credential> &credentials);

signals:
    void logoutRequested();

private slots:
    void onFilterChanged(const QString &text);
    void onCopyLogin();
    void onCopyPassword();

private:
    void populateTable(const QString &filter = QString());
    bool requestPin(QString &outPin);

    QLineEdit *filterEdit_;
    QTableWidget *table_;
    QPushButton *copyLoginBtn_;
    QPushButton *copyPasswordBtn_;
    QPushButton *logoutBtn_;

    QVector<Credential> credentials_;
};

#endif // CREDENTIALSWINDOW_H
