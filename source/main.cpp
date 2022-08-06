#include <QCoreApplication>
#include <source/rabbitmqc.h>

int main(int argc, char *argv[]){
    QCoreApplication a(argc, argv);
    MQClient* client=new MQClient();
    client->connectToServer();
    return a.exec();
}
