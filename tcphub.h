#ifndef TCPHUB_H
#define TCPHUB_H

#include <QObject>



/*
 *  - VESC connects to Server (HUB) and gets registered.
 *  - Timeout if no valid data from connected thing.
 *  - VESC sends its FW-info and a password message that gets associated with the TCP connection
 *    - Break out the FW_info command (assemble info) make callable form VESC_C_IF extension.
 *  - Connected vesc sends IM-ALIVE messages periodically.
 *
 *  - VESC_TOOL connects to server and and can list connected VESCS
 *  - VESC_TOOL can connect to a VESC, via the HUB, if it provides the correct password.
 *
 *  - If VESC_TOOL drops, reconnect if made avaialable again
 *  - If the VESC drops, kill connection.
 */

class TcpHub : public QObject
{
    Q_OBJECT
public:
    explicit TcpHub(QObject *parent = nullptr);

signals:

};

#endif // TCPHUB_H