#include "rabbitmqc.h"

/*@@@@@@@RabbitMQ监听器@@@@@@@*/
RabbitMQC::RabbitMQC(QObject* parent):QThread(parent){}
RabbitMQC::~RabbitMQC(){
    stopListen();
}
bool RabbitMQC::connect(){
    QByteArray temp;
    _connection=amqp_new_connection();
    _socket=amqp_tcp_socket_new(_connection);
    RabbitMQC::ADDRESS address=getAddress();
    int reply = amqp_socket_open(_socket,address.host.toUtf8().data(),address.port);
    //建立socket失败
    if(reply!=AMQP_STATUS_OK){
        qDebug()<<"建立socket失败";
        return false;
    }
    amqp_rpc_reply_t loginReply=amqp_login(_connection,"/",AMQP_DEFAULT_MAX_CHANNELS,AMQP_DEFAULT_FRAME_SIZE,AMQP_DEFAULT_HEARTBEAT,AMQP_SASL_METHOD_PLAIN,_username.toUtf8().data(),_password.toUtf8().data());
    //登录失败
    if(loginReply.reply_type!=AMQP_RESPONSE_NORMAL){
        qDebug()<<"登录失败";
        return false;
    }
    msleep(50);
    //打开通道
    amqp_channel_open(_connection, _listenChannel);
    amqp_bytes_t _exchange = amqp_cstring_bytes("jld_exchange_topic");
    amqp_bytes_t _type = amqp_cstring_bytes("topic");
    amqp_boolean_t passive=0;//是否被动
    amqp_boolean_t durable=0;//是否耐用
    amqp_boolean_t auto_delete=0;//是否自动清除
    amqp_boolean_t internal=0;//是否内部
    amqp_exchange_declare(_connection, _listenChannel, _exchange, _type, passive, durable, auto_delete, internal, amqp_empty_table);
    amqp_rpc_reply_t exchangeDeclareReply=amqp_get_rpc_reply(_connection);
    //声明交换机失败
    if(exchangeDeclareReply.reply_type!=AMQP_RESPONSE_NORMAL){
        amqp_channel_close(_connection, _listenChannel, AMQP_REPLY_SUCCESS);
        qDebug()<<"声明交换机失败";
        return false;
    }
    msleep(50);
    QString queuename=getMacAddress();
    QByteArray queue=queuename.toUtf8();
    amqp_bytes_t _queue = amqp_cstring_bytes(queue.data());
    amqp_boolean_t _passive = 0;
    amqp_boolean_t _durable = 1;//队列是否持久化，1 为持久
    amqp_boolean_t _exclusive = 0;//是否独占（当前连接不在时，队列是否自动删除）；
    amqp_boolean_t _auto_delete = 1;//是否自动删除（无消费者时）
//    amqp_table_entry_t entries[1];
//    amqp_table_t _arguments;
//    entries[0].key=amqp_cstring_bytes("x-message-ttl");
//    entries[0].value.kind=AMQP_FIELD_KIND_U64;
//    entries[0].value.value.u64=1000*60;
//    _arguments.num_entries=1;
//    _arguments.entries=entries;
    amqp_queue_declare(_connection, _listenChannel, _queue, _passive, _durable, _exclusive, _auto_delete, amqp_empty_table);
    amqp_rpc_reply_t queueDeclareReply=amqp_get_rpc_reply(_connection);
    //声明通道失败
    if(queueDeclareReply.reply_type!=AMQP_RESPONSE_NORMAL){
        amqp_channel_close(_connection, _listenChannel, AMQP_REPLY_SUCCESS);
        qDebug()<<"声明通道失败";
        return false;
    }
    msleep(50);
    temp = _pickupCode.arg(queuename).toUtf8();
    amqp_bytes_t _routeKeyPickup  = amqp_cstring_bytes(temp.data());
    amqp_queue_bind(_connection, _listenChannel, _queue, _exchange, _routeKeyPickup, amqp_empty_table);
    amqp_rpc_reply_t pickupCodeBindReply=amqp_get_rpc_reply(_connection);
    //绑定RoutingKey失败
    if(pickupCodeBindReply.reply_type!=AMQP_RESPONSE_NORMAL){
        amqp_channel_close(_connection, _listenChannel, AMQP_REPLY_SUCCESS);
        qDebug()<<"绑定pickupCodeBindReply失败";
        return false;
    }
    msleep(50);
    temp = _ackNoteKey.arg(queuename).toUtf8();
    amqp_bytes_t _routekeyNote  = amqp_cstring_bytes(temp.data());
    amqp_queue_bind(_connection, _listenChannel, _queue, _exchange, _routekeyNote, amqp_empty_table);
    amqp_rpc_reply_t routeNoteBindReply=amqp_get_rpc_reply(_connection);
    //绑定RoutingKey失败
    if(routeNoteBindReply.reply_type!=AMQP_RESPONSE_NORMAL){
        amqp_channel_close(_connection, _listenChannel, AMQP_REPLY_SUCCESS);
        qDebug()<<"绑定routeNoteBindReply失败";
        return false;
    }
    msleep(50);
    temp = _receiveOrderKey.arg(queuename).toUtf8();
    amqp_bytes_t _routekeyOrder  = amqp_cstring_bytes(temp.data());
    amqp_queue_bind(_connection, _listenChannel, _queue, _exchange, _routekeyOrder, amqp_empty_table);
    amqp_rpc_reply_t routeOrderBindReply=amqp_get_rpc_reply(_connection);
    //绑定RoutingKey失败
    if(routeOrderBindReply.reply_type!=AMQP_RESPONSE_NORMAL){
        amqp_channel_close(_connection, _listenChannel, AMQP_REPLY_SUCCESS);
        qDebug()<<"绑定routeOrderBindReply失败";
        return false;
    }
    msleep(50);
    amqp_bytes_t _routekeyStatus  = amqp_cstring_bytes("jld_test.server.vehicle.state.achieve.");
    amqp_queue_bind(_connection, _listenChannel, _queue, _exchange, _routekeyStatus, amqp_empty_table);
    amqp_rpc_reply_t routeStatusBindReply=amqp_get_rpc_reply(_connection);
    //绑定RoutingKey失败
    if(routeStatusBindReply.reply_type!=AMQP_RESPONSE_NORMAL){
        amqp_channel_close(_connection, _listenChannel, AMQP_REPLY_SUCCESS);
        qDebug()<<"绑定jld_test.server.vehicle.state.achieve.失败";
        return false;
    }
    listen();
    return true;
}
QString RabbitMQC::getMacAddress(){
    QList<QNetworkInterface> nets = QNetworkInterface::allInterfaces();// 获取所有网络接口列表
    int nCnt = nets.count();
    QString strMacAddr = "";
    for(int i = 0; i < nCnt; i ++){
        // 如果此网络接口被激活并且正在运行并且不是回环地址，则就是我们需要找的Mac地址
        if(nets[i].flags().testFlag(QNetworkInterface::IsUp) && nets[i].flags().testFlag(QNetworkInterface::IsRunning) && !nets[i].flags().testFlag(QNetworkInterface::IsLoopBack)){
            strMacAddr = nets[i].hardwareAddress();
            strMacAddr.replace(":","");
            break;
        }
    }
    return strMacAddr;
}
RabbitMQC::ADDRESS RabbitMQC::getAddress(){
    QFile testFile(QDir::homePath()+"/DeliveryRobot/DeliveryRobot/Config/test");
    QFile previewFile(QDir::homePath()+"/DeliveryRobot/DeliveryRobot/Config/preview");
    RabbitMQC::ADDRESS address;
    if(testFile.exists()&&!previewFile.exists()){
        address.host=_hostTest;
        address.port=_portTest;
    }else if(previewFile.exists()){
        address.host=_hostPreview;
        address.port=_portPreviewAndTrue;
    }else{
        address.host=_hostTrue;
        address.port=_portPreviewAndTrue;
    }
    return address;
}
void RabbitMQC::listen(quint8 getNum, struct timeval *timeout){
    _listenInfo.getNum=getNum;
    _listenInfo.timeout=timeout;
    start();
}
void RabbitMQC::run(){
    amqp_basic_qos(_connection,_listenChannel, 0, _listenInfo.getNum, 0);
    int ack = 0; // no_ack    是否需要确认消息后再从队列中删除消息
    QByteArray queue=getMacAddress().toUtf8();
    amqp_bytes_t queuename= amqp_cstring_bytes(queue.data());
    amqp_basic_consume(_connection, _listenChannel, queuename, amqp_empty_bytes, 0, ack, 0, amqp_empty_table);
    {
        //监听循环
        for (;;) {
            amqp_maybe_release_buffers(_connection);
            amqp_envelope_t envelope;
            amqp_rpc_reply_t res = amqp_consume_message(_connection, &envelope, _listenInfo.timeout, 0);
            if (AMQP_RESPONSE_NORMAL != res.reply_type) {
                qDebug()<<"监听循环失败";
                break;
            }
            QString message=QString::fromStdString(std::string((char *)envelope.message.body.bytes, (char *)envelope.message.body.bytes + envelope.message.body.len));
            QString routingKey=QString::fromStdString(std::string((char*)envelope.routing_key.bytes,(char *)envelope.routing_key.bytes+envelope.routing_key.len));
//            qDebug()<<routingKey<<message;
            emit receiceMessageSignal(routingKey,message);
            amqp_basic_ack(_connection, _listenChannel, envelope.delivery_tag, 0);
            amqp_destroy_envelope(&envelope);
            amqp_maybe_release_buffers(_connection);
            if(_exitListen){
                break;
            }
        }
    }
}
void RabbitMQC::publish(QByteArray message){
    amqp_bytes_t _message;
    QString messageStr=message;
    std::string messageString=messageStr.toStdString();
    _message.len = messageString.length();
    _message.bytes = (void *)(messageString.c_str());
    amqp_bytes_t exchange = amqp_cstring_bytes("jld_exchange_topic");
    amqp_bytes_t routekey = amqp_cstring_bytes("jld_test.vehicle.state");
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; // persistent delivery mode
    int publishResult=amqp_basic_publish(_connection, _listenChannel, exchange, routekey, 0, 0, &props, _message);
    amqp_rpc_reply_t exchangeDeclareReply=amqp_get_rpc_reply(_connection);
    if(publishResult!=0||exchangeDeclareReply.reply_type!=AMQP_RESPONSE_NORMAL){
        amqp_channel_close(_connection, _listenChannel, AMQP_REPLY_SUCCESS);
        return;
    }
}
/*@@@@@@@TCPClient@@@@@@@*/
MQClient::MQClient(QObject* parent):QObject(parent){
    _socket=new QTcpSocket(this);
    _timer=new QTimer(this);
    connect(_timer,SIGNAL(timeout()),this,SLOT(timer1000TimeoutSlot()));
}
MQClient::~MQClient(){}
void MQClient::connectToServer(){
    connect(_socket,SIGNAL(connected()),this,SLOT(socketConnectedSlot()));
    _socket->connectToHost(QHostAddress::LocalHost,8888);
    if(_socket->waitForConnected()){
        _timer->start(1000);
        qDebug()<<"连接主程序服务端成功";
    }else{
        qDebug()<<"连接主程序服务端失败";
    }
}
void MQClient::socketConnectedSlot(){
    _mq=new RabbitMQC(this);
    connect(_mq,SIGNAL(receiceMessageSignal(QString,QString)),this,SLOT(receiveMessageSlot(QString,QString)));
    _mq->connect();
    connect(_socket,SIGNAL(readyRead()),this,SLOT(readyReadSlot()));
}
void MQClient::receiveMessageSlot(QString routingKey,QString message){
    QJsonDocument doc;
    QJsonObject obj;
    obj.insert("routingKey",routingKey);
    obj.insert("message",QJsonDocument::fromJson(message.toUtf8()).object());
    doc.setObject(obj);
    _socket->write(doc.toJson(QJsonDocument::Compact)+"\n");
    _socket->flush();
}
void MQClient::readyReadSlot(){
    _timerCount=0;
    QByteArray bytes=_socket->readAll();
    _dealMessage(bytes);
}
void MQClient::_dealMessage(QByteArray bytes){
    if(bytes.contains("\n")){
        int index=bytes.indexOf("\n");
        _wholeBytes+=bytes.mid(0,index);
        _mq->publish(bytes);
        _wholeBytes.clear();
        _dealMessage(bytes.mid(index+1,bytes.length()));
    }else{
        _wholeBytes+=bytes;
    }
}
void MQClient::timer1000TimeoutSlot(){
    _timerCount++;
    if(_timerCount==60){
        int a=system("killall RabbitMQ");
        Q_UNUSED(a);
    }else{}
}
