import QtQuick 2.6
import org.qgis 1.0
import "js/style.js" as Style

VisibilityFadingRow {
  id: digitizingToolbar
  property RubberbandModel rubberbandModel
  property bool isDigitizing: rubberbandModel ? rubberbandModel.vertexCount > 1 : false //!< Readonly

  spacing: 4 * dp
  padding: 4 * dp

  signal vertexAdded
  signal vertexRemoved
  signal cancel
  signal confirm

  Button {
    id: cancelButton
    iconSource: Style.getThemeIcon( "ic_clear_white_24dp" )
    visible: rubberbandModel.vertexCount > 1
    round: true
    bgcolor: "#900000"

    onClicked: {
      cancel()
    }
  }

  Button {
    id: confirmButton
    iconSource: {
      Style.getThemeIcon( "ic_save_white_24dp" )
    }
    visible: {
      if ( Number( rubberbandModel ? rubberbandModel.geometryType : 0 ) === 0 || stateMachine.state === 'measure' )
      {
        false
      }
      else if  ( Number( rubberbandModel.geometryType ) === 1 )
      {
        // Line: at least 2 points (last point not saved)
        rubberbandModel.vertexCount > 2
      }
      else if  ( Number( rubberbandModel.geometryType ) === 2 )
      {
        // Polygon: at least 3 points (last point not saved)
        rubberbandModel.vertexCount > 3
      }
    }
    round: true
    bgcolor: "#80cc28"

    onClicked: {
      // remove editing vertex for lines and polygons
      vertexRemoved()
      confirm()
    }
  }

  Button {
    id: removeVertexButton
    iconSource: Style.getThemeIcon( "ic_remove_white_24dp" )
    visible: rubberbandModel.vertexCount > 1
    round: true
    bgcolor: "#212121"

    onClicked: {
      vertexRemoved()
    }
  }

  Button {
    id: addVertexButton
    iconSource: {
        Style.getThemeIcon( "ic_add_white_24dp" )
    }
    round: true
    bgcolor: stateMachine.state === 'measure' ? "#000000": Number( rubberbandModel ? rubberbandModel.geometryType : 0 ) === QgsWkbTypes.PointGeometry ? "#80cc28" : "#212121"

    onClicked: {
      if ( Number( rubberbandModel.geometryType ) === QgsWkbTypes.PointGeometry ||
           Number( rubberbandModel.geometryType ) === QgsWkbTypes.NullGeometry )
        confirm()
      else
        vertexAdded()
    }
  }
}