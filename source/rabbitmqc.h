#ifndef RABBITMQC_H
#define RABBITMQC_H

#include <QNetworkInterface>
#include <QThread>
#include <QObject>
#include <QFile>
#include <QDir>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

/*@@@@@@@RabbitMQ监听器@@@@@@@*/
class RabbitMQC:public QThread{
    Q_OBJECT
public:
    struct ADDRESS{
        QString host;
        int port;
    };
public:
    explicit RabbitMQC(QObject* parent=nullptr);
    ~RabbitMQC();
    bool connect();
    void publish(const QByteArray message);
protected:
    void run() override;
private:
    void listen(quint8 getNum=1,struct timeval *timeout = NULL);
    static QString getMacAddress();
    ADDRESS getAddress();
    inline void stopListen(){_exitListen=true;}
signals:
    void receiceMessageSignal(QString,QString);
private:
    amqp_channel_t _listenChannel=1;
    amqp_socket_t* _socket=NULL;
    amqp_connection_state_t _connection=NULL;

    struct LISTEN_INFO{
        quint8 getNum;
        struct timeval* timeout;
    }_listenInfo;

    const QString _hostTest="192.168.10.72";
    const QString _hostPreview="mqtt.preview.jilida.net.cn";
    const QString _hostTrue="mqtt.jilida.net.cn";
    const int     _portTest=5673;
    const int     _portPreviewAndTrue=5672;
    const QString _username="car_device";
    const QString _password="car145236";
    const QString _exchangeName="jld_exchange_topic";
    const QString _ackRobotStateKey="jld_test.server.vehicle.state.achieve.";//服务端索要数据 服务端发送
    const QString _receiveOrderKey="jld_test.server.hand.order.%1"; //服务端发送订单详情
    const QString _sendMQkey="jld_test.vehicle.state";//向服务端发送小车状态
    const QString _ackNoteKey="jld_test.server.vehicle.log.achieve.%1";//服务端索要日志
    const QString _pickupCode="jld_test.server.pickupCode.%1";//MQ发送取货码消息

    bool _exitListen=false;
};
/*@@@@@@@TCPClient@@@@@@@*/
class MQClient:public QObject{
    Q_OBJECT
public:
    explicit MQClient(QObject* parent=nullptr);
    ~MQClient();
    void connectToServer();
private slots:
    void socketConnectedSlot();
    void receiveMessageSlot(QString,QString);
    void readyReadSlot();
    void timer1000TimeoutSlot();
private:
    void _dealMessage(QByteArray bytes);
private:
    QTcpSocket* _socket;
    RabbitMQC* _mq;
    QByteArray _wholeBytes="";
    QTimer* _timer;
    quint8 _timerCount=0;
};

#endif // RABBITMQC_H
