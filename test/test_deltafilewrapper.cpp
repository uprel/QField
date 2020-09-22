/***************************************************************************
                        test_deltafilewrapper.h
                        -----------------------
  begin                : Apr 2020
  copyright            : (C) 2020 by Ivan Ivanov
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

#include <QtTest>

#include <qgsproject.h>

#include "qfield_testbase.h"
#include "qfield.h"
#include "deltafilewrapper.h"
#include "utils/fileutils.h"


class TestDeltaFileWrapper: public QObject
{
    Q_OBJECT
  private slots:
    void initTestCase()
    {
      QTemporaryDir projectDir;
      projectDir.setAutoRemove( false );

      QVERIFY2( projectDir.isValid(), "Failed to create temp dir" );
      QVERIFY2( mTmpFile.open(), "Cannot open temporary delta file" );


      QgsProject::instance()->setPresetHomePath( projectDir.path() );
      QgsProject::instance()->writeEntry( QStringLiteral( "qfieldcloud" ), QStringLiteral( "projectId" ), QStringLiteral( "TEST_PROJECT_ID" ) );

      QFile attachmentFile( QStringLiteral( "%1/%2" ) .arg( projectDir.path(), QStringLiteral( "attachment.jpg" ) ) );

      const char *fileContents = "кирилица"; // SHA 256 71055d022f50027387eae32426a1857d6e2fa2d416d64753b63470db7f00f239
      QVERIFY( attachmentFile.open( QIODevice::ReadWrite ) );
      QVERIFY( attachmentFile.write( fileContents ) );
      QVERIFY( attachmentFile.flush() );

      mAttachmentFileName = attachmentFile.fileName();
      mAttachmentFileChecksum = FileUtils::fileChecksum( mAttachmentFileName ).toHex();

      mLayer.reset( new QgsVectorLayer( QStringLiteral( "Point?crs=EPSG:3857&field=fid:integer&field=int:integer&field=dbl:double&field=str:string&field=attachment:string" ), QStringLiteral( "layer_name" ), QStringLiteral( "memory" ) ) );
      mLayer->setEditorWidgetSetup( mLayer->fields().indexFromName( QStringLiteral( "attachment" ) ), QgsEditorWidgetSetup( QStringLiteral( "ExternalResource" ), QVariantMap() ) );

      QVERIFY( QgsProject::instance()->addMapLayer( mLayer.get(), false, false ) );
    }


    void init()
    {
      mTmpFile.resize( 0 );

      QgsProject::instance()->removeMapLayer( mLayer.get() );

      mLayer.reset( new QgsVectorLayer( QStringLiteral( "Point?crs=EPSG:3857&field=fid:integer&field=int:integer&field=dbl:double&field=str:string&field=attachment:string" ), QStringLiteral( "layer_name" ), QStringLiteral( "memory" ) ) );
      mLayer->setEditorWidgetSetup( mLayer->fields().indexFromName( QStringLiteral( "attachment" ) ), QgsEditorWidgetSetup( QStringLiteral( "ExternalResource" ), QVariantMap() ) );

      QVERIFY( QgsProject::instance()->addMapLayer( mLayer.get(), false, false ) );

      QgsFeature f( mLayer->fields(), 1 );

      f.setAttribute( QStringLiteral( "fid" ), 1 );
      f.setAttribute( QStringLiteral( "dbl" ), 3.14 );
      f.setAttribute( QStringLiteral( "int" ), 42 );
      f.setAttribute( QStringLiteral( "str" ), QStringLiteral( "stringy" ) );
      f.setAttribute( QStringLiteral( "attachment" ), mAttachmentFileName );

      QVERIFY( mLayer->startEditing() );
      QVERIFY( mLayer->addFeature( f ) );
      QVERIFY( mLayer->commitChanges() );
    }


    void testNoMoreThanOneInstance()
    {
      QString fileName( std::tmpnam( nullptr ) );
      DeltaFileWrapper dfw1( mProject, fileName );

      QCOMPARE( dfw1.errorType(), DeltaFileWrapper::NoError );

      DeltaFileWrapper dfw2( mProject, fileName );

      QCOMPARE( dfw2.errorType(), DeltaFileWrapper::LockError );
    }


    void testNoErrorExistingFile()
    {
      QString correctExistingContents = QStringLiteral( R""""(
        {
          "deltas":[],
          "files":[],
          "id":"11111111-1111-1111-1111-111111111111",
          "project":"projectId",
          "version":"1.0"
        }
      )"""" );
      QVERIFY( mTmpFile.write( correctExistingContents.toUtf8() ) );
      mTmpFile.flush();
      DeltaFileWrapper correctExistingDfw( mProject, mTmpFile.fileName() );
      QCOMPARE( correctExistingDfw.errorType(), DeltaFileWrapper::NoError );
      QJsonDocument correctExistingDoc = normalizeSchema( correctExistingDfw.toString() );
      QVERIFY( ! correctExistingDoc.isNull() );
      QCOMPARE( correctExistingDoc, QJsonDocument::fromJson( correctExistingContents.toUtf8() ) );
    }


    void testNoErrorNonExistingFile()
    {
      QString fileName( std::tmpnam( nullptr ) );
      DeltaFileWrapper dfw( mProject, fileName );
      QCOMPARE( dfw.errorType(), DeltaFileWrapper::NoError );
      QVERIFY( QFileInfo::exists( fileName ) );
      DeltaFileWrapper validNonexistingFileCheckDfw( mProject, fileName );
      QFile deltaFile( fileName );
      QVERIFY( deltaFile.open( QIODevice::ReadOnly ) );
      QJsonDocument fileContents = normalizeSchema( deltaFile.readAll() );
      QVERIFY( ! fileContents.isNull() );
      qDebug() << fileContents;
      QCOMPARE( fileContents, QJsonDocument::fromJson( R""""(
        {
          "deltas": [],
          "files":[],
          "id":"11111111-1111-1111-1111-111111111111",
          "project": "projectId",
          "version": "1.0"
        }
      )"""" ) );
    }


    void testErrorInvalidName()
    {
      DeltaFileWrapper dfw( mProject, "" );
      QCOMPARE( dfw.errorType(), DeltaFileWrapper::IOError );
    }


    void testErrorInvalidJsonParse()
    {
      QVERIFY( mTmpFile.write( R""""( asd )"""" ) );
      mTmpFile.flush();
      DeltaFileWrapper dfw( mProject, mTmpFile.fileName() );
      QCOMPARE( dfw.errorType(), DeltaFileWrapper::JsonParseError );
    }


    void testErrorJsonFormatVersionType()
    {
      QVERIFY( mTmpFile.write( R""""({"version":5,"files":[],"id":"11111111-1111-1111-1111-111111111111","project":"projectId","deltas":[]})"""" ) );
      mTmpFile.flush();
      DeltaFileWrapper dfw( mProject, mTmpFile.fileName() );
      QCOMPARE( dfw.errorType(), DeltaFileWrapper::JsonFormatVersionError );
    }


    void testErrorJsonFormatVersionEmpty()
    {
      QVERIFY( mTmpFile.write( R""""({"version":"","files":[],"id":"11111111-1111-1111-1111-111111111111","project":"projectId","deltas":[]})"""" ) );
      mTmpFile.flush();
      DeltaFileWrapper emptyVersionDfw( mProject, mTmpFile.fileName() );
      QCOMPARE( emptyVersionDfw.errorType(), DeltaFileWrapper::JsonFormatVersionError );
    }


    void testErrorJsonFormatVersionValue()
    {
      QVERIFY( mTmpFile.write( R""""({"version":"2.0","files":[],"id":"11111111-1111-1111-1111-111111111111","project":"projectId","deltas":[]})"""" ) );
      mTmpFile.flush();
      DeltaFileWrapper wrongVersionNumberDfw( mProject, mTmpFile.fileName() );
      QCOMPARE( wrongVersionNumberDfw.errorType(), DeltaFileWrapper::JsonIncompatibleVersionError );
    }


    void testErrorJsonFormatIdType()
    {
      QVERIFY( mTmpFile.write( R""""({"version":"2.0","files":[],"id": 5,"project":"projectId","deltas":[]})"""" ) );
      mTmpFile.flush();
      DeltaFileWrapper wrongIdTypeDfw( mProject, mTmpFile.fileName() );
      QCOMPARE( wrongIdTypeDfw.errorType(), DeltaFileWrapper::JsonFormatIdError );
    }


    void testErrorJsonFormatIdEmpty()
    {
      QVERIFY( mTmpFile.write( R""""({"version":"2.0","files":[],"id": "","project":"projectId","deltas":[]})"""" ) );
      mTmpFile.flush();
      DeltaFileWrapper emptyIdDfw( mProject, mTmpFile.fileName() );
      QCOMPARE( emptyIdDfw.errorType(), DeltaFileWrapper::JsonFormatIdError );
    }


    void testErrorJsonFormatProjectIdType()
    {
      QVERIFY( mTmpFile.write( R""""({"version":"2.0","files":[],"id": "11111111-1111-1111-1111-111111111111","project":5,"deltas":[]})"""" ) );
      mTmpFile.flush();
      DeltaFileWrapper wrongProjectIdTypeDfw( mProject, mTmpFile.fileName() );
      QCOMPARE( wrongProjectIdTypeDfw.errorType(), DeltaFileWrapper::JsonFormatProjectIdError );
    }


    void testErrorJsonFormatProjectIdEmpty()
    {
      QVERIFY( mTmpFile.write( R""""({"version":"2.0","files":[],"id": "11111111-1111-1111-1111-111111111111","project":"","deltas":[]})"""" ) );
      mTmpFile.flush();
      DeltaFileWrapper emptyProjectIdDfw( mProject, mTmpFile.fileName() );
      QCOMPARE( emptyProjectIdDfw.errorType(), DeltaFileWrapper::JsonFormatProjectIdError );
    }


    void testErrorJsonFormatDeltasType()
    {
      QVERIFY( mTmpFile.write( R""""({"version":"2.0","files":[],"id": "11111111-1111-1111-1111-111111111111","project":"projectId","deltas":{}})"""" ) );
      mTmpFile.flush();
      DeltaFileWrapper wrongDeltasTypeDfw( mProject, mTmpFile.fileName() );
      QCOMPARE( wrongDeltasTypeDfw.errorType(), DeltaFileWrapper::JsonFormatDeltasError );
    }


    void testFileName()
    {
      QString fileName( std::tmpnam( nullptr ) );
      DeltaFileWrapper dfw( mProject, fileName );
      QCOMPARE( dfw.fileName(), fileName );
    }


    void testId()
    {
      DeltaFileWrapper dfw( mProject, QString( std::tmpnam( nullptr ) ) );

      QVERIFY( ! QUuid::fromString( dfw.id() ).isNull() );
    }


    void testReset()
    {
      DeltaFileWrapper dfw( mProject, QString( std::tmpnam( nullptr ) ) );
      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), QgsFeature() );

      QCOMPARE( getDeltasArray( dfw.toString() ).size(), 1 );

      dfw.reset();

      QCOMPARE( getDeltasArray( dfw.toString() ).size(), 0 );

      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), QgsFeature() );

      QCOMPARE( getDeltasArray( dfw.toString() ).size(), 1 );
    }


    void testResetId()
    {
      DeltaFileWrapper dfw( mProject, QString( std::tmpnam( nullptr ) ) );

      QCOMPARE( getDeltasArray( dfw.toString() ).size(), 0 );

      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), QgsFeature() );

      const QString dfwId = dfw.id();
      dfw.resetId();

      QCOMPARE( getDeltasArray( dfw.toString() ).size(), 1 );
      QVERIFY( dfwId != dfw.id() );
    }


    void testToString()
    {
      DeltaFileWrapper dfw( mProject, QString( std::tmpnam( nullptr ) ) );
      QgsFields fields;
      fields.append( QgsField( "fid", QVariant::Int, "integer" ) );

      QgsFeature f1( fields, 100 );
      f1.setAttribute( "fid", 100 );
      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), f1 );

      QgsFeature f2( fields, 101 );
      f2.setAttribute( "fid", 101 );
      dfw.addDelete( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), f2 );

      QJsonDocument doc = normalizeSchema( dfw.toString() );

      QVERIFY( ! doc.isNull() );
      qDebug() << doc;
      QCOMPARE( doc, QJsonDocument::fromJson( R""""(
        {
          "deltas": [
            {
              "fid": "100",
              "layerId": "dummyLayerId1",
              "method": "create",
              "new": {
                "attributes": {
                  "fid": 100
                },
                "geometry": null
              },
              "tmpFid": "100"
            },
            {
              "fid": "101",
              "layerId": "dummyLayerId1",
              "method": "delete",
              "old": {
                "attributes": {
                  "fid": 101
                },
                "geometry": null
              },
              "tmpFid": "101"
            }
          ],
          "files":[],
          "id": "11111111-1111-1111-1111-111111111111",
          "project": "projectId",
          "version": "1.0"
        }
      )"""" ) );
    }


    void testToJson()
    {
      DeltaFileWrapper dfw( mProject, QString( std::tmpnam( nullptr ) ) );
      QgsFields fields;
      fields.append( QgsField( "fid", QVariant::Int, "integer" ) );

      QgsFeature f1( fields, 100 );
      f1.setAttribute( "fid", 100 );
      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), f1 );

      QgsFeature f2( fields, 101 );
      f2.setAttribute( "fid", 101 );
      dfw.addDelete( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), f2 );

      QJsonDocument doc = normalizeSchema( QString( dfw.toJson() ) );

      QVERIFY( ! doc.isNull() );
      qDebug() << doc;
      QCOMPARE( doc, QJsonDocument::fromJson( R""""(
        {
          "deltas": [
            {
              "fid": "100",
              "layerId": "dummyLayerId1",
              "method": "create",
              "new": {
                "attributes": {
                  "fid": 100
                },
                "geometry": null
              },
              "tmpFid": "100"
            },
            {
              "fid": "101",
              "layerId": "dummyLayerId1",
              "method": "delete",
              "old": {
                "attributes": {
                  "fid": 101
                },
                "geometry": null
              },
              "tmpFid": "101"
            }
          ],
          "files":[],
          "id": "11111111-1111-1111-1111-111111111111",
          "project": "projectId",
          "version": "1.0"
        }
      )"""" ) );
    }


    void testProjectId()
    {
      DeltaFileWrapper dfw( mProject, QString( std::tmpnam( nullptr ) ) );

      QCOMPARE( dfw.projectId(), QStringLiteral( "TEST_PROJECT_ID" ) );
    }


    void testIsDirty()
    {
      QString fileName = std::tmpnam( nullptr );
      DeltaFileWrapper dfw( mProject, fileName );

      QCOMPARE( dfw.isDirty(), false );

      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), QgsFeature() );

      QCOMPARE( dfw.isDirty(), true );
      QVERIFY( dfw.toFile() );
      QCOMPARE( dfw.isDirty(), false );

      dfw.reset();

      QCOMPARE( dfw.isDirty(), true );
    }


    void testCount()
    {
      DeltaFileWrapper dfw( mProject, std::tmpnam( nullptr ) );

      QCOMPARE( dfw.count(), 0 );

      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), QgsFeature() );

      QCOMPARE( dfw.count(), 1 );

      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), QgsFeature() );

      QCOMPARE( dfw.count(), 2 );
    }


    void testDeltas()
    {
      DeltaFileWrapper dfw( mProject, std::tmpnam( nullptr ) );

      QCOMPARE( QJsonDocument( dfw.deltas() ), QJsonDocument::fromJson( "[]" ) );

      QgsFields fields;
      fields.append( QgsField( "fid", QVariant::Int, "integer" ) );
      QgsFeature f1( fields, 100 );
      f1.setAttribute( "fid", 100 );

      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), f1 );

      QCOMPARE( QJsonDocument( normalizeDeltasSchema( dfw.deltas() ) ), QJsonDocument::fromJson( R""""(
        [
          {
            "fid": "100",
            "layerId": "dummyLayerId1",
            "method": "create",
            "new": {
              "attributes": {
                "fid": 100
              },
              "geometry": null
            },
            "tmpFid": "100"
          }
        ]
      )"""" ) );

      QgsFeature f2( fields, 101 );
      f2.setAttribute( "fid", 101 );
      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), f2 );

      QCOMPARE( QJsonDocument( normalizeDeltasSchema( dfw.deltas() ) ), QJsonDocument::fromJson( R""""(
        [
          {
            "fid": "100",
            "layerId": "dummyLayerId1",
            "method": "create",
            "new": {
              "attributes": {
                "fid": 100
              },
              "geometry": null
            },
            "tmpFid": "100"
          },
          {
            "fid": "101",
            "layerId": "dummyLayerId1",
            "method": "create",
            "new": {
              "attributes": {
                "fid": 101
              },
              "geometry": null
            },
            "tmpFid": "101"
          }
        ]
      )"""" ) );
    }


    void testToFile()
    {
      QString fileName = std::tmpnam( nullptr );
      DeltaFileWrapper dfw1( mProject, fileName );
      dfw1.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), QgsFeature() );

      QVERIFY( ! dfw1.hasError() );
      QCOMPARE( getDeltasArray( dfw1.toString() ).size(), 1 );
      QVERIFY( dfw1.toFile() );
      QCOMPARE( getDeltasArray( dfw1.toString() ).size(), 1 );

      QFile deltaFile( fileName );
      QVERIFY( deltaFile.open( QIODevice::ReadOnly ) );
      QCOMPARE( getDeltasArray( deltaFile.readAll() ).size(), 1 );
    }


    void testAppend()
    {
      DeltaFileWrapper dfw1( mProject, QString( std::tmpnam( nullptr ) ) );
      DeltaFileWrapper dfw2( mProject, QString( std::tmpnam( nullptr ) ) );
      dfw1.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), QgsFeature( QgsFields(), 100 ) );
      dfw2.append( &dfw1 );

      QCOMPARE( dfw2.count(), 1 );
    }


    void testAttachmentFieldNames()
    {
      DeltaFileWrapper dfw( mProject, QString( std::tmpnam( nullptr ) ) );

      QStringList attachmentFields = dfw.attachmentFieldNames( mProject, mLayer->id() );

      QCOMPARE( attachmentFields, QStringList( {QStringLiteral( "attachment" )} ) );
    }


    void testAttachmentFileNames()
    {
      QTemporaryFile deltaFile;

      QVERIFY( deltaFile.open() );
      QVERIFY( deltaFile.write( QStringLiteral( R""""(
        {
          "deltas": [
            {
              "fid": "100",
              "layerId": "%1",
              "method": "create",
              "new": {
                "attributes": {
                  "attachment": "FILE1.jpg",
                  "dbl": 3.14,
                  "int": 42,
                  "str": "stringy"
                },
                "files_sha256": {
                  "FILE1.jpg": null
                },
                "geometry": null
              }
            },
            {
              "fid": "102",
              "layerId": "%1",
              "method": "create",
              "new": {
                "attributes": {
                  "attachment": "FILE2.jpg",
                  "dbl": null,
                  "int": null,
                  "str": null
                },
                "files_sha256": {
                  "FILE2.jpg": null
                },
                "geometry": null
              }
            },
            {
              "fid": "102",
              "layerId": "%1",
              "method": "patch",
              "new": {
                "attributes": {
                  "attachment": "FILE3.jpg"
                },
                "files_sha256": {
                  "FILE3.jpg": null
                },
                "geometry": null
              },
              "old": {
                "attributes": {
                  "attachment": "FILE2.jpg"
                },
                "files_sha256": {
                  "FILE2.jpg": null
                },
                "geometry": null
              }
            }
          ],
          "files": [],
          "id": "11111111-1111-1111-1111-111111111111",
          "project": "projectId",
          "version": "1.0"
        }
      )"""" ).arg( mLayer->id() ).toUtf8() ) );
      QVERIFY( deltaFile.flush() );

      DeltaFileWrapper dfw( mProject, deltaFile.fileName() );

      QVERIFY( ! dfw.hasError() );

      QMap<QString, QString> attachmentFileNames = dfw.attachmentFileNames();
      QMap<QString, QString> expectedAttachmentFileNames(
      {
        {"FILE1.jpg", ""},
        {"FILE3.jpg", ""}
      } );

      QCOMPARE( attachmentFileNames, expectedAttachmentFileNames );
    }


    void testAddCreate()
    {
      DeltaFileWrapper dfw( mProject, QString( std::tmpnam( nullptr ) ) );
      QgsFeature f( mLayer->fields(), 100 );
      f.setAttribute( QStringLiteral( "fid" ), 100 );
      f.setAttribute( QStringLiteral( "dbl" ), 3.14 );
      f.setAttribute( QStringLiteral( "int" ), 42 );
      f.setAttribute( QStringLiteral( "str" ), QStringLiteral( "stringy" ) );
      f.setAttribute( QStringLiteral( "attachment" ), mAttachmentFileName );

      // Check if creates delta of a feature with a geometry and existing attachment
      f.setGeometry( QgsGeometry( new QgsPoint( 25.9657, 43.8356 ) ) );
      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), f );

      QCOMPARE( QJsonDocument( getDeltasArray( dfw.toString() ) ), QJsonDocument::fromJson( QStringLiteral( R""""(
        [
          {
            "fid": "100",
            "layerId": "dummyLayerId1",
            "method": "create",
            "new": {
              "attributes": {
                "attachment": "%1",
                "dbl": 3.14,
                "fid": 100,
                "int": 42,
                "str": "stringy"
              },
              "files_sha256": {
                "%1": "%2"
              },
              "geometry": "Point (25.96569999999999823 43.83559999999999945)"
            },
            "tmpFid": "100"
          }
        ]
      )"""" ).arg( mAttachmentFileName, mAttachmentFileChecksum ).toUtf8() ) );


      // Check if creates delta of a feature with a NULL geometry and non existant attachment.
      // NOTE this is the same as calling f clearGeometry()
      dfw.reset();
      f.setGeometry( QgsGeometry() );
      f.setAttribute( QStringLiteral( "attachment" ), std::tmpnam( nullptr ) );
      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), f );

      QCOMPARE( QJsonDocument( getDeltasArray( dfw.toString() ) ), QJsonDocument::fromJson( QStringLiteral( R""""(
        [
          {
            "fid": "100",
            "layerId": "dummyLayerId1",
            "method": "create",
            "new": {
              "attributes": {
                "attachment": "%1",
                "dbl": 3.14,
                "fid": 100,
                "int": 42,
                "str": "stringy"
              },
              "files_sha256": {
                "%1": null
              },
              "geometry": null
            },
            "tmpFid": "100"
          }
        ]
      )"""" ).arg( f.attribute( QStringLiteral( "attachment" ) ).toString() ).toUtf8() ) );


      // Check if creates delta of a feature without attributes
      dfw.reset();

      QgsFields fields;
      fields.append( QgsField( QStringLiteral( "fid" ), QVariant::Int ) );

      QgsFeature f1( fields, 101 );
      f1.setAttribute( QStringLiteral( "fid" ), 101 );
      f1.setGeometry( QgsGeometry( new QgsPoint( 25.9657, 43.8356 ) ) );

      dfw.addCreate( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), f1 );

      QCOMPARE( QJsonDocument( getDeltasArray( dfw.toString() ) ), QJsonDocument::fromJson( R""""(
        [
          {
            "fid": "101",
            "layerId": "dummyLayerId1",
            "method": "create",
            "new": {
              "attributes":{
                "fid": 101
              },
              "geometry": "Point (25.96569999999999823 43.83559999999999945)"
            },
            "tmpFid": "101"
          }
        ]
      )"""" ) );
    }


    void testAddPatch()
    {
      DeltaFileWrapper dfw( mProject, QString( std::tmpnam( nullptr ) ) );
      QgsFeature oldFeature( mLayer->fields(), 100 );
      oldFeature.setAttribute( QStringLiteral( "dbl" ), 3.14 );
      oldFeature.setAttribute( QStringLiteral( "int" ), 42 );
      oldFeature.setAttribute( QStringLiteral( "fid" ), 100 );
      oldFeature.setAttribute( QStringLiteral( "str" ), QStringLiteral( "stringy" ) );
      oldFeature.setAttribute( QStringLiteral( "attachment" ), QString() );
      QgsFeature newFeature( mLayer->fields(), 100 );
      newFeature.setAttribute( QStringLiteral( "dbl" ), 9.81 );
      newFeature.setAttribute( QStringLiteral( "int" ), 680 );
      newFeature.setAttribute( QStringLiteral( "fid" ), 100 );
      newFeature.setAttribute( QStringLiteral( "str" ), QStringLiteral( "pingy" ) );
      newFeature.setAttribute( QStringLiteral( "attachment" ), mAttachmentFileName );


      // Patch both the attributes with existing attachment and the geometry
      oldFeature.setGeometry( QgsGeometry( new QgsPoint( 25.9657, 43.8356 ) ) );
      newFeature.setGeometry( QgsGeometry( new QgsPoint( 23.398819, 41.7672147 ) ) );

      dfw.addPatch( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), oldFeature, newFeature );

      QCOMPARE( QJsonDocument( getDeltasArray( dfw.toString() ) ), QJsonDocument::fromJson( QStringLiteral( R""""(
        [
          {
            "fid": "100",
            "layerId": "dummyLayerId1",
            "method": "patch",
            "new": {
              "attributes": {
                "attachment": "%1",
                "dbl": 9.81,
                "int": 680,
                "str": "pingy"
              },
              "files_sha256": {
                "%1": "%2"
              },
              "geometry": "Point (23.39881899999999959 41.7672146999999967)"
            },
            "old": {
              "attributes": {
                "attachment": null,
                "dbl": 3.14,
                "int": 42,
                "str": "stringy"
              },
              "geometry": "Point (25.96569999999999823 43.83559999999999945)"
            },
            "tmpFid": "100"
          }
        ]
      )"""" ).arg( mAttachmentFileName, mAttachmentFileChecksum ).toUtf8() ) );


      // Patch attributes only with non existing attachnment
      dfw.reset();
      newFeature.setGeometry( QgsGeometry( new QgsPoint( 25.9657, 43.8356 ) ) );
      newFeature.setAttribute( QStringLiteral( "attachment" ), std::tmpnam( nullptr ) );
      oldFeature.setGeometry( QgsGeometry( new QgsPoint( 25.9657, 43.8356 ) ) );

      dfw.addPatch( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), oldFeature, newFeature );

      QCOMPARE( QJsonDocument( getDeltasArray( dfw.toString() ) ), QJsonDocument::fromJson( QStringLiteral( R""""(
        [
          {
            "fid": "100",
            "layerId": "dummyLayerId1",
            "method": "patch",
            "new": {
              "attributes": {
                "attachment": "%1",
                "dbl": 9.81,
                "int": 680,
                "str": "pingy"
              },
              "files_sha256": {
                "%1": null  
              }
            },
            "old": {
              "attributes": {
                "attachment": null,
                "dbl": 3.14,
                "int": 42,
                "str": "stringy"
              }
            },
            "tmpFid": "100"
          }
        ]
      )"""" ).arg( newFeature.attribute( "attachment" ).toString() ).toUtf8() ) );


      // Patch feature without geometry on attributes only with non existant attachment
      dfw.reset();
      newFeature.setGeometry( QgsGeometry() );
      oldFeature.setGeometry( QgsGeometry() );

      dfw.addPatch( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), oldFeature, newFeature );

      // Patch geometry only
      dfw.reset();
      newFeature.setAttribute( QStringLiteral( "dbl" ), 3.14 );
      newFeature.setAttribute( QStringLiteral( "int" ), 42 );
      newFeature.setAttribute( QStringLiteral( "fid" ), 100 );
      newFeature.setAttribute( QStringLiteral( "str" ), QStringLiteral( "stringy" ) );
      newFeature.setAttribute( QStringLiteral( "attachment" ), QVariant() );
      newFeature.setGeometry( QgsGeometry( new QgsPoint( 23.398819, 41.7672147 ) ) );
      oldFeature.setAttribute( QStringLiteral( "attachment" ), QVariant() );
      oldFeature.setGeometry( QgsGeometry( new QgsPoint( 25.9657, 43.8356 ) ) );

      dfw.addPatch( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), oldFeature, newFeature );

      QCOMPARE( QJsonDocument( getDeltasArray( dfw.toString() ) ), QJsonDocument::fromJson( R""""(
        [
          {
            "fid": "100",
            "layerId": "dummyLayerId1",
            "method": "patch",
            "new": {
              "geometry": "Point (23.39881899999999959 41.7672146999999967)"
            },
            "old": {
              "geometry": "Point (25.96569999999999823 43.83559999999999945)"
            },
            "tmpFid": "100"
          }
        ]
      )"""" ) );


      // Do not patch equal features
      dfw.reset();
      oldFeature.setGeometry( QgsGeometry( new QgsPoint( 25.9657, 43.8356 ) ) );
      newFeature.setGeometry( QgsGeometry( new QgsPoint( 25.9657, 43.8356 ) ) );

      dfw.addPatch( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), oldFeature, newFeature );

      QCOMPARE( QJsonDocument( getDeltasArray( dfw.toString() ) ), QJsonDocument::fromJson( "[]" ) );
    }


    void testAddDelete()
    {
      DeltaFileWrapper dfw( mProject, QString( std::tmpnam( nullptr ) ) );
      QgsFeature f( mLayer->fields(), 100 );
      f.setAttribute( QStringLiteral( "fid" ), 100 );
      f.setAttribute( QStringLiteral( "dbl" ), 3.14 );
      f.setAttribute( QStringLiteral( "int" ), 42 );
      f.setAttribute( QStringLiteral( "str" ), QStringLiteral( "stringy" ) );
      f.setAttribute( QStringLiteral( "attachment" ), mAttachmentFileName );

      // Check if creates delta of a feature with a geometry and existant attachment.
      f.setGeometry( QgsGeometry( new QgsPoint( 25.9657, 43.8356 ) ) );
      // ? why this is not working, as QgsPoint is QgsAbstractGeometry and there is example in the docs? https://qgis.org/api/classQgsFeature.html#a14dcfc99b476b613c21b8c35840ff388
      // f.setGeometry( QgsPoint( 25.9657, 43.8356 ) );
      dfw.addDelete( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), f );

      QCOMPARE( QJsonDocument( getDeltasArray( dfw.toString() ) ), QJsonDocument::fromJson( QStringLiteral( R""""(
        [
          {
            "fid": "100",
            "layerId": "dummyLayerId1",
            "method": "delete",
            "old": {
              "attributes": {
                "attachment": "%1",
                "dbl": 3.14,
                "fid": 100,
                "int": 42,
                "str": "stringy"
              },
              "files_sha256": {
                "%1": "%2"
              },
              "geometry": "Point (25.96569999999999823 43.83559999999999945)"
            },
            "tmpFid": "100"
          }
        ]
      )"""" ).arg( mAttachmentFileName, mAttachmentFileChecksum ).toUtf8() ) );


      // Check if creates delta of a feature with a NULL geometry and non existant attachment.
      // NOTE this is the same as calling f clearGeometry()
      dfw.reset();
      f.setGeometry( QgsGeometry() );
      f.setAttribute( QStringLiteral( "attachment" ), std::tmpnam( nullptr ) );
      dfw.addDelete( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), f );

      QCOMPARE( QJsonDocument( getDeltasArray( dfw.toString() ) ), QJsonDocument::fromJson( QStringLiteral( R""""(
        [
          {
            "fid": "100",
            "layerId": "dummyLayerId1",
            "method": "delete",
            "old": {
              "attributes": {
                "attachment": "%1",
                "dbl": 3.14,
                "fid": 100,
                "int": 42,
                "str": "stringy"
              },
              "files_sha256": {
                "%1": null
              },
              "geometry": null
            },
            "tmpFid": "100"
          }
        ]
      )"""" ).arg( f.attribute( QStringLiteral( "attachment" ) ).toString() ).toUtf8() ) );


      // Check if creates delta of a feature without attributes
      dfw.reset();

      QgsFields fields;
      fields.append( QgsField( QStringLiteral( "fid" ), QVariant::Int ) );

      QgsFeature f1( fields, 101 );

      f1.setAttribute( QStringLiteral( "fid" ), 101 );
      f1.setGeometry( QgsGeometry( new QgsPoint( 25.9657, 43.8356 ) ) );
      dfw.addDelete( mLayer->id(), QStringLiteral( "fid" ), QStringLiteral( "fid" ), f1 );

      QCOMPARE( QJsonDocument( getDeltasArray( dfw.toString() ) ), QJsonDocument::fromJson( R""""(
        [
          {
            "fid": "101",
            "layerId": "dummyLayerId1",
            "method": "delete",
            "old": {
              "attributes": {
                "fid": 101
              },
              "geometry": "Point (25.96569999999999823 43.83559999999999945)"
            },
            "tmpFid": "101"
          }
        ]
      )"""" ) );
    }

    void testMultipleDeltaAdd()
    {
      DeltaFileWrapper dfw( mProject, QString( std::tmpnam( nullptr ) ) );
      QgsFields fields;
      fields.append( QgsField( "dbl", QVariant::Double, "double" ) );
      fields.append( QgsField( "int", QVariant::Int, "integer" ) );
      fields.append( QgsField( "fid", QVariant::Int, "integer" ) );
      fields.append( QgsField( "str", QVariant::String, "text" ) );
      QgsFeature f1( fields, 100 );
      f1.setAttribute( QStringLiteral( "dbl" ), 3.14 );
      f1.setAttribute( QStringLiteral( "int" ), 42 );
      f1.setAttribute( QStringLiteral( "fid" ), 100 );
      f1.setAttribute( QStringLiteral( "str" ), QStringLiteral( "stringy" ) );

      QgsFields fields2;
      fields2.append( QgsField( "fid", QVariant::Int, "integer" ) );
      QgsFeature f2( fields2, 101 );
      f2.setGeometry( QgsGeometry( new QgsPoint( 25.9657, 43.8356 ) ) );
      f2.setAttribute( QStringLiteral( "fid" ), 101 );

      QgsFeature f3( fields, 102 );
      f3.setAttribute( QStringLiteral( "fid" ), 102 );

      dfw.addCreate( "dummyLayerId1", QStringLiteral( "fid" ), QStringLiteral( "fid" ), f1 );
      dfw.addDelete( "dummyLayerId2", QStringLiteral( "fid" ), QStringLiteral( "fid" ), f2 );
      dfw.addDelete( "dummyLayerId1", QStringLiteral( "fid" ), QStringLiteral( "fid" ), f3 );

      QJsonDocument doc = normalizeSchema( dfw.toString() );

      QVERIFY( ! doc.isNull() );
      QCOMPARE( doc, QJsonDocument::fromJson( R""""(
        {
          "deltas": [
            {
              "fid": "100",
              "layerId": "dummyLayerId1",
              "method": "create",
              "new": {
                "attributes": {
                  "dbl": 3.14,
                  "fid": 100,
                  "int": 42,
                  "str": "stringy"
                },
                "geometry": null
              },
              "tmpFid": "100"
            },
            {
              "fid": "101",
              "layerId": "dummyLayerId2",
              "method": "delete",
              "old": {
                "attributes": {
                  "fid": 101
                },
                "geometry": "Point (25.96569999999999823 43.83559999999999945)"
              },
              "tmpFid": "101"
            },
            {
              "fid": "102",
              "layerId": "dummyLayerId1",
              "method": "delete",
              "old": {
                "attributes": {
                  "dbl": null,
                  "fid": 102,
                  "int": null,
                  "str": null
                },
                "geometry": null
              },
              "tmpFid": "102"
            }
          ],
          "files":[],
          "id": "11111111-1111-1111-1111-111111111111",
          "project": "projectId",
          "version": "1.0"
        }
      )"""" ) );
    }


    void testApply()
    {
      QTemporaryFile deltaFile;

      QVERIFY( deltaFile.open() );
      QVERIFY( deltaFile.write( QStringLiteral( R""""(
        {
          "deltas": [
            {
              "fid": "100",
              "layerId": "%1",
              "method": "create",
              "new": {
                "attributes": {
                  "attachment": "FILE1.jpg",
                  "dbl": 3.14,
                  "fid": 100,
                  "int": 42,
                  "str": "stringy"
                },
                "files_sha256": {
                  "FILE1.jpg": null
                },
                "geometry": null
              },
              "tmpFid": "100"
            },
            {
              "fid": "102",
              "layerId": "%1",
              "method": "create",
              "new": {
                "attributes": {
                  "attachment": "FILE2.jpg",
                  "dbl": null,
                  "fid": 102,
                  "int": null,
                  "str": null
                },
                "files_sha256": {
                  "FILE2.jpg": null
                },
                "geometry": null
              },
              "tmpFid": "102"
            },
            {
              "fid": "102",
              "layerId": "%1",
              "method": "patch",
              "new": {
                "attributes": {
                  "attachment": "FILE3.jpg"
                },
                "files_sha256": {
                  "FILE3.jpg": null
                },
                "geometry": null
              },
              "old": {
                "attributes": {
                  "attachment": "FILE2.jpg"
                },
                "files_sha256": {
                  "FILE2.jpg": null
                },
                "geometry": null
              },
              "tmpFid": "102"
            },
            {
              "fid": "1",
              "layerId": "%1",
              "method": "delete",
              "old": {
                "attachment": "%2",
                "dbl": 3.14,
                "fid": 1,
                "int": 42,
                "str": "stringy"
              },
              "tmpFid": "1"
            }
          ],
          "files": [],
          "id": "11111111-1111-1111-1111-111111111111",
          "project": "projectId",
          "version": "1.0"
        }
      )"""" ).arg( mLayer->id(), mAttachmentFileName ).toUtf8() ) );
      QVERIFY( deltaFile.flush() );

      DeltaFileWrapper dfw( mProject, deltaFile.fileName() );

      // make sure there is a single feature with id 1
      QgsFeature f0;
      QgsFeatureIterator it0 = mLayer->getFeatures( QgsFeatureRequest( QgsExpression( " fid = 1 " ) ) );
      QVERIFY( it0.nextFeature( f0 ) );
      QCOMPARE( mLayer->featureCount(), 1 );

      QVERIFY( dfw.apply() );

      QCOMPARE( mLayer->featureCount(), 2 );

      QgsFeature f1;
      QgsFeatureIterator it1 = mLayer->getFeatures( QgsFeatureRequest( QgsExpression( " fid = 100 " ) ) );

      QVERIFY( it1.nextFeature( f1 ) );
      QVERIFY( f1.isValid() );
      QCOMPARE( f1.attribute( QStringLiteral( "int" ) ), 42 );
      QCOMPARE( f1.attribute( QStringLiteral( "dbl" ) ), 3.14 );
      QCOMPARE( f1.attribute( QStringLiteral( "str" ) ), QStringLiteral( "stringy" ) );
      QCOMPARE( f1.attribute( QStringLiteral( "attachment" ) ), QStringLiteral( "FILE1.jpg" ) );
      QVERIFY( ! it1.nextFeature( f1 ) );

      QgsFeature f2;
      QgsFeatureIterator it2 = mLayer->getFeatures( QgsFeatureRequest( QgsExpression( " fid = 102 " ) ) );

      QVERIFY( it2.nextFeature( f2 ) );
      QVERIFY( f2.isValid() );
      QCOMPARE( f2.attribute( QStringLiteral( "attachment" ) ).toString(), QStringLiteral( "FILE3.jpg" ) );
      QVERIFY( ! it2.nextFeature( f2 ) );

      QgsFeature f3;
      QgsFeatureIterator it3 = mLayer->getFeatures( QgsFeatureRequest( QgsExpression( " fid = 1 " ) ) );
      QVERIFY( ! it3.nextFeature( f3 ) );
    }


    void testApplyReversed()
    {
      QTemporaryFile deltaFile;

      QVERIFY( deltaFile.open() );
      QVERIFY( deltaFile.write( QStringLiteral( R""""(
        {
          "deltas": [
            {
              "fid": "100",
              "layerId": "%1",
              "method": "create",
              "new": {
                "attributes": {
                  "attachment": "FILE1.jpg",
                  "dbl": 3.14,
                  "fid": 100,
                  "int": 42,
                  "str": "stringy"
                },
                "files_sha256": {
                  "FILE1.jpg": null
                },
                "geometry": null
              },
              "tmpFid": "100"
            },
            {
              "fid": "102",
              "layerId": "%1",
              "method": "create",
              "new": {
                "attributes": {
                  "attachment": "FILE2.jpg",
                  "dbl": null,
                  "fid": 102,
                  "int": null,
                  "str": null
                },
                "files_sha256": {
                  "FILE2.jpg": null
                },
                "geometry": null
              },
              "tmpFid": "102"
            },
            {
              "fid": "102",
              "layerId": "%1",
              "method": "patch",
              "new": {
                "attributes": {
                  "attachment": "FILE3.jpg"
                },
                "files_sha256": {
                  "FILE3.jpg": null
                },
                "geometry": null
              },
              "old": {
                "attributes": {
                  "attachment": "FILE2.jpg"
                },
                "files_sha256": {
                  "FILE2.jpg": null
                },
                "geometry": null
              },
              "tmpFid": "102"
            },
            {
              "fid": "1",
              "layerId": "%1",
              "method": "delete",
              "old": {
                "attributes": {
                  "attachment": "%2",
                  "dbl": 3.14,
                  "fid": 1,
                  "int": 42,
                  "str": "stringy"
                },
                "files_sha256": {
                  "%2": "%3"
                }
              },
              "tmpFid": "1"
            }
          ],
          "files": [],
          "id": "11111111-1111-1111-1111-111111111111",
          "project": "projectId",
          "version": "1.0"
        }
      )"""" ).arg( mLayer->id(), mAttachmentFileName, mAttachmentFileChecksum ).toUtf8() ) );
      QVERIFY( deltaFile.flush() );

      DeltaFileWrapper dfw( mProject, deltaFile.fileName() );

      // make sure there is a single feature with id 1
      QgsFeature f0;
      QgsFeatureIterator it0 = mLayer->getFeatures( QgsFeatureRequest( QgsExpression( " fid = 1 " ) ) );
      QVERIFY( it0.nextFeature( f0 ) );
      QCOMPARE( mLayer->featureCount(), 1 );

      QVERIFY( dfw.apply() );

      QCOMPARE( mLayer->featureCount(), 2 );

      QgsFeature f1;
      QgsFeatureIterator it1 = mLayer->getFeatures( QgsFeatureRequest( QgsExpression( " fid = 100 " ) ) );

      QVERIFY( it1.nextFeature( f1 ) );
      QVERIFY( f1.isValid() );
      QCOMPARE( f1.attribute( QStringLiteral( "int" ) ), 42 );
      QCOMPARE( f1.attribute( QStringLiteral( "dbl" ) ), 3.14 );
      QCOMPARE( f1.attribute( QStringLiteral( "str" ) ), QStringLiteral( "stringy" ) );
      QCOMPARE( f1.attribute( QStringLiteral( "attachment" ) ), QStringLiteral( "FILE1.jpg" ) );
      QVERIFY( ! it1.nextFeature( f1 ) );

      QgsFeature f2;
      QgsFeatureIterator it2 = mLayer->getFeatures( QgsFeatureRequest( QgsExpression( " fid = 102 " ) ) );

      QVERIFY( it2.nextFeature( f2 ) );
      QVERIFY( f2.isValid() );
      QCOMPARE( f2.attribute( QStringLiteral( "attachment" ) ).toString(), QStringLiteral( "FILE3.jpg" ) );
      QVERIFY( ! it2.nextFeature( f2 ) );

      QgsFeature f3;
      QgsFeatureIterator it3 = mLayer->getFeatures( QgsFeatureRequest( QgsExpression( " fid = 1 " ) ) );
      QVERIFY( ! it3.nextFeature( f3 ) );

      // ^^^ the same as apply above

      dfw.applyReversed();

      QgsFeature f4;
      QgsFeatureIterator it4 = mLayer->getFeatures( QgsFeatureRequest( QgsExpression( " fid = 100 OR fid = 102 " ) ) );
      QVERIFY( ! it4.nextFeature( f4 ) );

      QgsFeature f5;
      QgsFeatureIterator it5 = mLayer->getFeatures( QgsFeatureRequest( QgsExpression( " fid = 1 " ) ) );
      QVERIFY( it5.nextFeature( f5 ) );
      QVERIFY( f5.isValid() );
      QCOMPARE( f5.attribute( QStringLiteral( "fid" ) ), 1 );
      QCOMPARE( f5.attribute( QStringLiteral( "int" ) ), 42 );
      QCOMPARE( f5.attribute( QStringLiteral( "dbl" ) ), 3.14 );
      QCOMPARE( f5.attribute( QStringLiteral( "str" ) ), QStringLiteral( "stringy" ) );
      QCOMPARE( f5.attribute( QStringLiteral( "attachment" ) ), mAttachmentFileName );
      QVERIFY( ! it5.nextFeature( f5 ) );
    }


  private:
    QgsProject *mProject = QgsProject::instance();


    QTemporaryFile mTmpFile;


    std::unique_ptr<QgsVectorLayer> mLayer;


    QString mAttachmentFileName;


    QString mAttachmentFileChecksum;


    /**
     * Normalized the random part of the delta file JSON schema to static values.
     * "id"         - "11111111-1111-1111-1111-111111111111"
     * "project"  - "projectId"
     *
     * @param json - JSON string
     * @return QJsonDocument normalized JSON document. NULL document if the input is invalid.
     */
    QJsonDocument normalizeSchema( const QString &json )
    {
      QJsonDocument doc = QJsonDocument::fromJson( json.toUtf8() );

      if ( doc.isNull() )
        return doc;

      QJsonObject o = doc.object();
      QJsonArray deltas;

      if ( o.value( QStringLiteral( "version" ) ).toString() != DeltaFileWrapper::FormatVersion )
        return QJsonDocument();
      if ( o.value( QStringLiteral( "project" ) ).toString().size() == 0 )
        return QJsonDocument();
      if ( QUuid::fromString( o.value( QStringLiteral( "id" ) ).toString() ).isNull() )
        return QJsonDocument();
      if ( ! o.value( QStringLiteral( "deltas" ) ).isArray() )
        return QJsonDocument();
      if ( ! o.value( QStringLiteral( "files" ) ).isArray() )
        return QJsonDocument();

      // normalize non-constant values
      o.insert( QStringLiteral( "id" ), QStringLiteral( "11111111-1111-1111-1111-111111111111" ) );
      o.insert( QStringLiteral( "project" ), QStringLiteral( "projectId" ) );
      o.insert( QStringLiteral( "deltas" ), normalizeDeltasSchema( o.value( QStringLiteral( "deltas" ) ).toArray() ) );
      o.insert( QStringLiteral( "files" ), normalizeFilesSchema( o.value( QStringLiteral( "files" ) ).toArray() ) );

      return QJsonDocument( o );
    }


    QJsonArray normalizeDeltasSchema( const QJsonArray &deltasJson )
    {
      QStringList layerIds;
      QJsonArray deltas;

      // normalize layerIds
      for ( const QJsonValue &v : deltasJson )
      {
        QJsonObject deltaItem = v.toObject();
        const QString layerId = deltaItem.value( QStringLiteral( "layerId" ) ).toString();

        if ( ! layerIds.contains( layerId ) )
          layerIds.append( layerId );

        int layerIdx = layerIds.indexOf( layerId ) + 1;

        deltaItem.insert( QStringLiteral( "layerId" ), QStringLiteral( "dummyLayerId%1" ).arg( layerIdx ) );
        deltas.append( deltaItem );
      }

      return deltas;
    }


    QJsonArray normalizeFilesSchema( const QJsonArray &filesJson )
    {
      QJsonArray files;

      // normalize file names
      int i = 0;
      while ( i < filesJson.size() )
        files.append( QStringLiteral( "file%1.jpg" ).arg( i++ ) );

      return files;
    }


    QJsonArray getDeltasArray( const QString &json )
    {
      return normalizeSchema( json.toUtf8() )
             .object()
             .value( QStringLiteral( "deltas" ) )
             .toArray();
    }
};

QFIELDTEST_MAIN( TestDeltaFileWrapper )
#include "test_deltafilewrapper.moc"
