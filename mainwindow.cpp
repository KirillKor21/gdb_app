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
#include <QTcpSocket>
#include <QHostAddress>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    //sshProcess.kill();
    socket.close();
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

    //Создание кнопки присоединение
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

// Запуск gdb и заполнение виджетов
int MainWindow::start_gdb(QString program)
{
    text_output = new QLabel();

    // Запуск gdb
    QString current_command = "gdb " + program;
    QString current_response = request_to_gdb_server(current_command);

    // Запуск программы/процесса в gdb
    current_command = "start";
    current_response = request_to_gdb_server(current_command);
    //Переключение на первую машинную команду
    QString current_command_step = "step";
    QString current_response_step = request_to_gdb_server(current_command_step);
    current_machine_command = 1;

    // парсинг ответа
    current_response = current_response.replace("(gdb)", "");

    // Отображение информации по запуску в поле Output
    text_output->setText(current_response);
    ui->scroll_area_output->setWidget(text_output);

    // Заполнение виджетов
    add_data_to_disassembled_listing();
    add_data_to_registers();

    //Окрашивание текущей строки
    colorize_machine_command(current_machine_command);

    return 0;
}

// Остановка gdb и очистка виджетов
int MainWindow::stop_gdb(){
    QString current_command = "quit";
    QString current_response = request_to_gdb_server(current_command);
    current_command = "Y";
    current_response = request_to_gdb_server(current_command);

    table_disassembled_listing->clear();
    table_disassembled_listing->setHorizontalHeaderLabels(QStringList() << "Address" << "Assembly");
    table_registers->clear();
    table_registers->setHorizontalHeaderLabels(QStringList() << "Register" << "Value" << "Address");
    text_output->setText("");

    return 0;
}

// Получение дизассемблированного листинга
int MainWindow::add_data_to_disassembled_listing()
{
    // передача команды в gdb на получение дизассемблированного листинга
    QString current_command = "disassemble";
    QString current_response = request_to_gdb_server(current_command);

    // Создание таблицы для отображения ответа от gdb
    table_disassembled_listing = new QTableWidget();
    table_disassembled_listing->setColumnCount(2);
    table_disassembled_listing->setRowCount(current_response.count("\n") - 2);
    table_disassembled_listing->setHorizontalHeaderLabels(QStringList() << "Address" << "Assembly");
    table_disassembled_listing->verticalHeader()->setVisible(false);
    table_disassembled_listing->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    //table->verticalHeader()->setStretchLastSection(true);
    table_disassembled_listing->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_disassembled_listing->setShowGrid(false);

    // удаление лишней информации из ответа от gdb
    int ind = current_response.indexOf("=>");
    current_response = current_response.remove(0, ind + 2);
    ind = current_response.indexOf("End");
    current_response = current_response.remove(ind, current_response.length());

    // парсинг ответа от gdb
    QStringList list = current_response.split(QRegularExpression("[\t\n]"));
    for (int i = 0; i < list.length() - 1; i++)
        table_disassembled_listing->setItem(i / 2, i % 2, new QTableWidgetItem(list[i].trimmed()));

    // отображение ответа от gdb
    ui->scroll_area_disassembled_listing->setWidget(table_disassembled_listing);

    return 0;
}

int MainWindow::colorize_machine_command(int current_machine_command)
{
    int row = current_machine_command; // номер строки на которой находится текущая машинная команда
    int column_count = table_disassembled_listing->columnCount();

    for (int column = 0; column < column_count; ++column) {
        QTableWidgetItem *item = table_disassembled_listing->item(row, column);
        item->setBackground(QColor(Qt::red));
    }

    return 0;
}


// Получение информации о регистрах
int MainWindow::add_data_to_registers()
{
    // передача команды в gdb на получение информации о регистрах
    QString current_command = "i r";
    QString current_response = request_to_gdb_server(current_command);

    // Создание таблицы для отображения ответа от gdb
    table_registers = new QTableWidget();
    table_registers->setColumnCount(3);
    table_registers->setRowCount(current_response.count("\n"));
    table_registers->setHorizontalHeaderLabels(QStringList() << "Register" << "Value" << "Address");
    table_registers->verticalHeader()->setVisible(false);
    table_registers->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    //table->verticalHeader()->setStretchLastSection(true);
    table_registers->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_registers->setShowGrid(false);

    // удаление лишней информации из ответа от gdb
    current_response.replace("(gdb)", "");
    while (current_response.indexOf("   ") != -1)
            current_response.replace("   ", "  ");
    current_response.replace("  ", "\t");

    // парсинг ответа от gdb
    QStringList list = current_response.split(QRegularExpression("[\t\n]"));
    qDebug() << list;
    for (int i = 0; i < list.length() - 1; i++)
        table_registers->setItem(i / 3, i % 3, new QTableWidgetItem(list[i]));

    // вывод информации на виджет
    ui->scroll_area_registers->setWidget(table_registers);

    return 0;
}


int MainWindow::add_data_to_console(QString data) {

    //ui->label->setText(data);

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

void MainWindow::on_actionOpen_executable_triggered()
{
    show_open_executable_window();
}

// Создание файлового менеджера
int MainWindow::show_open_executable_window()
{
    qDebug() << "Функция - show_open_executable_window \n";

    //Создание окна файлового менеджера
    open_executable_window = new QWidget();
    open_executable_window->setWindowTitle("Open executable");
    open_executable_window->setGeometry(630,320,500,200);

    scroll_area_for_executable = new QScrollArea();
    scroll_area_for_executable->setWidgetResizable(true);

    table_for_executable = new QTableWidget();
    connect(table_for_executable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(openSelected(int, int)));

    // Задание параметров таблицы
    table_for_executable->setColumnCount(3);
    table_for_executable->verticalHeader()->setVisible(false);
    table_for_executable->horizontalHeader()->setVisible(false);
    table_for_executable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_for_executable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_for_executable->setShowGrid(false);

    // Получение данных и заполнение таблицы
    get_data_to_table_for_executable("");

    // Добавление таблицы в скрол арену
    scroll_area_for_executable->setWidget(table_for_executable);

    //Создание главного лайаута и добавление в него скрол арены
    QVBoxLayout *main_layout = new QVBoxLayout();
    main_layout->addWidget(scroll_area_for_executable);

    //Добавление главного лайаута в основное окно показа процессов
    open_executable_window->setLayout(main_layout);

    //Отображение окна
    open_executable_window->show();

    return 0;
}

int MainWindow::add_data_to_table_for_executable(QString data)
{
    table_for_executable->clear();

    table_for_executable->setRowCount(data.count("\n") / 3 + 1);

    QStringList list = ("⮌\n" + data).split("\n");
    for (int i = 0; i < list.length() - 1; i++)
        table_for_executable->setItem(i / 3, i % 3, new QTableWidgetItem(list[i]));

    return 0;
}

void MainWindow::get_data_to_table_for_executable(QString dir)
{
    // Переход в директорию и получение списка файлов
    QString current_command = "cd " + dir + "&& ls -a";
    QString current_response = request_to_gdb_server(current_command);
    qDebug() << "cd" << current_response;

    // Проверка директории на пустоту
    if (current_response != ".\n..\n") {
        current_command = "ls";
        current_response = request_to_gdb_server(current_command);
        qDebug() << current_response;
    } else current_response = "";

    // Заполнение таблицы
    add_data_to_table_for_executable(current_response);
}

// Открытие файлов по двойному клику
void MainWindow::openSelected(int nRow, int nCol)
{
    QString value = table_for_executable->item(nRow, nCol)->text();
    if (value == "⮌") {
        get_data_to_table_for_executable("..");
    }
    if (value.indexOf(".") == -1) {
        get_data_to_table_for_executable(value);
    }
    if (value.indexOf(".out") != -1) {
        open_executable_window->close();
        start_gdb(value);
    }
}

void MainWindow::on_btn_send_clicked()
{
    request_data_tcp(ui->line_command->text());

//    QString current_command = ui->line_command->text();
//    QString current_response = request_to_gdb_server(current_command);
//    text_output->setText(text_output->text() + current_response);
//    qDebug() << current_response;

//    ui->line_command->setText("");
}

void MainWindow::on_stopGdbBtn_clicked()
{
    socket.close();
    stop_gdb();
}


void MainWindow::on_nextBtn_clicked()
{
    //Отменяем окрашивание предыдущей команды
    int row = current_machine_command; // номер строки на которой находится текущая машинная команда
    int column_count = table_disassembled_listing->columnCount();

    for (int column = 0; column < column_count; ++column) {
        QTableWidgetItem *item = table_disassembled_listing->item(row, column);
        item->setBackground(QColor(Qt::white));
    }
    current_machine_command++;
    colorize_machine_command(current_machine_command);
    //Обновляем окна
    reload_data();
}

int MainWindow::reload_data()
{
    // !!! Добавить сюда функции вызова стека и HEX, когда они появятся !!!
    delete[] table_registers;
    QString current_command_step = "step";
    QString current_response_step = request_to_gdb_server(current_command_step);
    qDebug() << "*********";
    add_data_to_registers();
    return 0;
}

int MainWindow::TCP_connection() {

    // Соединяемся с сервером по указанному IP и порту
    socket.connectToHost(QHostAddress("192.168.31.102"), 1234);

    // Ждем, пока будет установлено соединение
    if (socket.waitForConnected()) {
        QString command;
        int local_delay = 50;
        delay(local_delay);
        command = "+";
        socket.write(command.toUtf8());
        delay(local_delay);
        command = "qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;no-resumed+;xmlRegisters=i386";
        socket.write(((QString)"$" + command + (QString)"#" + calculateChecksum(command)).toUtf8());
        delay(local_delay);
        command = "+";
        socket.write(command.toUtf8());
        delay(local_delay);
        command = "Hc-1";
        socket.write(((QString)"$" + command + (QString)"#" + calculateChecksum(command)).toUtf8());
        delay(local_delay);
        command = "+";
        command = "vMustReplyEmpty";
        socket.write(((QString)"$" + command + (QString)"#" + calculateChecksum(command)).toUtf8());
        delay(local_delay);
        command = "+";
        socket.write(command.toUtf8());
        delay(local_delay);
        command = "QStartNoAckMode";
        socket.write(((QString)"$" + command + (QString)"#" + calculateChecksum(command)).toUtf8());
        delay(local_delay);
        command = "+";
        socket.write(command.toUtf8());
        delay(local_delay);
        command = "QProgramSignals:0;1;3;4;6;7;8;9;a;b;c;d;e;f;10;11;12;13;14;15;16;17;18;19;1a;1b;1c;1d;1e;1f;20;21;22;23;24;25;26;27;28;29;2a;2b;2c;2d;2e;2f;30;31;32;33;34;35;36;37;38;39;3a;3b;3c;3d;3e;3f;40;41;42;43;44;45;46;47;48;49;4a;4b;4c;4d;4e;4f;50;51;52;53;54;55;56;57;58;59;5a;5b;5c;5d;5e;5f;60;61;62;63;64;65;66;67;68;69;6a;6b;6c;6d;6e;6f;70;71;72;73;74;75;76;77;78;79;7a;7b;7c;7d;7e;7f;80;81;82;83;84;85;86;87;88;89;8a;8b;8c;8d;8e;8f;90;91;92;93;94;95;96;97;";
        socket.write(((QString)"$" + command + (QString)"#" + calculateChecksum(command)).toUtf8());
        delay(local_delay);
        command = "Hgp0.0";
        socket.write(((QString)"$" + command + (QString)"#" + calculateChecksum(command)).toUtf8());
        delay(local_delay);
        command = "qXfer:features:read:target.xml:0,1000";
        socket.write(((QString)"$" + command + (QString)"#" + calculateChecksum(command)).toUtf8());
        delay(local_delay);
        qDebug() << "TCP connection is success";


        // Отправляем данные на сервер
        //request_data_tcp();
    } else {
        qDebug() << "Error connecting to server: " << socket.errorString();
    }

    return 0;
}

int MainWindow::request_data_tcp(QString data) {

    socket.write(((QString)"$" + data + (QString)"#" + calculateChecksum(data)).toUtf8());

    // Ждем, пока данные будут отправлены
    if (socket.waitForBytesWritten()) {
        // Ждем, пока придет ответ от сервера
        if (socket.waitForReadyRead()) {
            // Считываем ответ
            QByteArray responseData = socket.readAll();
            qDebug() << "\n\nResponse from server: " << responseData << "\n\n";
        } else {
            qDebug() << "Error waiting for response from server: " << socket.errorString();
        }
    } else {
        qDebug() << "Error writing to server: " << socket.errorString();
    }

    // Закрываем соединение
    //socket.close();

    return 0;
}

QString MainWindow::calculateChecksum(QString str)
{
    int checksum = 0;

    // Преобразование строки в массив байтов
    QByteArray byteArray = str.toUtf8();

    // Вычисление контрольной суммы
    for (int i = 0; i < byteArray.size(); ++i)
    {
        checksum += byteArray[i];
    }

    // Остаток от деления на 256
    checksum %= 256;

    // Перевод контрольной суммы в 16-ричный формат
    QString checksumStr = QString("%1").arg(checksum, 2, 16, QChar('0'));

    return checksumStr;
}

void MainWindow::on_pushButton_clicked()
{
    TCP_connection();
}

