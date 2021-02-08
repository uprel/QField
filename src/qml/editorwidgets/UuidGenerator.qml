import QtQuick 2.14
import QtQuick.Controls 2.12

import org.qfield 1.0
import Theme 1.0

import "."

EditorWidgetBase {
  id: topItem

  height: childrenRect.height

  Label {
    id: textReadonlyValue
    height: fontMetrics.height + 20
    topPadding: 10
    bottomPadding: 10
    visible: height !== 0 && !isEnabled
    anchors.left: parent.left
    anchors.right: parent.right
    font: Theme.defaultFont
    color: 'gray'
    wrapMode: Text.Wrap

    text: value == null ? '' : value
  }

  TextField {
    id: textField
    topPadding: 10
    bottomPadding: 10
    visible: height !== 0 && isEnabled
    anchors.left: parent.left
    anchors.right: parent.right
    font: Theme.defaultFont
    color: 'black'
    wrapMode: TextInput.Wrap

    text: {
      if (value == null) {
        var val = UuidUtils.createUuid();
        valueChanged(val, false)
        return val
      }
      return value
    }
    readOnly: true

    background: Rectangle {
      y: textField.height - height - textField.bottomPadding / 2
      implicitWidth: 120
      height: textField.activeFocus ? 2: 1
      color: textField.activeFocus ? "#4CAF50" : "#C8E6C9"
    }

    onTextChanged: {
      valueChanged( text, text == '' )
    }
  }

  FontMetrics {
    id: fontMetrics
    font: textField.font
  }
}
