#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QCoreApplication"
#include "QThread"
#include <QProcess>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QVector>
#include <QSignalMapper>
#include <QSpacerItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//Функция вывода окна для подключения к хосту
void MainWindow::show_connection_window() {

    //Создаем само окно
    connection_window = new QWidget();
    connection_window->setGeometry(630,320,400,200);

    //Наполняем окно виджетами
    layout_connection_window = new QVBoxLayout;

    layout_of_hosts = new QHBoxLayout;
    host_label = new QLabel("Host: ");
    host_line_input = new QLineEdit();

    layout_of_hosts->addWidget(host_label);
    layout_of_hosts->addWidget(host_line_input);

    layout_of_users = new QHBoxLayout;
    user_label = new QLabel("User: ");
    user_name_line_input = new QLineEdit();

    layout_of_users->addWidget(user_label);
    layout_of_users->addWidget(user_name_line_input);

    //Создание кнопки и привязка сигнала
    connect_btn = new QPushButton("Connect");
    connect(connect_btn, SIGNAL(clicked()), this, SLOT(ssh_connection()));


    //Добавляем в окно лейбл кнопку и инпут
    layout_connection_window->addLayout(layout_of_hosts);
    layout_connection_window->addLayout(layout_of_users);
    layout_connection_window->addWidget(connect_btn);

    connection_window->setLayout(layout_connection_window);

    //Показываем окно
    connection_window->show();
}

int MainWindow::ssh_connection()
{
    //Устанавливаем ip хоста и имя пользователя
    host_address = host_line_input->text();
    user_name = user_name_line_input->text();

    // -T - отключает аллокацию псевдотерминала, что может помочь избежать ошибок
    // -o StrictHostKeyChecking=no - отключает проверку ключей хоста (для упрощения)
    //sshProcess.start("ssh", QStringList() << "-T" << "-o" << "StrictHostKeyChecking=no" << userName + "@" + hostAddress);\

    qDebug() << "Подключение...";
    sshProcess.start("ssh", QStringList() << "-T" << "-o" << "StrictHostKeyChecking=no" << user_name + "@" + host_address);

    delay(time_to_delay_mlsec);


    QString current_command = "ls ~";

    QString current_response = request_to_gdb_server(current_command);

    delay(time_to_delay_mlsec);

    qDebug() << "Ответ: \n" << current_response;

    // Ожидаем завершение процесса подключения по SSH
    if (current_response == nullptr) {
        qDebug() << "Ошибка подключения!" << sshProcess.errorString();
        error->critical(NULL,QObject::tr("Ошибка"),tr("Ошибка подключения!"));
        return 1;
    }
    else {
        error->information(NULL,QObject::tr("Подключение"),tr("Успешно подключено!"));

        delete[] connection_window;
        this->show_mainwindow();
        this->show();

        return 0;
    }


}

int MainWindow::show_mainwindow()
{

    return 0;
}



void MainWindow::delay(int time_to_wait_mlsec)
{
    QCoreApplication::processEvents();
    QThread::msleep(time_to_wait_mlsec);
}


void MainWindow::on_attach_to_process_btn_triggered()
{
    show_attach_to_process_window();

}

int MainWindow::show_attach_to_process_window()
{
    qDebug() << "Функция - show_attach_to_process_window \n";

    //Создание окна процессов
    attach_to_process_window = new QWidget();
    scroll_area_for_processes = new QScrollArea();
    layout_attach_to_process_window = new QVBoxLayout();

    scroll_area_for_processes->setWidgetResizable(true); // Устанавливаем, чтобы виджет внутри QScrollArea мог изменять размеры


    //Вызов функции получение процессов
    if(get_processes()){
        qDebug("Ошибка вызова функции запроса процессов!!!");
        return 0;
    }

    //Создание процессов
    for (int i = 0; i < count_of_processes - 1; i++) {
        QWidget *current_process = create_widget_process(i);
        layout_attach_to_process_window->addWidget(current_process);
    }


    //Создание виджета, хранения процессов
    QWidget *temp_wgd = new QWidget();
    temp_wgd->setLayout(layout_attach_to_process_window);

    //Добавляем виджет в скрол арену
    scroll_area_for_processes->setWidget(temp_wgd);

    //Создание главного лайаута и добавление в него скрол арены
    QVBoxLayout *main_layout = new QVBoxLayout();
    main_layout->addWidget(scroll_area_for_processes);

    //Добавление главного лайаута в основное окно показа процессов
    attach_to_process_window->setLayout(main_layout);

    //Отображение окна
    attach_to_process_window->show();

    return 0;
}

int MainWindow::get_processes()
{
    qDebug() << "Функция get_processes\n";

    QString current_request = "ps -aux";

    QString response = request_to_gdb_server(current_request);

    QStringList lines = response.split('\n', Qt::SkipEmptyParts);
    count_of_processes = response.count('\n') + 1;

    qDebug() << "Количество процессов = " << count_of_processes;

    //Сигнал на отслеживание процесса
    process_attach_mapper = new QSignalMapper(this);
    QObject::connect(process_attach_mapper,SIGNAL(mappedInt(int)),this,SLOT(attach_to_process(int)));

    // Проходим по каждой строке
    for (const QString& line : lines) {
        // Разбиваем строку на столбцы, используя пробел в качестве разделителя
        QStringList columns = line.split(' ', Qt::SkipEmptyParts);

        // Проверяем, что у нас есть достаточно столбцов (PID, User, Name)
        if (columns.size() >= 11) {
            QVector<QString> processInfo;
            // PID
            processInfo.append(columns[1]);
            // User
            processInfo.append(columns[0]);
            // Name (поле 'Name' может содержать пробелы, поэтому объединяем остальные столбцы)
            processInfo.append(columns.mid(10).join(' '));

            // Добавляем информацию о процессе в список
            list_of_processes.append(processInfo);
        }
    }

    return 0;
}


QWidget* MainWindow::create_widget_process(int id)
{
    qDebug() << "Функция create_widget_process\n";
    //Создание виджета текущего процесса
    QWidget * current_widget = new QWidget();
    //Создание лайаута для виджета
    QHBoxLayout *current_layout = new QHBoxLayout();

    //Создание форм
    QLabel *PID = new QLabel(list_of_processes[id][0]);
    QLabel *User = new QLabel(list_of_processes[id][1]);
    QLabel *Name = new QLabel(list_of_processes[id][2]);

    //Создание кнопки присоеденение
    QPushButton *current_widget_btn = new QPushButton("attach");

    //Сигналы на передачу данных от кнопок
    process_attach_mapper->setMapping(current_widget_btn,id); //Вместе с кнопкой передаем номер цикла - это будет id
    connect(current_widget_btn,SIGNAL(clicked()),process_attach_mapper,SLOT(map())); //слот на передачу при нажатии

    //Добавление в элементов лайаут
    current_layout->addWidget(PID);
    current_layout->addWidget(User);
    current_layout->addWidget(Name);
    current_layout->addWidget(current_widget_btn);

    //Добавление лайаута в виджет
    current_widget->setLayout(current_layout);

    //current_widget->setFixedSize(380,30);

    //current_widget->setStyleSheet("background-color:purple; color: white;");
    return current_widget;
}

int MainWindow::attach_to_process(int id)
{
    //qDebug() << "Функция attach_to_process, id кнопки - " << id;
    //qDebug() << "PID процесса " << list_of_processes[id][0];

    add_data_to_console(list_of_processes[id][0]);

    return 0;
}





int MainWindow::add_data_to_console(QString data) {

    ui->label->setText(data);

    return 0;
}

QString MainWindow::request_to_gdb_server(QString request)
{

    sshProcess.write(request.toUtf8() + "\n");
    sshProcess.waitForBytesWritten();

    delay(time_to_delay_mlsec);

    // Ждем ответа от удаленного хоста
    sshProcess.waitForReadyRead();

    QString response = sshProcess.readAll().data();

    return response;
}


