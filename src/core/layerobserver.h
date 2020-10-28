/***************************************************************************
                        layerobserver.h
                        ---------------
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

#ifndef LAYEROBSERVER_H
#define LAYEROBSERVER_H


#include "deltafilewrapper.h"

#include <QList>
#include <qgsfeature.h>
#include <qgsmaplayer.h>
#include <qgsproject.h>
#include <qgsvectorlayer.h>

typedef QMap<QgsFeatureId, QgsFeature> QgsChangedFeatures;

/**
 * Monitors all layers for changes and writes those changes to a delta file
 */
class LayerObserver : public QObject
{
    Q_OBJECT

    Q_PROPERTY( DeltaFileWrapper *currentDeltaFileWrapper READ currentDeltaFileWrapper WRITE setCurrentDeltaFileWrapper NOTIFY currentDeltaFileWrapperChanged )
    Q_PROPERTY( DeltaFileWrapper *committedDeltaFileWrapper READ committedDeltaFileWrapper WRITE setCommittedDeltaFileWrapper NOTIFY committedDeltaFileWrapperChanged )

  public:
    /**
     * Construct a new Layer Observer object
     *
     * @param project
     */
    explicit LayerObserver( const QgsProject *project );


    /**
     * Generates a new complete name with path for deltas file
     *
     * @param isCurrentDeltaFile if true, no timestamp is appended
     * @return QString
     */
    QString generateDeltaFileName( bool isCurrentDeltaFile = false );


    /**
     * Returns whether delta file writing has an error
     *
     * @return bool
     */
    bool hasError() const;


    /**
     * Starts new delta file and finishes writing for the old one
     *
     * @return bool whether the commit was successful
     */
    Q_INVOKABLE bool commit();


    /**
     * Clears the current delta file changes
     */
    Q_INVOKABLE void reset( bool isHardReset = false ) const;


    /**
     * Gets the current delta file
     *
     * @return current delta file
     */
    DeltaFileWrapper *currentDeltaFileWrapper() const;
    void setCurrentDeltaFileWrapper( DeltaFileWrapper *deltaFileWrapper );


    /**
     * Gets the committed delta file
     *
     * @return committed delta file
     */
    DeltaFileWrapper *committedDeltaFileWrapper() const;
    void setCommittedDeltaFileWrapper( DeltaFileWrapper *deltaFileWrapper );


    /**
     * Add the needed event listeners to monitor for changes.
     * Assigns listeners only for layer actions of `cloud` and `offline`.
     */
    void addLayerListeners();

  signals:
    void layerEdited( const QString &layerId );
    void currentDeltaFileWrapperChanged();
    void committedDeltaFileWrapperChanged();
    void deltaFileCommitted();


  private slots:
    /**
     * Monitors the current project for new layers.
     *
     * @param layers layers added
     */
    void onLayersAdded( const QList<QgsMapLayer *> &layers );


    /**
     * Commit the changes of the current delta file and
     *
     */
//    void onHomePathChanged();


    /**
     * Extracts the old values of the modified features
     */
    void onBeforeCommitChanges();


    /**
     * Writes the "create" deltas
     *
     * @param layerId layer ID
     * @param addedFeatures new features
     */
    void onCommittedFeaturesAdded( const QString &layerId, const QgsFeatureList &addedFeatures );


    /**
     * Writes the "delete" deltas
     *
     * @param layerId layer ID
     * @param deletedFeatureIds old feature IDs
     */
    void onCommittedFeaturesRemoved( const QString &layerId, const QgsFeatureIds &deletedFeatureIds );


    /**
     * Writes the "patch" deltas
     *
     * @param layerId
     * @param changedAttributesValues
     */
    void onCommittedAttributeValuesChanges( const QString &layerId, const QgsChangedAttributesMap &changedAttributesValues );


    /**
     * Writes the "patch" deltas.
     *
     * @param layerId
     * @param changedGeometries
     */
    void onCommittedGeometriesChanges( const QString &layerId, const QgsGeometryMap &changedGeometries );


    /**
     * Writes the deltas to the delta file
     */
    void onEditingStopped();


  private:
    /**
     * The current Deltas File Wrapper object
     */
    DeltaFileWrapper *mCurrentDeltaFileWrapper;


    /**
     * The commited Deltas File Wrapper object
     */
    DeltaFileWrapper *mCommittedDeltaFileWrapper;


    /**
     * The current project instance
     */
    const QgsProject *mProject = nullptr;


    /**
     * Store the old version of changed (patch or delete) features per layer.
     * key    - layer ID
     * value  - changed features for that layer
     */
    QMap<QString, QgsChangedFeatures> mChangedFeatures;


    /**
     * Store the old version of patched features per layer.
     * key    - layer ID
     * value  - patched feature IDs for that layer
     */
    QMap<QString, QgsFeatureIds> mPatchedFids;

};

#endif // LAYEROBSERVER_H
