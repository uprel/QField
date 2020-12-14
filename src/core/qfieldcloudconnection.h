/***************************************************************************
    qfieldcloudconnection.cpp
    ---------------------
    begin                : January 2020
    copyright            : (C) 2020 by Matthias Kuhn
    email                : matthias at opengis dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QFIELDCLOUDCONNECTION_H
#define QFIELDCLOUDCONNECTION_H

#include "networkmanager.h"
#include "networkreply.h"

#include <QObject>
#include <QVariantMap>


class QNetworkRequest;

class QFieldCloudConnection : public QObject
{
    Q_OBJECT

  public:
    enum class ConnectionStatus
    {
      Disconnected,
      Connecting,
      LoggedIn
    };
    Q_ENUM( ConnectionStatus )

    enum class ConnectionState
    {
      Idle,
      Busy
    };
    Q_ENUM( ConnectionState )

    QFieldCloudConnection();

    Q_PROPERTY( QString username READ username WRITE setUsername NOTIFY usernameChanged )
    Q_PROPERTY( QString password READ password WRITE setPassword NOTIFY passwordChanged )
    Q_PROPERTY( QString url READ url WRITE setUrl NOTIFY urlChanged )
    Q_PROPERTY( QString defaultUrl READ defaultUrl CONSTANT )

    Q_PROPERTY( ConnectionStatus status READ status NOTIFY statusChanged )
    Q_PROPERTY( ConnectionState state READ state NOTIFY stateChanged )
    Q_PROPERTY( bool hasToken READ hasToken NOTIFY tokenChanged )

    /**
     * Returns an error string to be shown to the user if \a reply has an error.
     */
    static QString errorString( QNetworkReply *reply );

    /**
     * Returns the currently set server connection URL.
     */
    QString url() const;

    /**
     * Sets the current server connection URL and saves it into QSettings.
     */
    void setUrl( const QString &url );

    /**
     * Default server connection URL, pointing to the production server.
     */
    static QString defaultUrl();

    QString username() const;
    void setUsername( const QString &username );

    QString password() const;
    void setPassword( const QString &password );

    Q_INVOKABLE void login();
    Q_INVOKABLE void logout();

    ConnectionStatus status() const;
    ConnectionState state() const;

    bool hasToken() { return !mToken.isEmpty(); }

    /**
     * Sends a post request with the given \a parameters to the given \a endpoint.
     *
     * If this connection is not logged in, will return nullptr.
     * The returned reply needs to be deleted by the caller.
     */
    NetworkReply *post( const QString &endpoint, const QVariantMap &params = QVariantMap(), const QStringList &fileNames = QStringList() );


    /**
     * Sends a get request to the given \a endpoint.
     *
     * If this connection is not logged in, will return nullptr.
     * The returned reply needs to be deleted by the caller.
     */
    NetworkReply *get( const QString &endpoint, const QVariantMap &params = QVariantMap() );

  signals:
    void usernameChanged();
    void passwordChanged();
    void urlChanged();
    void statusChanged();
    void stateChanged();
    void tokenChanged();
    void error();

    void loginFailed( const QString &reason );

  private:
    void setStatus( ConnectionStatus status );
    void setState( ConnectionState state );
    void setToken( const QByteArray &token );
    void invalidateToken();
    void setAuthenticationToken( QNetworkRequest &request );

    QString mPassword;
    QString mUsername;
    QString mUrl;
    ConnectionStatus mStatus = ConnectionStatus::Disconnected;
    ConnectionState mState = ConnectionState::Idle;
    QByteArray mToken;

    int mPendingRequests = 0;

};

#endif // QFIELDCLOUDCONNECTION_H
