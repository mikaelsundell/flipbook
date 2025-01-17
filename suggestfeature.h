#include <QDialog>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class FeatureRequestDialog : public QDialog {
    Q_OBJECT

public:
    explicit FeatureRequestDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Request a Feature");
        
        QVBoxLayout *layout = new QVBoxLayout(this);

        titleEdit = new QLineEdit(this);
        titleEdit->setPlaceholderText("Enter feature title...");
        layout->addWidget(titleEdit);

        descriptionEdit = new QTextEdit(this);
        descriptionEdit->setPlaceholderText("Describe the feature...");
        layout->addWidget(descriptionEdit);

        QPushButton *submitButton = new QPushButton("Submit", this);
        layout->addWidget(submitButton);

        connect(submitButton, &QPushButton::clicked, this, &FeatureRequestDialog::submitFeatureRequest);
    }

private slots:
    void submitFeatureRequest() {
        QString title = titleEdit->text();
        QString description = descriptionEdit->toPlainText();

        if (title.isEmpty() || description.isEmpty()) {
            return;  // Do not submit if fields are empty
        }

        QString repoOwner = "YourGitHubUsername";
        QString repoName = "YourRepository";
        QString githubToken = "your_personal_access_token";  // Store securely!

        QUrl url(QString("https://api.github.com/repos/%1/%2/issues").arg(repoOwner).arg(repoName));
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", QString("token %1").arg(githubToken).toUtf8());

        QJsonObject json;
        json["title"] = title;
        json["body"] = description;
        QJsonDocument doc(json);

        QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
        QNetworkReply *reply = networkManager->post(request, doc.toJson());

        connect(reply, &QNetworkReply::finished, this, [reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                qDebug() << "Feature request submitted successfully!";
            } else {
                qDebug() << "Error submitting request: " << reply->errorString();
            }
            reply->deleteLater();
        });

        this->close();
    }

private:
    QLineEdit *titleEdit;
    QTextEdit *descriptionEdit;
};
