/***************************************************************************
                        uuidutils.h
                        ---------------
  begin                : Feb 2021
  copyright            : (C) 2021 by Ivan Ivanov
  email                : ivan@opengis.ch
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef UUIDUTILS_H
#define UUIDUTILS_H

#include <QObject>


class UuidUtils : public QObject
{
    Q_OBJECT

  public:


    explicit UuidUtils( QObject *parent = nullptr );

    //! Checks whether the string \a term is part of \a source
    static Q_INVOKABLE QString createUuid();
};

#endif // UUIDUTILS_H

