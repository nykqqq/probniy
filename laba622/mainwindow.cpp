#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupTables();

    connect(ui->loadButton, &QPushButton::clicked,
            this, &MainWindow::on_loadButton_clicked);
    connect(ui->saveValidButton, &QPushButton::clicked,
            this, &MainWindow::on_saveValidButton_clicked);
    connect(ui->saveInvalidButton, &QPushButton::clicked,
            this, &MainWindow::on_saveInvalidButton_clicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupTables()
{
    // Настройка таблицы для корректных объектов
    validModel = new QStandardItemModel(this);
    validModel->setHorizontalHeaderLabels(QStringList()
                                          << "Название" << "Описание" << "Коэффициент защиты" << "Тип защиты");
    ui->validTableView->setModel(validModel);
    ui->validTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Настройка таблицы для битых объектов
    invalidModel = new QStandardItemModel(this);
    invalidModel->setHorizontalHeaderLabels(QStringList()
                                            << "Название" << "Описание" << "Коэффициент защиты" << "Тип защиты" << "Ошибка");
    ui->invalidTableView->setModel(invalidModel);
    ui->invalidTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::on_loadButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    "Выберите JSON файл с данными",
                                                    "",
                                                    "JSON файлы (*.json);;Все файлы (*)");

    if (filePath.isEmpty())
        return;

    loadAndValidateData(filePath);
}

void MainWindow::loadAndValidateData(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл: " + filePath);
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);

    if (jsonDoc.isNull())
    {
        QMessageBox::warning(this, "Ошибка", "Неверный формат JSON");
        return;
    }

    // Очищаем таблицы
    validModel->removeRows(0, validModel->rowCount());
    invalidModel->removeRows(0, invalidModel->rowCount());

    QJsonArray itemsArray;

    if (jsonDoc.isArray())
    {
        itemsArray = jsonDoc.array();
    }
    else if (jsonDoc.isObject())
    {
        itemsArray.append(jsonDoc.object());
    }

    if (itemsArray.isEmpty())
    {
        QMessageBox::information(this, "Информация", "Файл не содержит данных");
        return;
    }

    int validCount = 0;
    int invalidCount = 0;

    for (const auto &value : itemsArray)
    {
        if (!value.isObject())
            continue;

        QJsonObject item = value.toObject();
        QString errorMessage;

        if (validateItem(item, errorMessage))
        {
            addToTable(validModel, item);
            validCount++;
        }
        else
        {
            addToTable(invalidModel, item, errorMessage);
            invalidCount++;
        }
    }

    QMessageBox::information(this, "Загрузка завершена",
                             QString("Корректных объектов: %1\nБитых объектов: %2")
                                 .arg(validCount).arg(invalidCount));
}

bool MainWindow::validateItem(const QJsonObject &item, QString &errorMessage)
{
    // Проверка названия
    if (!item.contains("name") || item["name"].toString().isEmpty())
    {
        errorMessage = "Отсутствует или пустое поле 'name'";
        return false;
    }

    QString name = item["name"].toString();
    QRegularExpression nameRegex("^[А-Я][а-я0-9\\s]*$");
    if (!nameRegex.match(name).hasMatch())
    {
        errorMessage = "Неверный формат названия (должно начинаться с заглавной буквы)";
        return false;
    }

    // Проверка описания
    if (!item.contains("description") || item["description"].toString().isEmpty())
    {
        errorMessage = "Отсутствует или пустое поле 'description'";
        return false;
    }

    // Проверка коэффициента защиты
    if (!item.contains("coefficient"))
    {
        errorMessage = "Отсутствует поле 'coefficient'";
        return false;
    }

    if (!item["coefficient"].isDouble())
    {
        errorMessage = "Поле 'coefficient' не является числом";
        return false;
    }

    double coefficient = item["coefficient"].toDouble();
    if (coefficient < 0 || coefficient > 999.99)
    {
        errorMessage = "Коэффициент защиты должен быть в диапазоне 0-999.99";
        return false;
    }

    // Проверка типа защиты
    if (!item.contains("protection_type"))
    {
        errorMessage = "Отсутствует поле 'protection_type'";
        return false;
    }

    QString protectionType = item["protection_type"].toString();
    QStringList validTypes = {"Магическая", "Универсальная", "Силовая"};

    if (!validTypes.contains(protectionType))
    {
        errorMessage = "Неверный тип защиты. Допустимые: Магическая, Универсальная, Силовая";
        return false;
    }

    return true;
}

void MainWindow::addToTable(QStandardItemModel *model, const QJsonObject &item, const QString &errorMessage)
{
    int row = model->rowCount();

    QStandardItem *nameItem = new QStandardItem(item["name"].toString());
    model->setItem(row, 0, nameItem);

    QStandardItem *descItem = new QStandardItem(item["description"].toString());
    model->setItem(row, 1, descItem);

    QStandardItem *coefItem = new QStandardItem(QString::number(item["coefficient"].toDouble(), 'f', 2));
    model->setItem(row, 2, coefItem);

    QStandardItem *typeItem = new QStandardItem(item["protection_type"].toString());
    model->setItem(row, 3, typeItem);

    if (!errorMessage.isEmpty())
    {
        QStandardItem *errorItem = new QStandardItem(errorMessage);
        model->setItem(row, 4, errorItem);

        // Подсвечиваем красным строку с ошибкой
        for (int col = 0; col < 4; col++)
        {
            if (model->item(row, col))
                model->item(row, col)->setBackground(QColor(255, 200, 200));
        }
    }
}

void MainWindow::on_saveValidButton_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    "Сохранить корректные объекты",
                                                    "valid_objects.json",
                                                    "JSON файлы (*.json)");

    if (filePath.isEmpty())
        return;

    QJsonArray validArray;

    for (int row = 0; row < validModel->rowCount(); row++)
    {
        QJsonObject item;
        item["name"] = validModel->item(row, 0)->text();
        item["description"] = validModel->item(row, 1)->text();
        item["coefficient"] = validModel->item(row, 2)->text().toDouble();
        item["protection_type"] = validModel->item(row, 3)->text();

        validArray.append(item);
    }

    QJsonDocument jsonDoc(validArray);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл");
        return;
    }

    file.write(jsonDoc.toJson(QJsonDocument::Indented));
    file.close();

    if (validArray.isEmpty())
    {
        QMessageBox::information(this, "Успех", "Сохранен пустой файл (нет корректных объектов)");
    }
    else
    {
        QMessageBox::information(this, "Успех",
                                 QString("Сохранено %1 объектов").arg(validModel->rowCount()));
    }
}

void MainWindow::on_saveInvalidButton_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    "Сохранить битые объекты",
                                                    "invalid_objects.json",
                                                    "JSON файлы (*.json)");

    if (filePath.isEmpty())
        return;

    QJsonArray invalidArray;

    for (int row = 0; row < invalidModel->rowCount(); row++)
    {
        QJsonObject item;
        item["name"] = invalidModel->item(row, 0)->text();
        item["description"] = invalidModel->item(row, 1)->text();
        item["coefficient"] = invalidModel->item(row, 2)->text().toDouble();
        item["protection_type"] = invalidModel->item(row, 3)->text();
        item["error"] = invalidModel->item(row, 4)->text();

        invalidArray.append(item);
    }

    QJsonDocument jsonDoc(invalidArray);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл");
        return;
    }

    file.write(jsonDoc.toJson(QJsonDocument::Indented));
    file.close();

    if (invalidArray.isEmpty())
    {
        QMessageBox::information(this, "Успех", "Сохранен пустой файл (нет битых объектов)");
    }
    else
    {
        QMessageBox::information(this, "Успех",
                                 QString("Сохранено %1 битых объектов").arg(invalidModel->rowCount()));
    }
}
