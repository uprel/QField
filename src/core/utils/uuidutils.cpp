/***************************************************************************
                        uuidutils.cpp
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

#include "uuidutils.h"
#include <QUuid>

UuidUtils::UuidUtils( QObject *parent )
  : QObject( parent )
{

}

QString UuidUtils::createUuid()
{
  return QUuid::createUuid().toString();
}
