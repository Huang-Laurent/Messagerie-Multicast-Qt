#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QBuffer>
#include <QFileDialog>
#include <QImage>
#include <QInputDialog>


#define MULTICAST_ADDRESS "239.255.255.252"
#define MULTICAST_PORT    12345


/* Constructeur fenetre principale */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    isJoined_(false),
    count_image(0)
{
    /* Setup fenetre */
    ui->setupUi(this);
    this->setWindowTitle("Messagerie Multicast");
    ui->lineEdit->setText("Anonyme");

    /* Setup socket UDP */
    socket = new QUdpSocket(this);
    socket->setSocketOption(QAbstractSocket::MulticastTtlOption, 1);

    if (!socket->bind(QHostAddress(MULTICAST_ADDRESS), MULTICAST_PORT, QUdpSocket::ReuseAddressHint|QUdpSocket::ShareAddress))
        exit(EXIT_FAILURE);
    socket->joinMulticastGroup((QHostAddress(MULTICAST_ADDRESS)));

    connect(socket, &QUdpSocket::readyRead, this, &MainWindow::processPendingDatagram);

    /* Creation d'un dossier temporaire pour fichiers */
    QDir dir_tmp("./tmp");
    if (!dir_tmp.exists())
        dir_tmp.mkdir("./tmp");

    /* Pop-up pseudo au demarrage */
    bool ok;
    QString text = QInputDialog::getText(this,
                                         tr("Messagerie Multicast"),
                                         tr("Choisissez votre pseudo"),
                                         QLineEdit::Normal,
                                         QDir::home().dirName(),
                                         &ok);
    if (ok and !text.isEmpty())
        ui->lineEdit->setText(text);
    else
        ui->lineEdit->setText("Anonyme");
    ui->lineEdit->setReadOnly(true);

    /* Prevenir les autres utilisateurs l'arivee */
    on_pushButton_Send_clicked();
}


/* Destructeur fenetre principale */
MainWindow::~MainWindow()
{
    /* Destruction dossier tmp */
//    QString path = "./tmp/";
//    if (path.isEmpty())
//          return;
//    QDir dir(path);
//    if(!dir.exists())
//        return;
//    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
//    QFileInfoList fileList = dir.entryInfoList();
//    foreach (QFileInfo file, fileList){
//        if (file.isFile()){
//            bool isDelete = file.dir().remove(file.fileName());
//            if(isDelete) {
//                m_fileCount++;
//            }
//            qDebug() << "isDelete= " << isDelete << " m_fileCount = " << m_fileCount << " filename= " << file.fileName();
//        }else{
//            deleteDir(file.absoluteFilePath());
//        }
//    }
//    dir.rmpath(dir.absolutePath());

    /* Prevenir les autres utilisateurs le depart */
    QByteArray datagram = ui->lineEdit->text().toUtf8();
    datagram = datagram + " s'est déconnecté :(";
    socket->writeDatagram(datagram, QHostAddress(MULTICAST_ADDRESS), MULTICAST_PORT);

    /* Destruction UI */
    delete ui;
}


/* Constructeur bool isJoined_ */
void MainWindow::setisJoined(const bool& isJoined)
{
    this->isJoined_ = isJoined;
}


/* Slot bouton "Envoyer" */
void MainWindow::on_pushButton_Send_clicked()
{
    if (isJoined_ == true)
    {
        QByteArray message = ui->textEdit->toPlainText().toUtf8();
        QByteArray datagram = ui->lineEdit->text().toUtf8();
        datagram = datagram + " : " + message;
        socket->writeDatagram(datagram, QHostAddress(MULTICAST_ADDRESS), MULTICAST_PORT);
        ui->textEdit->clear();
    } else
    {
        QByteArray datagram = ui->lineEdit->text().toUtf8();
        datagram = datagram + ":arejoint#147258!@.";
        socket->writeDatagram(datagram, QHostAddress(MULTICAST_ADDRESS), MULTICAST_PORT);
        ui->textEdit->clear();
        setisJoined(true);
    }
}


/* Slot reception message */
void MainWindow::processPendingDatagram()
{
    while (socket->hasPendingDatagrams())
    {
        QByteArray datagram;
        QHostAddress des;
        quint16 peerPort;
        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &des, &peerPort);
        if (datagram.size() < 2000)
        {
            QStringList msg_split = QString(datagram.data()).split(":");
            if (msg_split[0] == "Anonyme " and msg_split[1] != "/image#147258!@.")
            {
                ui->textBrowser->append("Anonyme " + QString::number(peerPort) + " :" + msg_split[1]);
            } else if (msg_split[0] == "Anonyme" and msg_split[1] == "/image#147258!@.")
            {
                ui->textBrowser->append("Anonyme " + QString::number(peerPort) + " a envoyé une image.");
                ui->label_envoyeurImage->setText("Image envoyée par Anonyme " + QString::number(peerPort));
            } else if (msg_split[0] != "Anonyme" and msg_split[1] == "/image#147258!@.")
            {
                ui->textBrowser->append(msg_split[0] + " a envoyé une image.");
                ui->label_envoyeurImage->setText("Image envoyée par " + msg_split[0]);
            } else if (msg_split[0] != "Anonyme" and msg_split[1] == "arejoint#147258!@.")
            {
                ui->textBrowser->append(msg_split[0] + " a rejoint la discussion !");
            } else if (msg_split[0] == "Anonyme" and msg_split[1] == "arejoint#147258!@.")
            {
                ui->textBrowser->append("Anonyme " + QString::number(peerPort) + " a rejoint la discussion !");
            } else
            {
                ui->textBrowser->append(datagram.data());
            }
        } else
        {
            QImage image_temp = get_imagedata_from_byte(datagram.data());
            image_temp.save(QString("./tmp/image%1.png").arg(count_image), "PNG");
            QImage* imgScaled = new QImage;
            *imgScaled = image_temp.scaled(400, 400, Qt::KeepAspectRatio);
            ui->label_image->setPixmap(QPixmap::fromImage(*imgScaled));
            // ui->label_image->setPixmap(QPixmap::fromImage(image_temp).scaled(ui->label_image->size()));
            count_image++;
        }
    }
}


QByteArray MainWindow::get_imagedata_from_imagefile(const QImage &image)
{
    QByteArray imageData;
    QBuffer buffer(&imageData);
    image.save(&buffer, "JPG");
    imageData = imageData.toBase64();
    return imageData;
}


QImage MainWindow::get_imagedata_from_byte(const QString &data)
{
    QByteArray imageData = QByteArray::fromBase64(data.toLatin1());
    QImage image;
    image.loadFromData(imageData);
    return image;
}


/* Slot bouton "Envoyer une image" */
void MainWindow::on_pushButton_Send_Image_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Image à envoyer", QDir::currentPath(), "Image Files(*.jpg *.png)");
    if (file_name == "")
        return;

    image.load(file_name);
    ui->label_image->setPixmap(QPixmap::fromImage(image).scaled(ui->label_image->size()));

    image_file_data = get_imagedata_from_imagefile(image);
    socket->writeDatagram(image_file_data.toLatin1(), image_file_data.size(), QHostAddress(MULTICAST_ADDRESS), MULTICAST_PORT);

    QByteArray datagram = ui->lineEdit->text().toUtf8();
    datagram += ":/image#147258!@.";
    socket->writeDatagram(datagram, QHostAddress(MULTICAST_ADDRESS), MULTICAST_PORT);
}


/* Slot bouton "Enregistrer l'image" */
void MainWindow::on_pushButton_Send_Image_2_clicked()
{
    QString file_path = QFileDialog::getExistingDirectory(this, "Enregistrer sous...", "./");
    QImage image_temp = QImage(QString("./tmp/image%1.png").arg(count_image-1));
    if (!image_temp.save(file_path + QString("/image%1.png").arg(count_image-1), "PNG"))
        // qDebug() << file_path + QString("/image%1.png").arg(count_image-1) << endl;
        return;
}
