#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QVector>
#include <QSignalMapper>
#include <QScrollArea>
#include <QTableWidget>
#include <QHeaderView>
#include <QRegularExpression>
#include "QTcpSocket"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QProcess sshProcess;

public slots:
    void show_connection_window();

private:

    //Окно подключения
    QWidget *connection_window;

    QVBoxLayout *layout_connection_window;
    QHBoxLayout *layout_of_hosts;
    QHBoxLayout *layout_of_users;

    QLabel *host_label;
    QLabel *user_label;
    QLabel* text_output;

    QLineEdit *host_line_input;
    QLineEdit *user_name_line_input;
    QPushButton *connect_btn;

    QString host_address;
    QString user_name;

    QMessageBox *error;


    int time_to_delay_mlsec = 200;

    //Окно присоеденения к процессу
    QWidget *attach_to_process_window;
    QScrollArea *scroll_area_for_processes;
    QVBoxLayout *layout_attach_to_process_window;
    int count_of_processes = 0;

    QVector<QVector<QString>> list_of_processes; //Двумерный массив хранит в себе процессы, в виде PID User Name

    QSignalMapper *process_attach_mapper;//отслеживание кнопки копирования

    // Окно файлового менеджера
    QWidget* open_executable_window;
    QScrollArea* scroll_area_for_executable;
    QTableWidget* table_for_executable;
    QString path_to_executable = "/";

    // Окно gdb_gui
    QTableWidget *table_disassembled_listing;
    QTableWidget *table_registers;
    QTcpSocket socket;

    int current_machine_command;

    Ui::MainWindow *ui;

private slots:
    QString calculateChecksum(QString str);
    int ssh_connection();
    int show_mainwindow();
    void delay(int time_to_wait_mlsec);

    void on_attach_to_process_btn_triggered();

    int show_attach_to_process_window();

    int show_open_executable_window();

    int add_data_to_table_for_executable(QString data);

    void get_data_to_table_for_executable(QString value);

    void openSelected(int nRow, int nCol);

    int get_processes();

    QWidget *create_widget_process(int id);

    int attach_to_process(int id);

    int add_data_to_console(QString data);

    QString request_to_gdb_server(QString response);

    int add_data_to_disassembled_listing();

    int colorize_machine_command(int current_machine_command);

    int add_data_to_registers();

    int start_gdb(QString program);

    int stop_gdb();

    void on_actionOpen_executable_triggered();
    void on_btn_send_clicked();
    void on_stopGdbBtn_clicked();
    void on_nextBtn_clicked();

    int reload_data();

    int TCP_connection();
    int request_data_tcp(QString data);
    void on_pushButton_clicked();

};
#endif // MAINWINDOW_H
